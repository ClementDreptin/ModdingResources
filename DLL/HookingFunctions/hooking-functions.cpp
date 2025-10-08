#include <xtl.h>

#include <cstdint>
#include <string>

// Get the address of a function from a module by its ordinal
void *ResolveFunction(const std::string &moduleName, uint32_t ordinal)
{
    HMODULE moduleHandle = GetModuleHandle(moduleName.c_str());
    if (moduleHandle == nullptr)
        return nullptr;

    return GetProcAddress(moduleHandle, reinterpret_cast<const char *>(ordinal));
}

// Create a pointer to XNotifyQueueUI in xam.xex
typedef void (*XNOTIFYQUEUEUI)(uint32_t type, uint32_t userIndex, uint64_t areas, const wchar_t *displayText, void *pContextData);
XNOTIFYQUEUEUI XNotifyQueueUI = static_cast<XNOTIFYQUEUEUI>(ResolveFunction("xam.xex", 656));

// Enum for game title IDs
typedef enum _TitleId
{
    Title_Dashboard = 0xFFFE07D1,
    Title_MW2 = 0x41560817,
} TitleId;

// Imports from the Xbox libraries
extern "C"
{
    uint32_t XamGetCurrentTitleId();

    uint32_t ExCreateThread(
        HANDLE *pHandle,
        uint32_t stackSize,
        uint32_t *pThreadId,
        void *pApiThreadStartup,
        PTHREAD_START_ROUTINE pStartAddress,
        void *pParameter,
        uint32_t creationFlags
    );

    bool MmIsAddressValid(void *pAddress);
}

void InitMW2();

bool g_Running = true;

// Infinitely check the current game running
uint32_t MonitorTitleId(void *pThreadParameter)
{
    uint32_t currentTitleId = 0;

    while (g_Running)
    {
        uint32_t newTitleId = XamGetCurrentTitleId();

        if (newTitleId == currentTitleId)
            continue;

        currentTitleId = newTitleId;

        switch (newTitleId)
        {
        case Title_Dashboard:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
            break;
        case Title_MW2:
            if (!strcmp(reinterpret_cast<char *>(0x82001270), "multiplayer"))
                InitMW2();
            break;
        }
    }

    return 0;
}

#define MAX_HOOK_COUNT 100
#define NUM_INSTRUCTIONS_IN_JUMP 4
#define POWERPC_B 0x48
#define POWERPC_BL 0x4B

class Detour
{
public:
    Detour(void *pSource, const void *pDestination)
        : m_pSource(pSource), m_pDestination(pDestination), m_HookIndex(static_cast<size_t>(-1))
    {
    }

    Detour(uintptr_t sourceAddress, const void *pDestination)
        : m_pSource(reinterpret_cast<void *>(sourceAddress)), m_pDestination(pDestination), m_HookIndex(static_cast<size_t>(-1))
    {
    }

    ~Detour()
    {
        Remove();
    }

    HRESULT Install()
    {
        if (s_HookCount >= MAX_HOOK_COUNT || m_pSource == nullptr || m_pDestination == nullptr)
            return E_FAIL;

        if (s_CriticalSection.Synchronization.RawEvent[0] == 0)
            InitializeCriticalSection(&s_CriticalSection);

        EnterCriticalSection(&s_CriticalSection);

        // Keep track of where the stub of the current instance is in s_StubSection
        m_HookIndex = s_HookCount;

        // Copy the original instructions at m_pSource before hooking to be able to
        // restore them later
        memcpy(&m_Original, m_pSource, sizeof(m_Original));

        DetourFunctionStart();

        s_HookCount++;

        LeaveCriticalSection(&s_CriticalSection);

        return S_OK;
    }

    void Remove()
    {
        // Restore the original instructions if needed
        if (m_HookIndex != -1 && m_pSource != nullptr && MmIsAddressValid(m_pSource))
            memcpy(m_pSource, &m_Original, sizeof(m_Original));

        m_pSource = nullptr;
        m_pDestination = nullptr;
        m_HookIndex = 0;
        m_Original = Jump();
    }

    template<typename T>
    inline T GetOriginal() const
    {
        return reinterpret_cast<T>(&s_StubSection[m_HookIndex]);
    }

private:
    typedef uint32_t POWERPC_INSTRUCTION;
    typedef uint8_t POWERPC_INSTRUCTION_TYPE;

    struct Stub
    {
        POWERPC_INSTRUCTION Instructions[20]; // DetourFunctionStart can copy up to 20 instructions

        Stub() { ZeroMemory(&Instructions, sizeof(Instructions)); }
    };

    struct Jump
    {
        POWERPC_INSTRUCTION Instructions[NUM_INSTRUCTIONS_IN_JUMP];

        Jump() { ZeroMemory(&Instructions, sizeof(Instructions)); }
    };

    void *m_pSource;
    const void *m_pDestination;
    size_t m_HookIndex;
    Jump m_Original;

    static Stub s_StubSection[MAX_HOOK_COUNT];
    static size_t s_HookCount;
    static CRITICAL_SECTION s_CriticalSection;

    void DetourFunctionStart()
    {
        POWERPC_INSTRUCTION *pSource = static_cast<POWERPC_INSTRUCTION *>(m_pSource);
        POWERPC_INSTRUCTION *pStub = reinterpret_cast<POWERPC_INSTRUCTION *>(&s_StubSection[m_HookIndex]);
        size_t instructionCount = 0;

        for (size_t i = 0; i < NUM_INSTRUCTIONS_IN_JUMP; i++)
        {
            POWERPC_INSTRUCTION instruction = pSource[i];
            POWERPC_INSTRUCTION_TYPE instructionType = *reinterpret_cast<POWERPC_INSTRUCTION_TYPE *>(&pSource[i]);

            // If the function op code is null, it's invalid
            if (instruction == 0)
                break;

            // If the instruction is a branch
            if (instructionType == POWERPC_B || instructionType == POWERPC_BL)
            {
                // Get a pointer to where the branch goes
                void *pBranchDestination = ResolveBranch(instruction, &pSource[i]);
                bool linked = (instruction & 1) != 0;

                // Jump from the stub to where the branch goes
                PatchInJump(&pStub[instructionCount], pBranchDestination, linked);
                instructionCount += NUM_INSTRUCTIONS_IN_JUMP;

                // If it was a branch to a different section of the same function (b loc_),
                // we won't need to add anything else to the stub
                if (!linked)
                {
                    PatchInJump(pSource, m_pDestination, false);
                    return;
                }
            }
            // Otherwise, just copy the instruction to the stub
            else
            {
                pStub[instructionCount] = instruction;
                instructionCount++;
            }
        }

        // Make the stub call the original function
        PatchInJump(&pStub[instructionCount], &pSource[NUM_INSTRUCTIONS_IN_JUMP], false);

        // Make the original function call the stub
        PatchInJump(pSource, m_pDestination, false);
    }

    void PatchInJump(void *pSource, const void *pDestination, bool linked)
    {
        Jump jump;
        uintptr_t destinationAddress = reinterpret_cast<uintptr_t>(pDestination);

        jump.Instructions[0] = 0x3C000000 + (destinationAddress >> 16);    // lis    %r0, dest>>16
        jump.Instructions[1] = 0x60000000 + (destinationAddress & 0xFFFF); // ori    %r0, %r0, dest&0xFFFF
        jump.Instructions[2] = 0x7C0903A6;                                 // mtctr  %r0
        jump.Instructions[3] = 0x4E800420 + (linked ? 1 : 0);              // bctr/bctrl

        memcpy(pSource, &jump, sizeof(jump));

        __dcbst(0, pSource);
        __sync();
        __emit(0x4C00012C);
    }

    void *ResolveBranch(POWERPC_INSTRUCTION instruction, const void *pBranch)
    {
        // Taken from here
        // https://github.com/skiff/libpsutil/blob/master/libpsutil/system/memory.cpp#L90

        uintptr_t offset = instruction & 0x3FFFFFC;

        if (offset & (1 << 25))
            offset |= 0xFC000000;

        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pBranch) + offset);
    }
};

// This will hold all the instructions for all hooks. This needs to be a static buffer because,
// if it was a class member, it would be allocated on the stack and stack memory isn't executable
Detour::Stub Detour::s_StubSection[MAX_HOOK_COUNT];
size_t Detour::s_HookCount = 0;
CRITICAL_SECTION Detour::s_CriticalSection = { 0 };

// Function we found in the previous section
void (*SV_GameSendServerCommand)(int clientNum, int type, const char *text) = reinterpret_cast<void (*)(int, int, const char *)>(0x822548D8);

Detour *pSV_ExecuteClientCommandDetour = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    pSV_ExecuteClientCommandDetour->GetOriginal<decltype(&SV_ExecuteClientCommandHook)>()(client, s, clientOK, fromOldServer);

    // Printing the command in the killfeed
    std::string command = "f \"";
    command += s;
    command += "\"";
    SV_GameSendServerCommand(-1, 0, command.c_str());
}

// Sets up the hook
void InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);

    const uintptr_t SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    pSV_ExecuteClientCommandDetour = new Detour(SV_ExecuteClientCommandAddr, SV_ExecuteClientCommandHook);
    pSV_ExecuteClientCommandDetour->Install();
}

HANDLE g_ThreadHandle = INVALID_HANDLE_VALUE;

// State variables for enabling notifications in system threads
static uint16_t defaultInstruction = 0;
static uintptr_t patchAddress = 0x816A3158;

BOOL DllMain(HINSTANCE hModule, DWORD reason, void *pReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        // Allow notifications to be displayed from system threads
        if (defaultInstruction == 0)
            defaultInstruction = *reinterpret_cast<uint16_t *>(patchAddress);
        *reinterpret_cast<uint16_t *>(patchAddress) = 0x4800;

        // Run MonitorTitleId in separate thread
        ExCreateThread(&g_ThreadHandle, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
        break;
    case DLL_PROCESS_DETACH:
        // Remove patch for system thread notifications
        if (defaultInstruction != 0)
            *reinterpret_cast<uint16_t *>(patchAddress) = defaultInstruction;

        g_Running = false;

        if (pSV_ExecuteClientCommandDetour)
            delete pSV_ExecuteClientCommandDetour;

        // Wait for the run thread to finish
        WaitForSingleObject(g_ThreadHandle, INFINITE);
        CloseHandle(g_ThreadHandle);
        break;
    }

    return TRUE;
}

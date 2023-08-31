#pragma once

// Gets the address of the function within a module by its ordinal
void *ResolveFunction(const std::string &moduleName, uint32_t ordinal);

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef void (*XNOTIFYQUEUEUI)(uint32_t type, uint32_t userIndex, uint64_t areas, const wchar_t *displayText, void *pContextData);
extern XNOTIFYQUEUEUI XNotifyQueueUI;

// Enum for game title IDs
enum Games
{
    GAME_DASHBOARD = 0xFFFE07D1,
    GAME_MW2 = 0x41560817
};

// Imports from the Xbox libraries
extern "C"
{
    DWORD XamGetCurrentTitleId();

    DWORD __stdcall ExCreateThread(
        HANDLE *pHandle,
        DWORD dwStackSize,
        DWORD *pThreadId,
        void *apiThreadStartup,
        PTHREAD_START_ROUTINE pStartAddress,
        void *pParameter,
        DWORD dwCreationFlagsMod
    );

    bool MmIsAddressValid(void *pAddress);
}

// Maintains the main loop state
extern bool g_Running;

// Infinitely checks the current game running
uint32_t MonitorTitleId(void *pThreadParameter);

#define POWERPC_BRANCH_OPTIONS_ALWAYS 20
#define TRAMPOLINE_BUFFER_MAX_SIZE 0x1000

typedef uint32_t POWERPC_INSTRUCTION;

// Original version made my iMoD1998 (https://gist.github.com/iMoD1998/4aa48d5c990535767a3fc3251efc0348)
class Detour
{
public:
    Detour(uintptr_t hookSourceAddress, const void *pHookTarget);
    Detour(void *pHookSource, const void *pHookTarget);

    ~Detour();

    bool Install();

    bool Remove();

    template<typename T>
    inline T GetOriginal() const
    {
        return reinterpret_cast<T>(m_pTrampolineDestination);
    }

private:
    const void *m_pHookTarget;

    void *m_pHookSource;

    void *m_pTrampolineDestination;

    uint8_t m_OriginalInstructions[30];

    size_t m_OriginalLength;

    static uint8_t s_TrampolineBuffer[TRAMPOLINE_BUFFER_MAX_SIZE];

    static size_t s_TrampolineSize;

    // Function that contains to constructor logic, it's meant to share the same logic for multiple constructors.
    // This is necessary because C++0x doesn't support calling one constructor from another constructor.
    void Init(void *pHookSource, const void *pHookTarget);

    // Write both conditional and unconditional branches using the count register to the destination address that will branch to the target address.
    size_t WriteFarBranch(void *pDestination, const void *pBranchTarget, bool linked = false, bool preserveRegister = false, uint32_t branchOptions = POWERPC_BRANCH_OPTIONS_ALWAYS, uint8_t conditionRegisterBit = 0, uint8_t registerIndex = 0);

    // Copy and fix relative branch instructions to a new location.
    size_t RelocateBranch(void *pDestination, const void *pSource);

    // Copy an instruction enusuring things such as PC relative offsets are fixed.
    size_t CopyInstruction(void *pDestination, const void *pSource);
};

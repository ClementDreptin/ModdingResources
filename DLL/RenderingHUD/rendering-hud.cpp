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

// HUD API structs and enums
typedef enum _he_type_t
{
    HE_TYPE_FREE,
    HE_TYPE_TEXT,
    HE_TYPE_VALUE,
    HE_TYPE_PLAYERNAME,
    HE_TYPE_MATERIAL,
    HE_TYPE_MAPNAME,
    HE_TYPE_GAMETYPE,
    HE_TYPE_TIMER_DOWN,
    HE_TYPE_TIMER_UP,
    HE_TYPE_TIMER_STATIC,
    HE_TYPE_TENTHS_TIMER_DOWN,
    HE_TYPE_TENTHS_TIMER_UP,
    HE_TYPE_CLOCK_DOWN,
    HE_TYPE_CLOCK_UP,
    HE_TYPE_WAYPOINT,
    HE_TYPE_COUNT,
} he_type_t;

typedef enum _align_t
{
    ALIGN_TOP_LEFT = 0,
    ALIGN_MIDDLE_LEFT = 1,
    ALIGN_BOTTOM_LEFT = 2,
    ALIGN_TOP_MIDDLE = 4,
    ALIGN_MIDDLE_MIDDLE = 5,
    ALIGN_BOTTOM_MIDDLE = 6,
    ALIGN_TOP_RIGHT = 8,
    ALIGN_MIDDLE_RIGHT = 9,
    ALIGN_BOTTOM_RIGHT = 10
} align_t;

struct hudelem_color_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct hudelem_s
{
    he_type_t type;
    float y;
    float x;
    float z;
    int targetEntNum;
    float fontScale;
    float fromFontScale;
    int fontScaleStartTime;
    int fontScaleTime;
    int label;
    int font;
    align_t alignOrg;
    align_t alignScreen;
    hudelem_color_t color;
    hudelem_color_t fromColor;
    int fadeStartTime;
    int fadeTime;
    int height;
    int width;
    int materialIndex;
    int fromHeight;
    int fromWidth;
    int scaleStartTime;
    int scaleTime;
    float fromY;
    float fromX;
    int fromAlignOrg;
    int fromAlignScreen;
    int moveStartTime;
    int moveTime;
    float value;
    int time;
    int duration;
    int text;
    float sort;
    hudelem_color_t glowColor;
    int fxBirthTime;
    int fxLetterTime;
    int fxDecayStartTime;
    int fxDecayDuration;
    int soundID;
    int flags;
};

struct game_hudelem_s
{
    hudelem_s elem;
    int clientNum;
    int teamNum;
    int archived;
};

// HUD API functions
game_hudelem_s *(*HudElem_Alloc)(int clientNum, int teamNum) = reinterpret_cast<game_hudelem_s *(*)(int, int)>(0x821DF928);

int (*G_MaterialIndex)(const char *name) = reinterpret_cast<int (*)(const char *)>(0x8220C960);

int (*G_LocalizedStringIndex)(const char *string) = reinterpret_cast<int (*)(const char *)>(0x8220C7A0);

// Rendering API structs
struct Color
{
    float r;
    float g;
    float b;
    float a;
};

struct Font_s
{
    int fontName;
    int pixelHeight;
    int glyphCount;
    int material;
    int glowMaterial;
    int glyphs;
};

// Rendering API functions
void (*R_AddCmdDrawStretchPic)(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const float *color, HANDLE material) =
    reinterpret_cast<void (*)(float, float, float, float, float, float, float, float, const float *, HANDLE)>(0x8234F9B8);

void (*R_AddCmdDrawText)(const char *text, int maxChars, Font_s *font, float x, float y, float xScale, float yScale, float rotation, const float *color, int style) =
    reinterpret_cast<void (*)(const char *, int, Font_s *, float, float, float, float, float, const float *, int)>(0x82350278);

Font_s *(*R_RegisterFont)(const char *font, int imageTrack) = reinterpret_cast<Font_s *(*)(const char *, int)>(0x8234DCB0);

HANDLE(*Material_RegisterHandle)
(const char *name, int imageTrack) = reinterpret_cast<HANDLE (*)(const char *, int)>(0x8234E510);

// Used to change some dvars
void (*SV_GameSendServerCommand)(int clientNum, int type, const char *text) = reinterpret_cast<void (*)(int, int, const char *)>(0x822548D8);

Detour *pSCR_DrawScreenFieldDetour = nullptr;
HANDLE materialHandle = nullptr;
Font_s *pFont = nullptr;
Color black = { 0.0f, 0.0f, 0.0f, 0.0f };
Color white = { 1.0f, 1.0f, 1.0f, 0.0f };

void SCR_DrawScreenFieldHook(const int localClientNum, int refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    pSCR_DrawScreenFieldDetour->GetOriginal<decltype(&SCR_DrawScreenFieldHook)>()(localClientNum, refreshedUI);

    // Register the white material the first time we draw
    if (!materialHandle)
        materialHandle = Material_RegisterHandle("white", 0);

    // Register the normal font the first time we draw
    if (!pFont)
        pFont = R_RegisterFont("fonts/normalFont", 0);

    // Rendering the rectangle only if the alpha channel is positive
    if (black.a > 0.0f)
        R_AddCmdDrawStretchPic(5.0f, 5.0f, 400.0f, 710.0f, 0.0f, 0.0f, 1.0f, 1.0f, reinterpret_cast<float *>(&black), materialHandle);

    // Rendering the text only if the alpha channel is positive
    if (white.a > 0.0f)
    {
        const char *text = "Rendering API Text";
        R_AddCmdDrawText(text, strlen(text), pFont, 90.0f, 375.0f, 1.0f, 1.0f, 0.0f, reinterpret_cast<float *>(&white), 0);
    }
}

Detour *pSV_ExecuteClientCommandDetour = nullptr;
game_hudelem_s *pRectangleElem = nullptr;
game_hudelem_s *pTextElem = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    pSV_ExecuteClientCommandDetour->GetOriginal<decltype(&SV_ExecuteClientCommandHook)>()(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19"))
    {
        // Creating the rectangle and removing localization warnings only the first time we press the button
        if (!pRectangleElem)
        {
            // Removing UNLOCALIZED() around texts
            SV_GameSendServerCommand(0, 0, "s loc_warnings \"0\"");
            SV_GameSendServerCommand(0, 0, "s loc_warningsUI \"0\"");

            pRectangleElem = HudElem_Alloc(0, 0);
            pRectangleElem->elem.x = 441.0f;
            pRectangleElem->elem.y = 5.0f;
            pRectangleElem->elem.width = 300;
            pRectangleElem->elem.height = 470;
            pRectangleElem->elem.color.r = 0;
            pRectangleElem->elem.color.g = 0;
            pRectangleElem->elem.color.b = 0;
            pRectangleElem->elem.color.a = 0;
            pRectangleElem->elem.type = HE_TYPE_MATERIAL;
            pRectangleElem->elem.alignOrg = ALIGN_TOP_LEFT;
            pRectangleElem->elem.alignScreen = ALIGN_TOP_LEFT;
            pRectangleElem->elem.sort = 0.0f;
            pRectangleElem->elem.materialIndex = G_MaterialIndex("white");
        }

        // Creating the text only the first time we press the button
        if (!pTextElem)
        {
            pTextElem = HudElem_Alloc(0, 0);
            pTextElem->elem.x = 441.0f + (300.0f / 2.0f) + (5.0f / 2.0f);
            pTextElem->elem.y = 5.0f + (470.0f / 2.0f);
            pTextElem->elem.color.r = 255;
            pTextElem->elem.color.g = 255;
            pTextElem->elem.color.b = 255;
            pTextElem->elem.color.a = 0;
            pTextElem->elem.type = HE_TYPE_TEXT;
            pTextElem->elem.alignOrg = ALIGN_MIDDLE_MIDDLE;
            pTextElem->elem.alignScreen = ALIGN_TOP_LEFT;
            pTextElem->elem.font = 4;
            pTextElem->elem.sort = 1.0f;
            pTextElem->elem.fontScale = 2.0f;
            pTextElem->elem.text = G_LocalizedStringIndex("HUD API Text");
        }

        // Toggle the visibility of the rectangle and the text from the HUD API
        if (!pRectangleElem->elem.color.a)
        {
            pRectangleElem->elem.color.a = 255;
            pTextElem->elem.color.a = 255;
        }
        else
        {
            pRectangleElem->elem.color.a = 0;
            pTextElem->elem.color.a = 0;
        }

        // Toggle the visibility of the rectangle and the text from the rendering API
        if (!black.a)
        {
            black.a = 1.0f;
            white.a = 1.0f;
        }
        else
        {
            black.a = 0.0f;
            white.a = 0.0f;
        }
    }
}

// Sets up the hook
void InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);

    const uintptr_t SV_ExecuteClientCommandAddr = 0x82253140;
    const uintptr_t SCR_DrawScreenFieldAddr = 0x8214BEB8;

    // Hooking SV_ExecuteClientCommand and SCR_DrawScreenField
    pSV_ExecuteClientCommandDetour = new Detour(SV_ExecuteClientCommandAddr, SV_ExecuteClientCommandHook);
    pSCR_DrawScreenFieldDetour = new Detour(SCR_DrawScreenFieldAddr, SCR_DrawScreenFieldHook);
    pSV_ExecuteClientCommandDetour->Install();
    pSCR_DrawScreenFieldDetour->Install();
}

int DllMain(HANDLE hModule, DWORD reason, void *pReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        // Runs MonitorTitleId in separate thread
        ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
        break;
    case DLL_PROCESS_DETACH:
        g_Running = false;

        if (pSV_ExecuteClientCommandDetour)
            delete pSV_ExecuteClientCommandDetour;

        if (pSCR_DrawScreenFieldDetour)
            delete pSCR_DrawScreenFieldDetour;

        // We give the system some time to clean up the thread before exiting
        Sleep(250);
        break;
    }

    return TRUE;
}

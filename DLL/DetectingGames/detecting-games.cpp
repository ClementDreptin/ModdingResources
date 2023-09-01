#include <xtl.h>
#include <string>
#include <cstdint>

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
enum Games
{
    GAME_DASHBOARD = 0xFFFE07D1,
    GAME_MW2 = 0x41560817
};

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
}

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
        case GAME_DASHBOARD:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
            break;
        case GAME_MW2:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);
            break;
        }
    }

    return 0;
}

int DllMain(HANDLE hModule, DWORD reason, void *pReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        // Run MonitorTitleId in separate thread
        ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
        break;
    case DLL_PROCESS_DETACH:
        g_Running = false;
        // We give the system some time to clean up the thread before exiting
        Sleep(250);
        break;
    }

    return TRUE;
}

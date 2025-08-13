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
        case Title_Dashboard:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
            break;
        case Title_MW2:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);
            break;
        }
    }

    return 0;
}

HANDLE g_ThreadHandle = INVALID_HANDLE_VALUE;

// State variables for enabling notifications in system threads
static uint32_t defaultInstruction = 0;
static uintptr_t patchAddress = 0x816A3158;

BOOL DllMain(HINSTANCE hModule, DWORD reason, void *pReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        // Allow notifications to be displayed from system threads
        if (defaultInstruction == 0)
            defaultInstruction = *reinterpret_cast<uint32_t *>(patchAddress);
        *reinterpret_cast<uint32_t *>(patchAddress) = 0x4800001C;

        // Run MonitorTitleId in separate thread
        ExCreateThread(&g_ThreadHandle, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
        break;
    case DLL_PROCESS_DETACH:
        // Remove patch for system thread notifications
        if (defaultInstruction != 0)
            *reinterpret_cast<uint32_t *>(patchAddress) = defaultInstruction;

        g_Running = false;
        // Wait for the run thread to finish
        WaitForSingleObject(g_ThreadHandle, INFINITE);
        break;
    }

    return TRUE;
}

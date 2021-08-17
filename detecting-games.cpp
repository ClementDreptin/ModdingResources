#include "pch.h"

// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(LPCSTR moduleName, DWORD ordinal)
{
    HMODULE mHandle = GetModuleHandle(moduleName);

    return (mHandle == NULL) ? NULL : (DWORD)GetProcAddress(mHandle, (LPCSTR)ordinal);
}

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef VOID (*XNOTIFYQUEUEUI)(DWORD exnq, DWORD dwUserIndex, ULONGLONG qwAreas, PWCHAR displayText, PVOID contextData);
XNOTIFYQUEUEUI XNotifyQueueUI = (XNOTIFYQUEUEUI)ResolveFunction("xam.xex", 656);

// Enum for game title IDs
enum Games : DWORD
{
	DASHBOARD = 0xFFFE07D1,
	MW2 = 0x41560817
};

// Import the XamGetCurrentTitleId function from xam
extern "C" DWORD XamGetCurrentTitleId();

DWORD MonitorTitleId(LPVOID lpThreadParameter)
{
    DWORD currentTitle;

    while (true)
    {
        DWORD newTitle = XamGetCurrentTitleId();
        if (newTitle != currentTitle)
        {
            switch (newTitle)
            {
                case DASHBOARD:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", 0);
                    break;
                case MW2:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", 0);
                    break;
            }
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
            // Runs MonitorTitleId in separate thread
			CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MonitorTitleId, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

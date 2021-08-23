#include "pch.h"

// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(LPCSTR moduleName, DWORD ordinal)
{
    HMODULE mHandle = GetModuleHandle(moduleName);

    return (mHandle == NULL) ? NULL : (DWORD)GetProcAddress(mHandle, (LPCSTR)ordinal);
}

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef VOID (*XNOTIFYQUEUEUI)(DWORD exnq, DWORD dwUserIndex, ULONGLONG qwAreas, PWCHAR displayText, LPVOID contextData);
XNOTIFYQUEUEUI XNotifyQueueUI = (XNOTIFYQUEUEUI)ResolveFunction("xam.xex", 656);

// Enum for game title IDs
enum Games : DWORD
{
    DASHBOARD = 0xFFFE07D1,
    MW2 = 0x41560817
};

// Imports from the Xbox libraries
extern "C" 
{
    DWORD XamGetCurrentTitleId();

    DWORD __stdcall ExCreateThread(
        LPHANDLE pHandle,
        DWORD dwStackSize,
        LPDWORD lpThreadId,
        LPVOID apiThreadStartup,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameters,
        DWORD dwCreationFlagsMod
    );
}

DWORD MonitorTitleId(LPVOID lpThreadParameter)
{
    DWORD currentTitle;

    while (true)
    {
        DWORD newTitle = XamGetCurrentTitleId();

        if (newTitle != currentTitle)
        {
            currentTitle = newTitle;

            switch (newTitle)
            {
                case DASHBOARD:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
                    break;
                case MW2:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);
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
            ExCreateThread(nullptr, 0, nullptr, nullptr, (LPTHREAD_START_ROUTINE)MonitorTitleId, nullptr, 2);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

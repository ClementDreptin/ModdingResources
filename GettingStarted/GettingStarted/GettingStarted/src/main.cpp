#include "pch.h"


// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(LPCSTR szModuleName, DWORD dwOrdinal)
{
    HMODULE hModule = GetModuleHandle(szModuleName);

    return (hModule == NULL) ? NULL : (DWORD)GetProcAddress(hModule, (LPCSTR)dwOrdinal);
}

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef VOID (*XNOTIFYQUEUEUI)(DWORD dwType, DWORD dwUserIndex, ULONGLONG qwAreas, PWCHAR wszDisplayText, LPVOID pContextData);
XNOTIFYQUEUEUI XNotifyQueueUI = (XNOTIFYQUEUEUI)ResolveFunction("xam.xex", 656);

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Hello World!", nullptr);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

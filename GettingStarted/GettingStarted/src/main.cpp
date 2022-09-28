#include "pch.h"

// Gets the address of the function within a module by its ordinal
void *ResolveFunction(const std::string &moduleName, uint32_t ordinal)
{
    HMODULE hModule = GetModuleHandle(moduleName.c_str());

    return (hModule == NULL) ? NULL : GetProcAddress(hModule, reinterpret_cast<const char *>(ordinal));
}

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef void (*XNOTIFYQUEUEUI)(uint32_t type, uint32_t userIndex, uint64_t areas, const wchar_t *displayText, void *pContextData);
XNOTIFYQUEUEUI XNotifyQueueUI = static_cast<XNOTIFYQUEUEUI>(ResolveFunction("xam.xex", 656));

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, void *pReserved)
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

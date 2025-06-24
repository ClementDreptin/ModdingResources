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

int DllMain(HANDLE hModule, DWORD reason, void *pReserved)
{
    uint32_t defaultInstruction = 0;
    uintptr_t patchAddress = 0x816A3158;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (defaultInstruction == 0)
            defaultInstruction = *reinterpret_cast<uint32_t *>(patchAddress);
        *reinterpret_cast<uint32_t *>(patchAddress) = 0x4800001C;

        XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Hello World!", nullptr);

        break;
    case DLL_PROCESS_DETACH:
        if (defaultInstruction != 0)
            *reinterpret_cast<uint32_t *>(patchAddress) = defaultInstruction;

        break;
    }

    return TRUE;
}

#include "pch.h"

#include "..\Utils\Utils.h"

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

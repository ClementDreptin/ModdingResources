#include "pch.h"

#include "..\Utils\Utils.h"


BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, void *pReserved)
{
    switch (fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
            // Runs MonitorTitleId in separate thread
            ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
            break;
        case DLL_PROCESS_DETACH:
            g_bRunning = FALSE;
            // We give the system some time to clean up the thread before exiting
            Sleep(250);
            break;
    }

    return TRUE;
}

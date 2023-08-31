#include "pch.h"

#include "../Utils/Utils.h"

// Function we found in the previous section
void (*SV_GameSendServerCommand)(int clientNum, int type, const char *text) = reinterpret_cast<void (*)(int, int, const char *)>(0x822548D8);

Detour *pSV_ExecuteClientCommandDetour = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    pSV_ExecuteClientCommandDetour->GetOriginal<decltype(&SV_ExecuteClientCommandHook)>()(client, s, clientOK, fromOldServer);

    // Printing the command in the killfeed
    std::string command = "f \"";
    command += s;
    command += "\"";
    SV_GameSendServerCommand(-1, 0, command.c_str());
}

// Sets up the hook
void InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    const uintptr_t SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    pSV_ExecuteClientCommandDetour = new Detour(SV_ExecuteClientCommandAddr, SV_ExecuteClientCommandHook);
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, void *pReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Runs MonitorTitleId in separate thread
        ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
        break;
    case DLL_PROCESS_DETACH:
        g_Running = false;

        if (pSV_ExecuteClientCommandDetour)
            delete pSV_ExecuteClientCommandDetour;

        // We give the system some time to clean up the thread before exiting
        Sleep(250);
        break;
    }

    return TRUE;
}

#include "pch.h"

#include "..\Utils\Utils.h"


// Function we found in the previous section
void (*SV_GameSendServerCommand)(int clientNum, int type, const char *text) = reinterpret_cast<void(*)(int, int, const char *)>(0x822548D8);

void __declspec(naked) SV_ExecuteClientCommandStub(int client, const char *s, int clientOK, int fromOldServer)
{
    // The stub needs to, at least, contain 7 instructions
    __asm
    {
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    }
}

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Printing the command in the killfeed
    std::string strCommand = "f \"";
    strCommand += s;
    strCommand += "\"";
    SV_GameSendServerCommand(-1, 0, strCommand.c_str());
}

// Sets up the hook
void InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    const DWORD SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart(reinterpret_cast<DWORD *>(SV_ExecuteClientCommandAddr), reinterpret_cast<DWORD *>(SV_ExecuteClientCommandStub), reinterpret_cast<DWORD>(SV_ExecuteClientCommandHook));
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
            g_bRunning = FALSE;
            // We give the system some time to clean up the thread before exiting
            Sleep(250);
            break;
    }

    return TRUE;
}

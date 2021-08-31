#include "pch.h"

#include "..\Utils\Utils.h"

// Function we found in the previous section
VOID (*SV_GameSendServerCommand)(INT clientNum, INT type, LPCSTR text) = (VOID(*)(INT, INT, LPCSTR))0x822548D8;

__declspec(naked) VOID SV_ExecuteClientCommandStub(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
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

VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Printing the command in the killfeed
    std::string command = "f \"";
    command += s;
    command += "\"";
    SV_GameSendServerCommand(-1, 0, command.c_str());
}

// Sets up the hook
VOID InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    CONST DWORD SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart((LPDWORD)SV_ExecuteClientCommandAddr, (LPDWORD)SV_ExecuteClientCommandStub, (DWORD)SV_ExecuteClientCommandHook);
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

#pragma once


// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(LPCSTR szModuleName, DWORD dwOrdinal);

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef VOID (*XNOTIFYQUEUEUI)(XNOTIFYQUEUEUI_TYPE dwType, DWORD dwUserIndex, ULONGLONG qwAreas, PWCHAR wszDisplayText, LPVOID pContextData);
extern XNOTIFYQUEUEUI XNotifyQueueUI;

// Enum for game title IDs
enum Games
{
    GAME_DASHBOARD = 0xFFFE07D1,
    GAME_MW2 = 0x41560817
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

// Maintains the main loop state
BOOL g_bRunning = TRUE;

// Infinitely checks the current game running
DWORD MonitorTitleId(LPVOID lpThreadParameter);

// Hooks a function
VOID HookFunctionStart(LPDWORD lpdwAddress, LPDWORD lpdwSaveStub, DWORD dwDestination);

#pragma once

// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(const std::string &strModuleName, DWORD dwOrdinal);

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef void (*XNOTIFYQUEUEUI)(XNOTIFYQUEUEUI_TYPE dwType, DWORD dwUserIndex, unsigned long long qwAreas, const wchar_t *wszDisplayText, void *pContextData);
extern XNOTIFYQUEUEUI XNotifyQueueUI;

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
        HANDLE *pHandle,
        DWORD dwStackSize,
        DWORD *pThreadId,
        void *apiThreadStartup,
        PTHREAD_START_ROUTINE pStartAddress,
        void *pParameter,
        DWORD dwCreationFlagsMod
    );
}

// Infinitely checks the current game running
DWORD MonitorTitleId(void *pThreadParameter);

// Hooks a function
void HookFunctionStart(DWORD *pdwAddress, DWORD *pdwSaveStub, DWORD dwDestination);

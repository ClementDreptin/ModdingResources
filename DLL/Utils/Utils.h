// Gets the address of the function within a module by its ordinal
DWORD ResolveFunction(LPCSTR moduleName, DWORD ordinal);

// Creates a function pointer from the address of XNotifyQueueUI retrieved by ResolveFunction
typedef VOID (*XNOTIFYQUEUEUI)(DWORD exnq, DWORD dwUserIndex, ULONGLONG qwAreas, PWCHAR displayText, LPVOID contextData);
XNOTIFYQUEUEUI XNotifyQueueUI = (XNOTIFYQUEUEUI)ResolveFunction("xam.xex", 656);

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
        LPHANDLE pHandle,
        DWORD dwStackSize,
        LPDWORD lpThreadId,
        LPVOID apiThreadStartup,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameters,
        DWORD dwCreationFlagsMod
    );
}

// Infinitely checks the current game running
DWORD MonitorTitleId(LPVOID lpThreadParameter);

// Hooks a function
VOID HookFunctionStart(LPDWORD address, LPDWORD saveStub, DWORD destination);

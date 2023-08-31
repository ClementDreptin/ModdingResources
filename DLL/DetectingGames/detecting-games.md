# Detecting games

Since your DLL is loaded on start-up and not when the game you want to mod starts, you need a way to detect when the user launches a specific game.
What I and most people do is creating a constantly running loop that will check the title ID of the game currently running to see if it's equal to the title ID of the game we want to mod.

First, we need a few things that will help us detect what game is currently running:

```C++
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
        HANDLE *pHandle,
        DWORD dwStackSize,
        DWORD *pThreadId,
        void *apiThreadStartup,
        PTHREAD_START_ROUTINE pStartAddress,
        void *pParameter,
        DWORD dwCreationFlagsMod
    );
}
```

We are going to create a function that runs forever and checks if the title ID of the currently running game has changed since the last loop iteration:

```C++
void MonitorTitleId()
{
    uint32_t currentTitleId = 0;

    while (g_Running)
    {
        uint32_t newTitleId = XamGetCurrentTitleId();

        if (newTitleId != currentTitleId)
        {
            currentTitleId = newTitleId;

            switch (newTitleId)
            {
                case GAME_DASHBOARD:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
                    break;
                case GAME_MW2:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);
                    break;
            }
        }
    }
}
```

We can't just call `MonitorTitleId` when `DllMain` receives the `DLL_PROCESS_ATTACH` reason because this will heavily slow down the main thread. That's why we need to run `MonitorTitleId` in a separate thread.
By looking at the signature of `ExCreateThread`, we see that the function pointer needs to be of type `PTHREAD_START_ROUTINE`, which is `DWORD (WINAPI *PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);`. Which means we need the signature of our `MonitorTitleId` function to become:

```C++
uint32_t MonitorTitleId(void *pThreadParameter)
{
    uint32_t currentTitleId = 0;

    while (g_Running)
    {
        uint32_t newTitleId = XamGetCurrentTitleId();

        if (newTitleId != currentTitleId)
        {
            currentTitleId = newTitleId;

            switch (newTitleId)
            {
                case GAME_DASHBOARD:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
                    break;
                case GAME_MW2:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);
                    break;
            }
        }
    }

    return 0;
}
```

Now, to run `MonitorTitleId` in a separate thread, simply call `ExCreateThread` and pass a pointer to `MonitorTitleId` to it in your main function like so:

```C++
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
            // We give the system some time to clean up the thread before exiting
            Sleep(250);
            break;
    }

    return TRUE;
}
```

You might be wondering why we are using `ExCreateThread` instead of `CreateThread`, which is a function provided by Windows in a header file. The reason is that, in order to keep your thread running even when you switch games, you need to pass `2` in the `dwCreationFlagsMod` argument. Passing `2` to the `dwCreationFlags` argument of `CreateThread` does not keep your thread running when you switch games.

<br/>

**Note:** The code example shown in this page is available [here](detecting-games.cpp).

<br/><br/>

&rarr; [Next: Finding game functions](../finding-functions.md)

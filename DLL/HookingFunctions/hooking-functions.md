# Hooking functions
Here's how the [Wikipedia page about hooking](https://en.wikipedia.org/wiki/Hooking) defines the concept:

>*In computer programming, the term hooking covers a range of techniques used to alter or augment the behaviour of an operating system, of applications, or of other software components by intercepting function calls or messages or events passed between software components. Code that handles such intercepted function calls, events or messages is called a hook.*

Put in a more concise way, hooking, and specifically function hooking in our case, is making a function from the game jump to one of our functions.

## Why?
We need to hook functions to be notified on certain events. The most common scenario is button inputs, we want to know when a button is pressed and what button was pressed. You could argue that we don't need to hook a function for button inputs because the XDK already provides us with the `XInputGetState` function. The problem is that this function needs to be called in an update loop and our update loop is not specific to a game, it's global to our DLL, adding extra checks in this loop to see what game is currently running would slow it down and it becomes messy if you want to listen for different button inputs on each game. Another thing is that the game already did all the work of creating an event system around button inputs, so why reinventing the wheel?

## Writing a hook
The following examples use the [`Detour` class](../Utils/Utils.h#L46) declared in [`Utils.h`](../Utils/Utils.h). This class is a slightly modified version of [iMoD1998](https://github.com/iMoD1998)'s [Detour class](https://gist.github.com/iMoD1998/4aa48d5c990535767a3fc3251efc0348). You can use it like so:
```C++
Detour *pGameFunctionDetour = nullptr;

void GameFunctionHook(int param1, int param2)
{
    // We call the initial function to keep the original behavior intact
    pGameFunctionDetour->GetOriginal<decltype(&GameFunctionHook)>()(param1, param2);

    std::cout << "GameFunction hooked!\n";
}

void Init()
{
    const uintptr_t gameFunctionAddr = 0x82345678;

    pGameFunctionDetour = new Detour(gameFunctionAddr, GameFunctionHook);
}

void Cleanup()
{
    if (pGameFunctionDetour)
        delete pGameFunctionDetour;
}
```
We have successfully hooked a function! Now, every time `GameFunction` gets called it will execute its original code then print "GameFunction hooked!" in the console.

## Real world example
MW2 has a function called `SV_ExecuteClientCommand` that monitors button inputs. Hooking it is going to allow us to know when the user presses or releases a button. We are just going to print the command passed to the function in the killfeed, it's up to you to imagine what you can do with it!

**Note**: Commands passed to `SV_ExecuteClientCommand` are not always explicit strings. For button events, the string has the following format: `n <number>`. There are two events per button, a button pressed event and a button released event, the event numbers for one button are consecutive. For example, the dpad left pressed event is `n 19` and the dpad left released event is `n 20`.

First, we need to create our detour pointer and hook function:
```C++
Detour *pSV_ExecuteClientCommandDetour = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    pSV_ExecuteClientCommandDetour->GetOriginal<decltype(&SV_ExecuteClientCommandHook)>()(client, s, clientOK, fromOldServer);
}
```
At the moment, we only call the original function in the hook so nothing new is happening. We are going to use the `SV_GameSendServerCommand` function that we found in the previous section to print the command passed to `SV_ExecuteClientCommand` in the killfeed.
```C++
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
```
Now we need to create a function that sets up the hook:
```C++
void InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    const uintptr_t SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    pSV_ExecuteClientCommandDetour = new Detour(SV_ExecuteClientCommandAddr, SV_ExecuteClientCommandHook);
}
```
The last thing to do is calling this function when MW2 is launched (in the switch statement of `MonitorTitleId`):
```C++
switch (newTitleId)
{
    case GAME_MW2:
        InitMW2();
        break;
}
```

You can now get on MW2 with your DLL loaded, press buttons while being in a game and you should see the commands in the killfeed.

<br/>

**Note:** The code example shown in this page is available [here](hooking-functions.cpp).

<br/><br/>

&rarr; [Next: Rendering HUD](../RenderingHUD/rendering-hud.md)

# Hooking functions
Here's how the [Wikipedia page about hooking](https://en.wikipedia.org/wiki/Hooking) defines the concept:

>*In computer programming, the term hooking covers a range of techniques used to alter or augment the behaviour of an operating system, of applications, or of other software components by intercepting function calls or messages or events passed between software components. Code that handles such intercepted function calls, events or messages is called a hook.*

Put in a more concise way, hooking, and specifically function hooking in our case, is making a function from the game jump to one of our functions.

## Why?
We need to hook functions to be notified on certain events. The most common scenario is button inputs, we want to know when a button is pressed and what button was pressed. You could argue that we don't need to hook a function for button inputs because the XDK already provides us with the `XInputGetState` function. The problem is that this function needs to be called in an update loop and our update loop is not specific to a game, it's global to our DLL, adding extra checks in this loop to see what game is currently running would slow it down and it becomes messy if you want to listen for different button inputs on each game. Another thing is that the game already did all the work of creating an event system around button inputs, so why reinventing the wheel?

## Utility functions
**Disclaimer:** I'm not an expert on the subject, I understand the concept but I don't master it enough to write my own functions. In my projects I use the ones from [The Tesseract on Xbox 360 for Modern Warfare 2](https://github.com/rheard/The-Tesseract-on-Xbox-360-for-Modern-Warfare-2) made by CraigChrist (who took those functions from the Dashlaunch source I think). You can tell that he has the assembly in mind when he writes C++ so his code is usually a little tricky to read. I gathered the useful functions for hooking and simplified them a little bit, I won't walk you through them because I don't understand everything they do:
```C++
// Forgotten PPC intrinsic
#define __isync() __emit(0x4C00012C)

void PatchInJump(DWORD *pdwAddress, DWORD dwDestination, bool bLinked)
{
    DWORD dwWriteBuffer;

    if (dwDestination & 0x8000)
        dwWriteBuffer = 0x3D600000 + (((dwDestination >> 16) & 0xFFFF) + 1);
    else
        dwWriteBuffer = 0x3D600000 + ((dwDestination >> 16) & 0xFFFF);

    pdwAddress[0] = dwWriteBuffer;
    dwWriteBuffer = 0x396B0000 + (dwDestination & 0xFFFF);
    pdwAddress[1] = dwWriteBuffer;
    dwWriteBuffer = 0x7D6903A6;
    pdwAddress[2] = dwWriteBuffer;

    if (bLinked)
        dwWriteBuffer = 0x4E800421;
    else
        dwWriteBuffer = 0x4E800420;

    pdwAddress[3] = dwWriteBuffer;

    __dcbst(0, pdwAddress);
    __sync();
    __isync();
}

void __declspec(naked) GLPR()
{
    __asm
    {
        std     r14, -0x98(sp)
        std     r15, -0x90(sp)
        std     r16, -0x88(sp)
        std     r17, -0x80(sp)
        std     r18, -0x78(sp)
        std     r19, -0x70(sp)
        std     r20, -0x68(sp)
        std     r21, -0x60(sp)
        std     r22, -0x58(sp)
        std     r23, -0x50(sp)
        std     r24, -0x48(sp)
        std     r25, -0x40(sp)
        std     r26, -0x38(sp)
        std     r27, -0x30(sp)
        std     r28, -0x28(sp)
        std     r29, -0x20(sp)
        std     r30, -0x18(sp)
        std     r31, -0x10(sp)
        stw     r12, -0x8(sp)
        blr
    }
}

DWORD RelinkGPLR(int nOffset, DWORD *pdwSaveStubAddr, DWORD *pdwOrgAddr)
{
    DWORD dwInst = 0, dwRepl;
    DWORD *pdwSaver = reinterpret_cast<DWORD *>(GLPR);

    if (nOffset & 0x2000000)
        nOffset = nOffset | 0xFC000000;

    dwRepl = pdwOrgAddr[nOffset / 4];

    for (int i = 0; i < 20; i++)
    {
        if (dwRepl == pdwSaver[i])
        {
            int nNewOffset = reinterpret_cast<int>(&pdwSaver[i]) - reinterpret_cast<int>(pdwSaveStubAddr);
            dwInst = 0x48000001 | (nNewOffset & 0x3FFFFFC);
        }
    }

    return dwInst;
}

void HookFunctionStart(DWORD *pdwAddress, DWORD *pdwSaveStub, DWORD dwDestination)
{
    if (pdwSaveStub != NULL && pdwAddress != NULL)
    {
        DWORD dwAddrReloc = reinterpret_cast<DWORD>(&pdwAddress[4]);
        DWORD dwWriteBuffer;

        if (dwAddrReloc & 0x8000)
            dwWriteBuffer = 0x3D600000 + (((dwAddrReloc >> 16) & 0xFFFF) + 1);
        else
            dwWriteBuffer = 0x3D600000 + ((dwAddrReloc >> 16) & 0xFFFF);

        pdwSaveStub[0] = dwWriteBuffer;
        dwWriteBuffer = 0x396B0000 + (dwAddrReloc & 0xFFFF);
        pdwSaveStub[1] = dwWriteBuffer;
        dwWriteBuffer = 0x7D6903A6;
        pdwSaveStub[2] = dwWriteBuffer;
    
        for (int i = 0; i < 4; i++)
        {
            if ((pdwAddress[i] & 0x48000003) == 0x48000001)
            {
                dwWriteBuffer = RelinkGPLR((pdwAddress[i] &~ 0x48000003), &pdwSaveStub[i + 3], &pdwAddress[i]);
                pdwSaveStub[i + 3] = dwWriteBuffer;
            }
            else
            {
                dwWriteBuffer = pdwAddress[i];
                pdwSaveStub[i + 3] = dwWriteBuffer;
            }
        }

        dwWriteBuffer = 0x4E800420; // bctr
        pdwSaveStub[7] = dwWriteBuffer;

        __dcbst(0, pdwSaveStub);
        __sync();
        __isync();

        PatchInJump(pdwAddress, dwDestination, false);
    }
}
```

## Writing a hook
Every time we want to hook a function we need to create an empty function with the same signature that will hold the instructions of the original one. That way we can execute the code that was originally in the function so that the game still functions properly. This empty function is called a stub, it needs to be declared with `__declspec(naked)` to make sure we control every instruction that's in our function and that the compiler doesn't add any by itself. Thus, the empty function needs to be written in inlined assembly.
We can write a hook like so:
```C++
void __declspec(naked) GameFunctionStub(int param1, int param2)
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

void GameFunctionHook(int param1, int param2)
{
    // We call the initial function to keep the original behavior intact
    GameFunctionStub(param1, param2);

    DbgPrint("GameFunction hooked!\n");
}

void Init()
{
    const DWORD dwGameFunctionAddr = 0x82345678;

    HookFunctionStart(reinterpret_cast<DWORD *>(dwGameFunctionAddr), reinterpret_cast<DWORD *>(GameFunctionStub), reinterpret_cast<DWORD>(GameFunctionHook));
}
```
We have successfully hooked a function! Now, every time `GameFunction` gets called it will execute its original code then print "GameFunction hooked!" in the console.

## Real world example
MW2 has a function called `SV_ExecuteClientCommand` that monitors button inputs. Hooking it is going to allow us to know when the user presses or releases a button. We are just going to print the command passed to the function in the killfeed, it's up to you to imagine what you can do with it!

**Note**: Commands passed to `SV_ExecuteClientCommand` are not always explicit strings. For button events, the string has the following format: `n <number>`. There are two events per button, a button pressed event and a button released, the event numbers for one button are consecutive. For example, the dpad left pressed event is `n 19` and the dpad left released event is `n 20`. A list of macros giving a meaningful name to a few buttons is available in the [source code of my personal DLL](https://github.com/ClementDreptin/Hayzen/blob/master/Hayzen/src/Games/MW2/Events.h).

First, we need to create our stub and hook functions:
```C++
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
}
```
At the moment, we only call the original function in the hook so nothing new is happening. We are going to use the `SV_GameSendServerCommand` function that we found in the previous section to print the command passed to `SV_ExecuteClientCommand` in the killfeed.
```C++
void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

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

    const DWORD SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart(reinterpret_cast<DWORD *>(SV_ExecuteClientCommandAddr), reinterpret_cast<DWORD *>(SV_ExecuteClientCommandStub), reinterpret_cast<DWORD>(SV_ExecuteClientCommandHook));
}
```
The last thing to do is calling this function when MW2 is launched (in the switch statement of `MonitorTitleId`):
```C++
switch (newTitle)
{
    case MW2_TITLE_ID:
        InitMW2();
        break;
}
```

You can now get on MW2 with your DLL loaded, press buttons while being in a game and you should see the commands in the killfeed.

<br/>

**Note:** The code example shown in this page is available [here](hooking-functions.cpp).

<br/><br/>

&rarr; [Next: Rendering HUD](../RenderingHUD/rendering-hud.md)

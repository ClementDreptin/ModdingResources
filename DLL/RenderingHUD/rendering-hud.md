# Rendering HUD
Getting feedback about what is going on in the killfeed is cool but kind of limited. In this section we are going to see 2 different ways of creating HUD elements.

## What are HUD elements?

>*In video gaming, the HUD (heads-up display) or status bar is the method by which information is visually relayed to the player as part of a game's user interface. It takes its name from the head-up displays used in modern aircraft.*

Source: [Wikipedia](https://en.wikipedia.org/wiki/HUD_(video_gaming))

On games like Call of Duty, the amount of ammo left in the weapon's magazine, the experience bar, the compass or the subtitles when you play the campaign mode are all part of the HUD. It's every piece of information displayed on the screen that's useful to the player without being part of the environment.

## HUD in Call of Duty
Call of Duty games have an HUD API (which is exposed to the GSC VM but we won't get into that), this API contains functions like `HudElem_Alloc` or `HudElem_Free` and structs such as `game_hudelem_s` or `hudelem_s`. This API allows you to easily create HUD elements without having to manually render them in an update loop. Another way to create HUD elements is to use the lower level rendering API from the engine that provides us with functions like `R_AddCmdDrawStretchPic` or `R_AddCmdDrawText`.

We are going to continue to use the code example shown in the previous section with the hook of the `SV_ExecuteClientCommand` function, and render HUD element when we press left on the DPAD. Which means our hook need to look like this:
```C++
VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19")
    {
        // do stuff
    }
}
```

### HUD API (high level)
Before creating any HUD element we need to create a few function pointers and structs.
```C++
enum he_type_t : INT
{
    HE_TYPE_FREE,
    HE_TYPE_TEXT,
    HE_TYPE_VALUE,
    HE_TYPE_PLAYERNAME,
    HE_TYPE_MATERIAL,
    HE_TYPE_MAPNAME,
    HE_TYPE_GAMETYPE,
    HE_TYPE_TIMER_DOWN,
    HE_TYPE_TIMER_UP,
    HE_TYPE_TIMER_STATIC,
    HE_TYPE_TENTHS_TIMER_DOWN,
    HE_TYPE_TENTHS_TIMER_UP,
    HE_TYPE_CLOCK_DOWN,
    HE_TYPE_CLOCK_UP,
    HE_TYPE_WAYPOINT,
    HE_TYPE_COUNT,
};

typedef enum align_t : INT
{
    ALIGN_TOP_LEFT = 0,
    ALIGN_MIDDLE_LEFT = 1,
    ALIGN_BOTTOM_LEFT = 2,
    ALIGN_TOP_MIDDLE = 4,
    ALIGN_MIDDLE_MIDDLE = 5,
    ALIGN_BOTTOM_MIDDLE = 6,
    ALIGN_TOP_RIGHT = 8,
    ALIGN_MIDDLE_RIGHT = 9,
    ALIGN_BOTTOM_RIGHT = 10
};

struct hudelem_color_t
{
    BYTE r;
    BYTE g;
    BYTE b;
    BYTE a;
};

struct hudelem_s
{
    he_type_t type;
    FLOAT y;
    FLOAT x;
    FLOAT z;
    INT targetEntNum;
    FLOAT fontScale;
    FLOAT fromFontScale;
    INT fontScaleStartTime;
    INT fontScaleTime;
    INT label;
    INT font;
    align_t alignOrg;
    align_t alignScreen;
    hudelem_color_t color;
    hudelem_color_t fromColor;
    INT fadeStartTime;
    INT fadeTime;
    INT height;
    INT width;
    INT materialIndex;
    INT fromHeight;
    INT fromWidth;
    INT scaleStartTime;
    INT scaleTime;
    FLOAT fromY;
    FLOAT fromX;
    INT fromAlignOrg;
    INT fromAlignScreen;
    INT moveStartTime;
    INT moveTime;
    FLOAT value;
    INT time;
    INT duration;
    INT text;
    FLOAT sort;
    hudelem_color_t glowColor;
    INT fxBirthTime;
    INT fxLetterTime;
    INT fxDecayStartTime;
    INT fxDecayDuration;
    INT soundID;
    INT flags;
};

struct game_hudelem_s
{
    hudelem_s elem;
    INT clientNum;
    INT teamNum;
    INT archived;
};

game_hudelem_s* (*HudElem_Alloc)(INT clientNum, INT teamNum) = (game_hudelem_s*(*)(INT, INT))0x821DF928;

INT (*G_MaterialIndex)(LPCSTR name) = (INT(*)(LPCSTR))0x8220C960;

INT (*G_LocalizedStringIndex)(LPCSTR string) = (INT(*)(LPCSTR))0x8220C7A0;
```
The structs are pretty self explanatory so I won't walk you through them. The functions are a little less intuitive. `HudElem_Alloc` simply creates a new `game_hudelem_s` and returns a pointer to it. Texts are generally not stored in the objects themselves but somewhere else and referenced by an index, `G_MaterialIndex` and `G_LocalizedStringIndex` register a material and a string respectively and return their index to reference them in our objects.

Now that we have everything we need, we can start creating our HUD elements! To do so, we just need to create a new element by calling `HudElem_Alloc` and filling the `hudelem_s`. We'll only create the element the first time we press the button then toggle its visibility by modifying the alpha channel of its color.
```C++
game_hudelem_s* rectangleElem = nullptr;

VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19")
    {
        // Creating the rectangle only the first time we press the button
        if (!rectangleElem)
        {
            rectangleElem = HudElem_Alloc(0, 0);
            rectangleElem->elem.x = 441.0f;
            rectangleElem->elem.y = 5.0f;
            rectangleElem->elem.width = 300;
            rectangleElem->elem.height = 470;
            rectangleElem->elem.color.r = 0;
            rectangleElem->elem.color.g = 0;
            rectangleElem->elem.color.b = 0;
            rectangleElem->elem.color.a = 0;
            rectangleElem->elem.type = HE_TYPE_MATERIAL;
            rectangleElem->elem.alignOrg = ALIGN_TOP_LEFT;
            rectangleElem->elem.alignScreen = ALIGN_TOP_LEFT;
            rectangleElem->elem.sort = 0.0f;
            rectangleElem->elem.materialIndex = G_MaterialIndex("white");
        }

        // Toggle the visibility of the rectangle
        if (!rectangleElem->elem.color.a)
            rectangleElem->elem.color.a = 255;
        else
            rectangleElem->elem.color.a = 0;
    }
}
```
We now have a black rectangle appearing on the screen when we press left on the DPAD!

In the [code example](rendering-hud.cpp), I also included a text element, the concept is the same as the rectangle so I'm not going to go through it.

### Rendering API (low-level)
Before creating HUD elements using the lower-level API, we'll need to create a few function pointers and structs and even hook a new function.
```C++
struct Color
{
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
};

VOID (*R_AddCmdDrawStretchPic)(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT s0, FLOAT t0, FLOAT s1, FLOAT t1, CONST PFLOAT color, LPVOID material) =
    (VOID(*)(FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, CONST PFLOAT, LPVOID))0x823BAC18;

LPVOID (*Material_RegisterHandle)(LPCSTR name, INT imageTrack) = (LPVOID(*)(LPCSTR, INT))0x823B6928;

__declspec(naked) VOID SCR_DrawScreenFieldStub(CONST INT localClientNum, INT refreshedUI)
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

VOID SCR_DrawScreenFieldHook(CONST INT localClientNum, INT refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    SCR_DrawScreenFieldStub(localClientNum, refreshedUI);
}
```
Since we are using a lower-level API, we need to draw our elements manually in an update loop, the game already has a drawing function called `SCR_DrawScreenField` so we are going to use it (by hooking it). Since we are hooking a new function, don't forget to add these two lines to your `InitMW2` function.
```C++
CONST DWORD SCR_DrawScreenFieldAddr = 0x8214BEB8;
HookFunctionStart((LPDWORD)SCR_DrawScreenFieldAddr, (LPDWORD)SCR_DrawScreenFieldStub, (DWORD)SCR_DrawScreenFieldHook);
```

To render HUD elements we first need to register a material with `Material_RegisterHandle`, we can then pass the returned pointer to `R_AddCmdDrawStretchPic` to render a rectangle. We'll set up the same toggling system as before in `SV_ExecuteClientCommand` by changing the alpha cchannel of the color used.
```C++
LPVOID materialHandle = nullptr;
Color black = { 0.0f, 0.0f, 0.0f, 1.0f };

VOID SCR_DrawScreenFieldHook(CONST INT localClientNum, INT refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    SCR_DrawScreenFieldStub(localClientNum, refreshedUI);

    // Register the white material the first time we draw
    if (!materialHandle)
        materialHandle = Material_RegisterHandle("white", 0);

    // Rendering the rectangle only if the alpha channel is positive
    if (black.a > 0.0f)
        R_AddCmdDrawStretchPic(5.0f, 5.0f, 400.0f, 710.0f, 0.0f, 0.0f, 1.0f, 1.0f, (PFLOAT)&black, materialHandle);
}

VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19")
    {
        // Toggle the visibility of the rectangle
        if (!black.a)
            black.a = 1.0f;
        else
            white.a = 0.0f;
    }
}
```
We now have a black rectangle appearing on the screen when we press left on the DPAD!

In the [code example](rendering-hud.cpp), I also included, once again, a text element. We use `R_AddCmdDrawText` instead of `R_AddCmdDrawStretchPic` and need to register a font with `R_RegisterFont` but the algorithm is more or less the same, you shouldn't be lost.

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
void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19"))
    {
        // do stuff
    }
}
```

### HUD API (high level)
Before creating any HUD element we need to create a few function pointers and structs.
```C++
enum he_type_t : int
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

typedef enum align_t : int
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
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct hudelem_s
{
    he_type_t type;
    float y;
    float x;
    float z;
    int targetEntNum;
    float fontScale;
    float fromFontScale;
    int fontScaleStartTime;
    int fontScaleTime;
    int label;
    int font;
    align_t alignOrg;
    align_t alignScreen;
    hudelem_color_t color;
    hudelem_color_t fromColor;
    int fadeStartTime;
    int fadeTime;
    int height;
    int width;
    int materialIndex;
    int fromHeight;
    int fromWidth;
    int scaleStartTime;
    int scaleTime;
    float fromY;
    float fromX;
    int fromAlignOrg;
    int fromAlignScreen;
    int moveStartTime;
    int moveTime;
    float value;
    int time;
    int duration;
    int text;
    float sort;
    hudelem_color_t glowColor;
    int fxBirthTime;
    int fxLetterTime;
    int fxDecayStartTime;
    int fxDecayDuration;
    int soundID;
    int flags;
};

struct game_hudelem_s
{
    hudelem_s elem;
    int clientNum;
    int teamNum;
    int archived;
};

game_hudelem_s *(*HudElem_Alloc)(int clientNum, int teamNum) = reinterpret_cast<game_hudelem_s *(*)(int, int)>(0x821DF928);

int (*G_MaterialIndex)(const char *name) = reinterpret_cast<int(*)(const char *)>(0x8220C960);

int (*G_LocalizedStringIndex)(const char *string) = reinterpret_cast<int(*)(const char *)>(0x8220C7A0);
```
The structs are pretty self explanatory so I won't walk you through them. The functions are a little less intuitive. `HudElem_Alloc` simply creates a new `game_hudelem_s` and returns a pointer to it. Texts are generally not stored in the objects themselves but somewhere else and referenced by an index, `G_MaterialIndex` and `G_LocalizedStringIndex` register a material and a string respectively and return their index to reference them in our objects.

Now that we have everything we need, we can start creating our HUD elements! To do so, we just need to create a new element by calling `HudElem_Alloc` and filling the `hudelem_s`. We'll only create the element the first time we press the button then toggle its visibility by modifying the alpha channel of its color.
```C++
game_hudelem_s *rectangleElem = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19"))
    {
        // Creating the rectangle only the first time we press the button
        if (!pRectangleElem)
        {
            pRectangleElem = HudElem_Alloc(0, 0);
            pRectangleElem->elem.x = 441.0f;
            pRectangleElem->elem.y = 5.0f;
            pRectangleElem->elem.width = 300;
            pRectangleElem->elem.height = 470;
            pRectangleElem->elem.color.r = 0;
            pRectangleElem->elem.color.g = 0;
            pRectangleElem->elem.color.b = 0;
            pRectangleElem->elem.color.a = 0;
            pRectangleElem->elem.type = HE_TYPE_MATERIAL;
            pRectangleElem->elem.alignOrg = ALIGN_TOP_LEFT;
            pRectangleElem->elem.alignScreen = ALIGN_TOP_LEFT;
            pRectangleElem->elem.sort = 0.0f;
            pRectangleElem->elem.materialIndex = G_MaterialIndex("white");
        }

        // Toggle the visibility of the rectangle
        if (!pRectangleElem->elem.color.a)
            pRectangleElem->elem.color.a = 255;
        else
            pRectangleElem->elem.color.a = 0;
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
    float r;
    float g;
    float b;
    float a;
};

void (*R_AddCmdDrawStretchPic)(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const float *color, HANDLE material) =
    reinterpret_cast<void(*)(float, float, float, float, float, float, float, float, const float *, HANDLE)>(0x8234F9B8);

HANDLE (*Material_RegisterHandle)(const char *name, int imageTrack) = reinterpret_cast<HANDLE(*)(const char *, int)>(0x8234E510);

void __declspec(naked) SCR_DrawScreenFieldStub(const int localClientNum, int refreshedUI)
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

void SCR_DrawScreenFieldHook(const int localClientNum, int refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    SCR_DrawScreenFieldStub(localClientNum, refreshedUI);
}
```
Since we are using a lower-level API, we need to draw our elements manually in an update loop, the game already has a drawing function called `SCR_DrawScreenField` so we are going to use it (by hooking it). Since we are hooking a new function, don't forget to add these two lines to your `InitMW2` function.
```C++
const DWORD SCR_DrawScreenFieldAddr = 0x8214BEB8;
HookFunctionStart(reinterpret_cast<DWORD *>(SCR_DrawScreenFieldAddr), reinterpret_cast<DWORD *>(SCR_DrawScreenFieldStub), reinterpret_cast<DWORD>(SCR_DrawScreenFieldHook));
```

To render HUD elements we first need to register a material with `Material_RegisterHandle`, we can then pass the returned pointer to `R_AddCmdDrawStretchPic` to render a rectangle. We'll set up the same toggling system as before in `SV_ExecuteClientCommand` by changing the alpha cchannel of the color used.
```C++
HANDLE hMaterial = nullptr;
Color black = { 0.0f, 0.0f, 0.0f, 0.0f };

void SCR_DrawScreenFieldHook(const int localClientNum, int refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    SCR_DrawScreenFieldStub(localClientNum, refreshedUI);

    // Register the white material the first time we draw
    if (!hMaterial)
        hMaterial = Material_RegisterHandle("white", 0);

    // Rendering the rectangle only if the alpha channel is positive
    if (black.a > 0.0f)
        R_AddCmdDrawStretchPic(5.0f, 5.0f, 400.0f, 710.0f, 0.0f, 0.0f, 1.0f, 1.0f, (PFLOAT)&black, hMaterial);
}

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
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

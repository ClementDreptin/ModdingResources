#include "pch.h"

#include "..\Utils\Utils.h"


// HUD API structs and enums
enum he_type_t
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

typedef enum align_t
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

// HUD API functions
game_hudelem_s* (*HudElem_Alloc)(INT clientNum, INT teamNum) = (game_hudelem_s*(*)(INT, INT))0x821DF928;

INT (*G_MaterialIndex)(LPCSTR name) = (INT(*)(LPCSTR))0x8220C960;

INT (*G_LocalizedStringIndex)(LPCSTR string) = (INT(*)(LPCSTR))0x8220C7A0;


// Rendering API structs
struct Color
{
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
};

struct Font_s
{
    INT fontName;
    INT pixelHeight;
    INT glyphCount;
    INT material;
    INT glowMaterial;
    INT glyphs;
};

// Rendering API functions
VOID (*R_AddCmdDrawStretchPic)(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT s0, FLOAT t0, FLOAT s1, FLOAT t1, CONST PFLOAT color, LPVOID material) =
    (VOID(*)(FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, CONST PFLOAT, LPVOID))0x8234F9B8;

VOID (*R_AddCmdDrawText)(LPCSTR text, INT maxChars, Font_s* font, FLOAT x, FLOAT y, FLOAT xScale, FLOAT yScale, FLOAT rotation, CONST PFLOAT color, INT style) =
    (VOID(*)(LPCSTR , INT, Font_s* , FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, CONST PFLOAT, INT))0x82350278;

Font_s* (*R_RegisterFont)(LPCSTR font, INT imageTrack) = (Font_s*(*)(LPCSTR, INT))0x8234DCB0;

HANDLE (*Material_RegisterHandle)(LPCSTR name, INT imageTrack) = (LPVOID(*)(LPCSTR, INT))0x8234E510;

// Used to change some dvars
VOID (*SV_GameSendServerCommand)(INT clientNum, INT type, LPCSTR text) = (VOID(*)(INT, INT, LPCSTR))0x822548D8;

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

HANDLE hMaterial = nullptr;
Font_s* pFont = nullptr;
Color black = { 0.0f, 0.0f, 0.0f, 0.0f };
Color white = { 1.0f, 1.0f, 1.0f, 0.0f };

VOID SCR_DrawScreenFieldHook(CONST INT localClientNum, INT refreshedUI)
{
    // Calling the original SCR_DrawScreenField function
    SCR_DrawScreenFieldStub(localClientNum, refreshedUI);

    // Register the white material the first time we draw
    if (!hMaterial)
        hMaterial = Material_RegisterHandle("white", 0);

    // Register the normal font the first time we draw
    if (!pFont)
        pFont = R_RegisterFont("fonts/normalFont", 0);

    // Rendering the rectangle only if the alpha channel is positive
    if (black.a > 0.0f)
        R_AddCmdDrawStretchPic(5.0f, 5.0f, 400.0f, 710.0f, 0.0f, 0.0f, 1.0f, 1.0f, (PFLOAT)&black, hMaterial);

    // Rendering the text only if the alpha channel is positive
    if (white.a > 0.0f)
    {
        const char* szText = "Rendering API Text";
        R_AddCmdDrawText(szText, strlen(szText), pFont, 90.0f, 375.0f, 1.0f, 1.0f, 0.0f, (PFLOAT)&white, 0);
    }
}

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
        li r3, 1
    }
}

game_hudelem_s* pRectangleElem = nullptr;
game_hudelem_s* pTextElem = nullptr;

VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19"))
    {
        // Creating the rectangle and removing localization warnings only the first time we press the button
        if (!pRectangleElem)
        {
            // Removing UNLOCALIZED() around texts
            SV_GameSendServerCommand(0, 0, "s loc_warnings \"0\"");
            SV_GameSendServerCommand(0, 0, "s loc_warningsUI \"0\"");

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

        // Creating the text only the first time we press the button
        if (!pTextElem)
        {
            pTextElem = HudElem_Alloc(0, 0);
            pTextElem->elem.x = 441.0f + (300.0f / 2.0f) + (5.0f / 2.0f);
            pTextElem->elem.y = 5.0f + (470.0f / 2.0f);
            pTextElem->elem.color.r = 255;
            pTextElem->elem.color.g = 255;
            pTextElem->elem.color.b = 255;
            pTextElem->elem.color.a = 0;
            pTextElem->elem.type = HE_TYPE_TEXT;
            pTextElem->elem.alignOrg = ALIGN_MIDDLE_MIDDLE;
            pTextElem->elem.alignScreen = ALIGN_TOP_LEFT;
            pTextElem->elem.font = 4;
            pTextElem->elem.sort = 1.0f;
            pTextElem->elem.fontScale = 2.0f;
            pTextElem->elem.text = G_LocalizedStringIndex("HUD API Text");
        }

        // Toggle the visibility of the rectangle and the text from the HUD API
        if (!pRectangleElem->elem.color.a)
        {
            pRectangleElem->elem.color.a = 255;
            pTextElem->elem.color.a = 255;
        }
        else
        {
            pRectangleElem->elem.color.a = 0;
            pTextElem->elem.color.a = 0;
        }

        // Toggle the visibility of the rectangle and the text from the rendering API
        if (!black.a)
        {
            black.a = 1.0f;
            white.a = 1.0f;
        }
        else
        {
            black.a = 0.0f;
            white.a = 0.0f;
        }
    }
}

// Sets up the hook
VOID InitMW2()
{
    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);

    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    CONST DWORD dwSV_ExecuteClientCommandAddr = 0x82253140;
    CONST DWORD dwSCR_DrawScreenFieldAddr = 0x8214BEB8;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart((LPDWORD)dwSV_ExecuteClientCommandAddr, (LPDWORD)SV_ExecuteClientCommandStub, (DWORD)SV_ExecuteClientCommandHook);
    HookFunctionStart((LPDWORD)dwSCR_DrawScreenFieldAddr, (LPDWORD)SCR_DrawScreenFieldStub, (DWORD)SCR_DrawScreenFieldHook);
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
            g_bRunning = FALSE;
            // We give the system some time to clean up the thread before exiting
            Sleep(250);
            break;
    }
    return TRUE;
}

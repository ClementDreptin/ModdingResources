#include "pch.h"

#include "..\Utils\Utils.h"


// HUD API structs and enums
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

// HUD API functions
game_hudelem_s *(*HudElem_Alloc)(int clientNum, int teamNum) = reinterpret_cast<game_hudelem_s *(*)(int, int)>(0x821DF928);

int (*G_MaterialIndex)(const char *name) = reinterpret_cast<int(*)(const char *)>(0x8220C960);

int (*G_LocalizedStringIndex)(const char *string) = reinterpret_cast<int(*)(const char *)>(0x8220C7A0);

// Rendering API structs
struct Color
{
    float r;
    float g;
    float b;
    float a;
};

struct Font_s
{
    int fontName;
    int pixelHeight;
    int glyphCount;
    int material;
    int glowMaterial;
    int glyphs;
};

// Rendering API functions
void (*R_AddCmdDrawStretchPic)(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const float *color, HANDLE material) =
    reinterpret_cast<void(*)(float, float, float, float, float, float, float, float, const float *, HANDLE)>(0x8234F9B8);

void (*R_AddCmdDrawText)(const char *text, int maxChars, Font_s *font, float x, float y, float xScale, float yScale, float rotation, const float *color, int style) =
    reinterpret_cast<void(*)(const char *, int, Font_s *, float, float, float, float, float, const float *, int)>(0x82350278);

Font_s *(*R_RegisterFont)(const char *font, int imageTrack) = reinterpret_cast<Font_s *(*)(const char *, int)>(0x8234DCB0);

HANDLE (*Material_RegisterHandle)(const char *name, int imageTrack) = reinterpret_cast<HANDLE(*)(const char *, int)>(0x8234E510);

// Used to change some dvars
void (*SV_GameSendServerCommand)(int clientNum, int type, const char *text) = reinterpret_cast<void(*)(int, int, const char *)>(0x822548D8);

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

HANDLE hMaterial = nullptr;
Font_s *normalFont = nullptr;
Color black = { 0.0f, 0.0f, 0.0f, 0.0f };
Color white = { 1.0f, 1.0f, 1.0f, 0.0f };

void SCR_DrawScreenFieldHook(const int localClientNum, int refreshedUI)
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
        R_AddCmdDrawStretchPic(5.0f, 5.0f, 400.0f, 710.0f, 0.0f, 0.0f, 1.0f, 1.0f, reinterpret_cast<float *>(&black), hMaterial);

    // Rendering the text only if the alpha channel is positive
    if (white.a > 0.0f)
    {
        const char *text = "Rendering API Text";
        R_AddCmdDrawText(text, strlen(text), normalFont, 90.0f, 375.0f, 1.0f, 1.0f, 0.0f, reinterpret_cast<float *>(&white), 0);
    }
}

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
        li r3, 1
    }
}

game_hudelem_s *rectangleElem = nullptr;
game_hudelem_s *textElem = nullptr;

void SV_ExecuteClientCommandHook(int client, const char *s, int clientOK, int fromOldServer)
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
void InitMW2()
{
    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"MW2", nullptr);

    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    const DWORD SV_ExecuteClientCommandAddr = 0x82253140;
    const DWORD SCR_DrawScreenFieldAddr = 0x8214BEB8;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart(reinterpret_cast<DWORD *>(SV_ExecuteClientCommandAddr), reinterpret_cast<DWORD *>(SV_ExecuteClientCommandStub), reinterpret_cast<DWORD>(SV_ExecuteClientCommandHook));
    HookFunctionStart(reinterpret_cast<DWORD *>(SCR_DrawScreenFieldAddr), reinterpret_cast<DWORD *>(SCR_DrawScreenFieldStub), reinterpret_cast<DWORD>(SCR_DrawScreenFieldHook));
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, void *pReserved)
{
    switch (fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
            // Runs MonitorTitleId in separate thread
            ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
            break;
        case DLL_PROCESS_DETACH:
            g_bRunning = FALSE;
            // We give the system some time to clean up the thread before exiting
            Sleep(250);
            break;
    }

    return TRUE;
}

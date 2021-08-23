#include "pch.h"

// Forgotten PPC intrinsic
#define __isync() __emit(0x4C00012C)

CONST DWORD MW2_TITLE_ID = 0x41560817;

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

VOID PatchInJump(LPDWORD address, DWORD destination, BOOL linked)
{
    DWORD writeBuffer;

    if (destination & 0x8000)
        writeBuffer = 0x3D600000 + (((destination >> 16) & 0xFFFF) + 1);
    else
        writeBuffer = 0x3D600000 + ((destination >> 16) & 0xFFFF);

    address[0] = writeBuffer;
    writeBuffer = 0x396B0000 + (destination & 0xFFFF);
    address[1] = writeBuffer;
    writeBuffer = 0x7D6903A6;
    address[2] = writeBuffer;

    if (linked)
        writeBuffer = 0x4E800421;
    else
        writeBuffer = 0x4E800420;

    address[3] = writeBuffer;

    __dcbst(0, address);
    __sync();
    __isync();
}

VOID __declspec(naked) GLPR()
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

DWORD RelinkGPLR(INT offset, LPDWORD saveStubAddr, LPDWORD orgAddr)
{
    DWORD inst = 0, repl;
    INT i;
    LPDWORD saver = (LPDWORD)GLPR;

    if (offset & 0x2000000)
        offset = offset | 0xFC000000;

    repl = orgAddr[offset / 4];

    for (i = 0; i < 20; i++)
    {
        if (repl == saver[i])
        {
            INT newOffset = (INT)&saver[i] - (INT)saveStubAddr;
            inst = 0x48000001 | (newOffset & 0x3FFFFFC);
        }
    }

    return inst;
}

VOID HookFunctionStart(LPDWORD address, LPDWORD saveStub, DWORD destination)
{
    if (saveStub != NULL && address != NULL)
    {
        INT i;
        DWORD addrReloc = (DWORD)(&address[4]);
        DWORD writeBuffer;

        if (addrReloc & 0x8000)
            writeBuffer = 0x3D600000 + (((addrReloc >> 16) & 0xFFFF) + 1);
        else
            writeBuffer = 0x3D600000 + ((addrReloc >> 16) & 0xFFFF);

        saveStub[0] = writeBuffer;
        writeBuffer = 0x396B0000 + (addrReloc & 0xFFFF);
        saveStub[1] = writeBuffer;
        writeBuffer = 0x7D6903A6;
        saveStub[2] = writeBuffer;
    
        for (i = 0; i < 4; i++)
        {
            if ((address[i] & 0x48000003) == 0x48000001)
            {
                writeBuffer = RelinkGPLR((address[i] &~ 0x48000003), &saveStub[i + 3], &address[i]);
                saveStub[i + 3] = writeBuffer;
            }
            else
            {
                writeBuffer = address[i];
                saveStub[i + 3] = writeBuffer;
            }
        }

        writeBuffer = 0x4E800420;
        saveStub[7] = writeBuffer;

        __dcbst(0, saveStub);
        __sync();
        __isync();

        PatchInJump(address, destination, FALSE);
    }
}

// HUD API structs and enums
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

// HUD API functions
game_hudelem_s* (*HudElem_Alloc)(INT clientNum, INT teamNum) = (game_hudelem_s*(*)(INT, INT))0x821DF928;

INT (*G_MaterialIndex)(LPCSTR name) = (INT(*)(LPCSTR))0x8220C960;


// Rendering API struct
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
    (VOID(*)(FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, CONST PFLOAT, LPVOID))0x823BAC18;

Font_s* (*R_RegisterFont)(LPCSTR font, INT imageTrack) = (Font_s*(*)(LPCSTR, INT))0x823B6D58;

LPVOID (*Material_RegisterHandle)(LPCSTR name, INT imageTrack) = (LPVOID(*)(LPCSTR, INT))0x823B6928;


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
        nop
    }
}

game_hudelem_s* hudElem = nullptr;

VOID SV_ExecuteClientCommandHook(INT client, LPCSTR s, INT clientOK, INT fromOldServer)
{
    // Calling the original SV_ExecuteClientCommand function
    SV_ExecuteClientCommandStub(client, s, clientOK, fromOldServer);

    // Checking if dpad left is pressed
    if (!strcmp(s, "n 19")
    {
        // Creating the HudElem only the first time we press the button
        if (!hudElem)
        {
            hudElem = HudElem_Alloc(0, 0);
            hudElem->elem.x = 441.0f;
            hudElem->elem.y = 5.0f;
            hudElem->elem.width = 300;
            hudElem->elem.height = 470;
            hudElem->elem.color.r = 0;
            hudElem->elem.color.g = 0;
            hudElem->elem.color.b = 0;
            hudElem->elem.color.a = 0;
            hudElem->elem.type = HE_TYPE_MATERIAL;
            hudElem->elem.alignOrg = ALIGN_TOP_LEFT;
            hudElem->elem.alignScreen = ALIGN_TOP_LEFT;
            hudElem->elem.sort = 0.0f;
            hudElem->elem.materialIndex = G_MaterialIndex("white");
        }

        // Toggle HudElem's visibility
        if (!hudElem->elem.color.a)
            hudElem->elem.color.a = 255;
        else
            hudElem->elem.color.a = 0;
    }
}

// Sets up the hook
VOID InitMW2()
{
    // Waiting a little bit for the game to be fully loaded in memory
    Sleep(200);

    CONST DWORD SV_ExecuteClientCommandAddr = 0x82253140;

    // Hooking SV_ExecuteClientCommand
    HookFunctionStart((LPDWORD)SV_ExecuteClientCommandAddr, (LPDWORD)SV_ExecuteClientCommandStub, (DWORD)SV_ExecuteClientCommandHook);
}

DWORD MonitorTitleId(LPVOID lpThreadParameter)
{
    DWORD currentTitle;

    while (true)
    {
        DWORD newTitle = XamGetCurrentTitleId();

        if (newTitle != currentTitle)
        {
            currentTitle = newTitle;

            switch (newTitle)
            {
                case MW2_TITLE_ID:
                    InitMW2();
                    break;
            }
        }
    }

    return 0;
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
            break;
    }
    return TRUE;
}

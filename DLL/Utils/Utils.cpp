#include "pch.h"
#include "Utils.h"


DWORD ResolveFunction(LPCSTR szModuleName, DWORD dwOrdinal)
{
    HMODULE hModule = GetModuleHandle(szModuleName);

    return (hModule == NULL) ? NULL : (DWORD)GetProcAddress(hModule, (LPCSTR)dwOrdinal);
}

XNOTIFYQUEUEUI XNotifyQueueUI = (XNOTIFYQUEUEUI)ResolveFunction("xam.xex", 656);

VOID InitMW2();

DWORD MonitorTitleId(LPVOID lpThreadParameter)
{
    DWORD dwCurrentTitle;

    while (g_bRunning)
    {
        DWORD dwNewTitle = XamGetCurrentTitleId();

        if (dwNewTitle != dwCurrentTitle)
        {
            dwCurrentTitle = dwNewTitle;

            switch (dwNewTitle)
            {
                case GAME_DASHBOARD:
                    XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"Dashboard", nullptr);
                    break;
                case GAME_MW2:
                    // Making sure we initialize MW2 only when the multiplayer XEX is running
                    if (!strcmp((LPSTR)0x82001270, "multiplayer"))
                        InitMW2();
                    break;
            }
        }
    }

    return 0;
}

// Forgotten PPC intrinsic
#define __isync() __emit(0x4C00012C)

VOID PatchInJump(LPDWORD lpdwAddress, DWORD dwDestination, BOOL bLinked)
{
    DWORD dwWriteBuffer;

    if (dwDestination & 0x8000)
        dwWriteBuffer = 0x3D600000 + (((dwDestination >> 16) & 0xFFFF) + 1);
    else
        dwWriteBuffer = 0x3D600000 + ((dwDestination >> 16) & 0xFFFF);

    lpdwAddress[0] = dwWriteBuffer;
    dwWriteBuffer = 0x396B0000 + (dwDestination & 0xFFFF);
    lpdwAddress[1] = dwWriteBuffer;
    dwWriteBuffer = 0x7D6903A6;
    lpdwAddress[2] = dwWriteBuffer;

    if (bLinked)
        dwWriteBuffer = 0x4E800421;
    else
        dwWriteBuffer = 0x4E800420;

    lpdwAddress[3] = dwWriteBuffer;

    __dcbst(0, lpdwAddress);
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

DWORD RelinkGPLR(INT nOffset, LPDWORD lpdwSaveStubAddr, LPDWORD lpdwOrgAddr)
{
    DWORD dwInst = 0, dwRepl;
    LPDWORD lpdwSaver = (LPDWORD)GLPR;

    if (nOffset & 0x2000000)
        nOffset = nOffset | 0xFC000000;

    dwRepl = lpdwOrgAddr[nOffset / 4];

    for (INT i = 0; i < 20; i++)
    {
        if (dwRepl == lpdwSaver[i])
        {
            INT newOffset = (INT)&lpdwSaver[i] - (INT)lpdwSaveStubAddr;
            dwInst = 0x48000001 | (newOffset & 0x3FFFFFC);
        }
    }

    return dwInst;
}

VOID HookFunctionStart(LPDWORD lpdwAddress, LPDWORD lpdwSaveStub, DWORD dwDestination)
{
    if (lpdwSaveStub != NULL && lpdwAddress != NULL)
    {
        DWORD dwAddrReloc = (DWORD)(&lpdwAddress[4]);
        DWORD dwWriteBuffer;

        if (dwAddrReloc & 0x8000)
            dwWriteBuffer = 0x3D600000 + (((dwAddrReloc >> 16) & 0xFFFF) + 1);
        else
            dwWriteBuffer = 0x3D600000 + ((dwAddrReloc >> 16) & 0xFFFF);

        lpdwSaveStub[0] = dwWriteBuffer;
        dwWriteBuffer = 0x396B0000 + (dwAddrReloc & 0xFFFF);
        lpdwSaveStub[1] = dwWriteBuffer;
        dwWriteBuffer = 0x7D6903A6;
        lpdwSaveStub[2] = dwWriteBuffer;
    
        for (INT i = 0; i < 4; i++)
        {
            if ((lpdwAddress[i] & 0x48000003) == 0x48000001)
            {
                dwWriteBuffer = RelinkGPLR((lpdwAddress[i] &~ 0x48000003), &lpdwSaveStub[i + 3], &lpdwAddress[i]);
                lpdwSaveStub[i + 3] = dwWriteBuffer;
            }
            else
            {
                dwWriteBuffer = lpdwAddress[i];
                lpdwSaveStub[i + 3] = dwWriteBuffer;
            }
        }

        dwWriteBuffer = 0x4E800420;
        lpdwSaveStub[7] = dwWriteBuffer;

        __dcbst(0, lpdwSaveStub);
        __sync();
        __isync();

        PatchInJump(lpdwAddress, dwDestination, FALSE);
    }
}

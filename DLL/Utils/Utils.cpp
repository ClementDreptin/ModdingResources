#include "pch.h"
#include "Utils.h"


DWORD ResolveFunction(const std::string &strModuleName, DWORD dwOrdinal)
{
    HMODULE hModule = GetModuleHandle(strModuleName.c_str());

    return (hModule == NULL) ? NULL : reinterpret_cast<DWORD>(GetProcAddress(hModule, reinterpret_cast<const char *>(dwOrdinal)));
}

XNOTIFYQUEUEUI XNotifyQueueUI = reinterpret_cast<XNOTIFYQUEUEUI>(ResolveFunction("xam.xex", 656));

bool g_bRunning = true;

void InitMW2();

DWORD MonitorTitleId(void *pThreadParameter)
{
    DWORD dwCurrentTitle = 0;

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
                    if (!strcmp(reinterpret_cast<const char *>(0x82001270), "multiplayer"))
                        InitMW2();
                    break;
            }
        }
    }

    return 0;
}

#define MASK_N_BITS(N) ((1 << (N)) - 1)

#define POWERPC_HI(X) ((X >> 16) & 0xFFFF)
#define POWERPC_LO(X) (X & 0xFFFF)

// PowerPC most significant bit is addressed as bit 0 in documentation.
#define POWERPC_BIT32(N) (31 - N)

// Opcode is bits 0-5.
// Allowing for op codes ranging from 0-63.
#define POWERPC_OPCODE(OP)       (OP << 26)
#define POWERPC_OPCODE_ADDI      POWERPC_OPCODE(14)
#define POWERPC_OPCODE_ADDIS     POWERPC_OPCODE(15)
#define POWERPC_OPCODE_BC        POWERPC_OPCODE(16)
#define POWERPC_OPCODE_B         POWERPC_OPCODE(18)
#define POWERPC_OPCODE_BCCTR     POWERPC_OPCODE(19)
#define POWERPC_OPCODE_ORI       POWERPC_OPCODE(24)
#define POWERPC_OPCODE_EXTENDED  POWERPC_OPCODE(31) // Use extended opcodes.
#define POWERPC_OPCODE_STW       POWERPC_OPCODE(36)
#define POWERPC_OPCODE_LWZ       POWERPC_OPCODE(32)
#define POWERPC_OPCODE_LD        POWERPC_OPCODE(58)
#define POWERPC_OPCODE_STD       POWERPC_OPCODE(62)
#define POWERPC_OPCODE_MASK      POWERPC_OPCODE(63)

#define POWERPC_EXOPCODE(OP)     (OP << 1)
#define POWERPC_EXOPCODE_BCCTR   POWERPC_EXOPCODE(528)
#define POWERPC_EXOPCODE_MTSPR   POWERPC_EXOPCODE(467)

// SPR field is encoded as two 5 bit bitfields.
#define POWERPC_SPR(SPR) static_cast<DWORD>(((SPR & 0x1F) << 5) | ((SPR >> 5) & 0x1F))

// Instruction helpers.
//
// rD - Destination register.
// rS - Source register.
// rA/rB - Register inputs.
// SPR - Special purpose register.
// UIMM/SIMM - Unsigned/signed immediate.
#define POWERPC_ADDI(rD, rA, SIMM)  static_cast<DWORD>(POWERPC_OPCODE_ADDI | (rD << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | SIMM)
#define POWERPC_ADDIS(rD, rA, SIMM) static_cast<DWORD>(POWERPC_OPCODE_ADDIS | (rD << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | SIMM)
#define POWERPC_LIS(rD, SIMM)       POWERPC_ADDIS(rD, 0, SIMM) // Mnemonic for addis %rD, 0, SIMM
#define POWERPC_LI(rD, SIMM)        POWERPC_ADDIrD, 0, SIMM)   // Mnemonic for addi %rD, 0, SIMM
#define POWERPC_MTSPR(SPR, rS)      static_cast<DWORD>(POWERPC_OPCODE_EXTENDED | (rS << POWERPC_BIT32(10)) | (POWERPC_SPR(SPR) << POWERPC_BIT32(20)) | POWERPC_EXOPCODE_MTSPR)
#define POWERPC_MTCTR(rS)           POWERPC_MTSPR(9, rS) // Mnemonic for mtspr 9, rS
#define POWERPC_ORI(rS, rA, UIMM)   static_cast<DWORD>(POWERPC_OPCODE_ORI | (rS << POWERPC_BIT32(10)) |  (rA << POWERPC_BIT32(15)) | UIMM)
#define POWERPC_BCCTR(BO, BI, LK)   static_cast<DWORD>(POWERPC_OPCODE_BCCTR | (BO << POWERPC_BIT32(10)) | (BI << POWERPC_BIT32(15) | LK & 1) | POWERPC_EXOPCODE_BCCTR)
#define POWERPC_STD(rS, DS, rA)     static_cast<DWORD>(POWERPC_OPCODE_STD | (rS << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | (static_cast<WORD>(DS) & 0xFFFF))
#define POWERPC_LD(rS, DS, rA)      static_cast<DWORD>(POWERPC_OPCODE_LD | (rS << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | (static_cast<DWORD>(DS) & 0xFFFF))

// Branch related fields.
#define POWERPC_BRANCH_LINKED    1
#define POWERPC_BRANCH_ABSOLUTE  2
#define POWERPC_BRANCH_TYPE_MASK (POWERPC_BRANCH_LINKED | POWERPC_BRANCH_ABSOLUTE)

byte Detour::s_pTrampolineBuffer[TRAMPOLINE_BUFFER_MAX_SIZE] = { 0 };
size_t Detour::s_uiTrampolineSize = 0;

Detour::Detour(DWORD dwHookSourceAddress, const void *pHookTarget)
{
    Init(reinterpret_cast<void *>(dwHookSourceAddress), pHookTarget);
}

Detour::Detour(void *pHookSource, const void *pHookTarget)
{
    Init(pHookSource, pHookTarget);
}

void Detour::Init(void *pHookSource, const void *pHookTarget)
{
    m_pHookSource = pHookSource;
    m_pHookTarget = pHookTarget;
    m_pTrampolineDestination = nullptr;
    m_uiOriginalLength = 0;

    Install();
}

Detour::~Detour()
{
    Remove();
}

bool Detour::Install()
{
    // Check if we are already hooked
    if (m_uiOriginalLength != 0)
        return false;

    const size_t uiHookSize = WriteFarBranch(nullptr, m_pHookTarget);

    // Save the original instructions for unhooking later on
    memcpy(m_pbOriginalInstructions, m_pHookSource, uiHookSize);

    m_uiOriginalLength = uiHookSize;

    // Create trampoline and copy and fix instructions to the trampoline
    m_pTrampolineDestination = &s_pTrampolineBuffer[s_uiTrampolineSize];

    for (size_t i = 0; i < (uiHookSize / 4); i++)
    {
        const void *pInstruction = reinterpret_cast<void *>(reinterpret_cast<DWORD>(m_pHookSource) + (i * 4));

        s_uiTrampolineSize += CopyInstruction(reinterpret_cast<void *>(&s_pTrampolineBuffer[s_uiTrampolineSize]), pInstruction);
    }

    // Trampoline branches back to the original function after the branch we used to hook
    const void *pAfterBranchAddress = reinterpret_cast<void *>(reinterpret_cast<DWORD>(m_pHookSource) + uiHookSize);

    s_uiTrampolineSize += WriteFarBranch(&s_pTrampolineBuffer[s_uiTrampolineSize], pAfterBranchAddress, false, true);

    // Finally write the branch to the function that we are hooking
    WriteFarBranch(m_pHookSource, m_pHookTarget);

    return true;
}

bool Detour::Remove()
{
    if (m_pHookSource && m_uiOriginalLength)
    {
        // Trying to remove a hook from a game function after the game has been closed could cause a segfault so we
        // make sure the hook function is still loaded in memory
        bool bIsHookSourceAddressValid = MmIsAddressValid(m_pHookSource);
        if (bIsHookSourceAddressValid)
            memcpy(m_pHookSource, m_pbOriginalInstructions, m_uiOriginalLength);

        m_uiOriginalLength = 0;
        m_pHookSource = nullptr;

        return true;
    }

    return false;
}

size_t Detour::WriteFarBranch(void *pDestination, const void *pBranchTarget, bool bLinked, bool bPreserveRegister, DWORD dwBranchOptions, byte bConditionRegisterBit, byte bRegisterIndex)
{
    const DWORD pdwBranchFarAsm[] =
    {
        POWERPC_LIS(bRegisterIndex, POWERPC_HI(reinterpret_cast<DWORD>(pBranchTarget))),                   // lis   %rX, BranchTarget@hi
        POWERPC_ORI(bRegisterIndex, bRegisterIndex, POWERPC_LO(reinterpret_cast<DWORD>(pBranchTarget))),   // ori   %rX, %rX, BranchTarget@lo
        POWERPC_MTCTR(bRegisterIndex),                                                                     // mtctr %rX
        POWERPC_BCCTR(dwBranchOptions, bConditionRegisterBit, bLinked)                                     // bcctr (bcctr 20, 0 == bctr)
    };

    const DWORD pdwBranchFarAsmPreserve[] =
    {
        POWERPC_STD(bRegisterIndex, -0x30, 1),                                                             // std   %rX, -0x30(%r1)
        POWERPC_LIS(bRegisterIndex, POWERPC_HI(reinterpret_cast<DWORD>(pBranchTarget))),                   // lis   %rX, BranchTarget@hi
        POWERPC_ORI(bRegisterIndex, bRegisterIndex, POWERPC_LO(reinterpret_cast<DWORD>(pBranchTarget))),   // ori   %rX, %rX, BranchTarget@lo
        POWERPC_MTCTR(bRegisterIndex),                                                                     // mtctr %rX
        POWERPC_LD(bRegisterIndex, -0x30, 1),                                                              // lwz   %rX, -0x30(%r1)
        POWERPC_BCCTR(dwBranchOptions, bConditionRegisterBit, bLinked)                                     // bcctr (bcctr 20, 0 == bctr)
    };

    const DWORD *pdwBranchAsm = bPreserveRegister ? pdwBranchFarAsmPreserve : pdwBranchFarAsm;
    const DWORD dwBranchAsmSize = bPreserveRegister ? sizeof(pdwBranchFarAsmPreserve) : sizeof(pdwBranchFarAsm);

    if (pDestination)
        memcpy(pDestination, pdwBranchAsm, dwBranchAsmSize);

    return dwBranchAsmSize;
}

size_t Detour::RelocateBranch(void *pDestination, const void *pSource)
{
    const DWORD dwInstruction = *reinterpret_cast<const DWORD *>(pSource);
    const DWORD dwInstructionAddress = reinterpret_cast<DWORD>(pSource);

    // Absolute branches don't need to be handled
    if (dwInstruction & POWERPC_BRANCH_ABSOLUTE)
    {
        *reinterpret_cast<DWORD *>(pDestination) = dwInstruction;

        return 4;
    }

    size_t uiBranchOffsetBitSize = 0;
    int iBranchOffsetBitBase = 0;
    DWORD dwBranchOptions = 0;
    byte bConditionRegisterBit = 0;

    switch (dwInstruction & POWERPC_OPCODE_MASK)
    {
        // B - Branch
        // [Opcode]            [Address]           [Absolute] [Linked]
        //   0-5                 6-29                  30        31
        //
        // Example
        //  010010   0000 0000 0000 0000 0000 0001      0         0
    case POWERPC_OPCODE_B:
        uiBranchOffsetBitSize = 24;
        iBranchOffsetBitBase = 2;
        dwBranchOptions = POWERPC_BRANCH_OPTIONS_ALWAYS;
        bConditionRegisterBit = 0;
        break;

        // BC - Branch Conditional
        // [Opcode]   [Branch Options]     [Condition Register]         [Address]      [Absolute] [Linked]
        //   0-5           6-10                    11-15                  16-29            30        31
        //
        // Example
        //  010000        00100                    00001             00 0000 0000 0001      0         0
    case POWERPC_OPCODE_BC:
        uiBranchOffsetBitSize = 14;
        iBranchOffsetBitBase = 2;
        dwBranchOptions = (dwInstruction >> POWERPC_BIT32(10)) & MASK_N_BITS(5);
        bConditionRegisterBit = (dwInstruction >> POWERPC_BIT32(15)) & MASK_N_BITS(5);
        break;
    }

    // Even though the address part of the instruction begins from bit 29 in the case of bc and b.
    // The value of the first bit is 4 as all addresses are aligned to for 4 for code therefore,
    // the branch offset can be caluclated by anding in place and removing any suffix bits such as the 
    // link register or absolute flags.
    DWORD dwBranchOffset = dwInstruction & (MASK_N_BITS(uiBranchOffsetBitSize) << iBranchOffsetBitBase);

    // Check if the MSB of the offset is set
    if (dwBranchOffset >> ((uiBranchOffsetBitSize + iBranchOffsetBitBase) - 1))
    {
        // Add the nessasary bits to our integer to make it negative
        dwBranchOffset |= ~MASK_N_BITS(uiBranchOffsetBitSize + iBranchOffsetBitBase);
    }

    const void *pBranchAddress = reinterpret_cast<void *>(dwInstructionAddress + dwBranchOffset);

    return WriteFarBranch(pDestination, pBranchAddress, dwInstruction & POWERPC_BRANCH_LINKED, true, dwBranchOptions, bConditionRegisterBit);
}

size_t Detour::CopyInstruction(void *pDestination, const void *pSource)
{
    const DWORD dwInstruction = *reinterpret_cast<const DWORD *>(pSource);

    switch (dwInstruction & POWERPC_OPCODE_MASK)
    {
    case POWERPC_OPCODE_B:  // B BL BA BLA
    case POWERPC_OPCODE_BC: // BEQ BNE BLT BGE
        return RelocateBranch(pDestination, pSource);
    default:
        *reinterpret_cast<DWORD *>(pDestination) = dwInstruction;
        return 4;
    }
}

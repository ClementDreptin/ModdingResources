#include "pch.h"
#include "Utils.h"

void *ResolveFunction(const std::string &moduleName, uint32_t ordinal)
{
    HMODULE hModule = GetModuleHandle(moduleName.c_str());

    return (hModule == NULL) ? NULL : GetProcAddress(hModule, reinterpret_cast<const char *>(ordinal));
}

XNOTIFYQUEUEUI XNotifyQueueUI = static_cast<XNOTIFYQUEUEUI>(ResolveFunction("xam.xex", 656));

bool g_Running = true;

void InitMW2();

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
#define POWERPC_OPCODE(OP) (OP << 26)
#define POWERPC_OPCODE_ADDI POWERPC_OPCODE(14)
#define POWERPC_OPCODE_ADDIS POWERPC_OPCODE(15)
#define POWERPC_OPCODE_BC POWERPC_OPCODE(16)
#define POWERPC_OPCODE_B POWERPC_OPCODE(18)
#define POWERPC_OPCODE_BCCTR POWERPC_OPCODE(19)
#define POWERPC_OPCODE_ORI POWERPC_OPCODE(24)
#define POWERPC_OPCODE_EXTENDED POWERPC_OPCODE(31) // Use extended opcodes.
#define POWERPC_OPCODE_STW POWERPC_OPCODE(36)
#define POWERPC_OPCODE_LWZ POWERPC_OPCODE(32)
#define POWERPC_OPCODE_LD POWERPC_OPCODE(58)
#define POWERPC_OPCODE_STD POWERPC_OPCODE(62)
#define POWERPC_OPCODE_MASK POWERPC_OPCODE(63)

#define POWERPC_EXOPCODE(OP) (OP << 1)
#define POWERPC_EXOPCODE_BCCTR POWERPC_EXOPCODE(528)
#define POWERPC_EXOPCODE_MTSPR POWERPC_EXOPCODE(467)

// SPR field is encoded as two 5 bit bitfields.
#define POWERPC_SPR(SPR) static_cast<POWERPC_INSTRUCTION>(((SPR & 0x1F) << 5) | ((SPR >> 5) & 0x1F))

// Instruction helpers.
//
// rD - Destination register.
// rS - Source register.
// rA/rB - Register inputs.
// SPR - Special purpose register.
// UIMM/SIMM - Unsigned/signed immediate.
#define POWERPC_ADDI(rD, rA, SIMM) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_ADDI | (rD << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | SIMM)
#define POWERPC_ADDIS(rD, rA, SIMM) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_ADDIS | (rD << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | SIMM)
#define POWERPC_LIS(rD, SIMM) POWERPC_ADDIS(rD, 0, SIMM) // Mnemonic for addis %rD, 0, SIMM
#define POWERPC_LI(rD, SIMM) POWERPC_ADDI(rD, 0, SIMM)   // Mnemonic for addi %rD, 0, SIMM
#define POWERPC_MTSPR(SPR, rS) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_EXTENDED | (rS << POWERPC_BIT32(10)) | (POWERPC_SPR(SPR) << POWERPC_BIT32(20)) | POWERPC_EXOPCODE_MTSPR)
#define POWERPC_MTCTR(rS) POWERPC_MTSPR(9, rS) // Mnemonic for mtspr 9, rS
#define POWERPC_ORI(rS, rA, UIMM) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_ORI | (rS << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | UIMM)
#define POWERPC_BCCTR(BO, BI, LK) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_BCCTR | (BO << POWERPC_BIT32(10)) | (BI << POWERPC_BIT32(15) | LK & 1) | POWERPC_EXOPCODE_BCCTR)
#define POWERPC_STD(rS, DS, rA) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_STD | (rS << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | (static_cast<uint16_t>(DS) & 0xFFFF))
#define POWERPC_LD(rS, DS, rA) static_cast<POWERPC_INSTRUCTION>(POWERPC_OPCODE_LD | (rS << POWERPC_BIT32(10)) | (rA << POWERPC_BIT32(15)) | (static_cast<uint16_t>(DS) & 0xFFFF))

// Branch related fields.
#define POWERPC_BRANCH_LINKED 1
#define POWERPC_BRANCH_ABSOLUTE 2
#define POWERPC_BRANCH_TYPE_MASK (POWERPC_BRANCH_LINKED | POWERPC_BRANCH_ABSOLUTE)

uint8_t Detour::s_TrampolineBuffer[TRAMPOLINE_BUFFER_MAX_SIZE] = { 0 };
size_t Detour::s_TrampolineSize = 0;

Detour::Detour(uintptr_t hookSourceAddress, const void *pHookTarget)
{
    Init(reinterpret_cast<void *>(hookSourceAddress), pHookTarget);
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
    m_OriginalLength = 0;

    Install();
}

Detour::~Detour()
{
    Remove();
}

bool Detour::Install()
{
    // Check if we are already hooked
    if (m_OriginalLength != 0)
        return false;

    const size_t hookSize = WriteFarBranch(nullptr, m_pHookTarget);

    // Save the original instructions for unhooking later on
    memcpy(m_OriginalInstructions, m_pHookSource, hookSize);

    m_OriginalLength = hookSize;

    // Create trampoline and copy and fix instructions to the trampoline
    m_pTrampolineDestination = &s_TrampolineBuffer[s_TrampolineSize];

    for (size_t i = 0; i < (hookSize / sizeof(POWERPC_INSTRUCTION)); i++)
    {
        const void *pInstruction = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_pHookSource) + (i * sizeof(POWERPC_INSTRUCTION)));

        s_TrampolineSize += CopyInstruction(reinterpret_cast<void *>(&s_TrampolineBuffer[s_TrampolineSize]), pInstruction);
    }

    // Trampoline branches back to the original function after the branch we used to hook
    const void *pAfterBranchAddress = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_pHookSource) + hookSize);

    s_TrampolineSize += WriteFarBranch(&s_TrampolineBuffer[s_TrampolineSize], pAfterBranchAddress, false, true);

    // Finally write the branch to the function that we are hooking
    WriteFarBranch(m_pHookSource, m_pHookTarget);

    return true;
}

bool Detour::Remove()
{
    if (m_pHookSource && m_OriginalLength)
    {
        // Trying to remove a hook from a game function after the game has been closed could cause a segfault so we
        // make sure the hook function is still loaded in memory
        bool isHookSourceAddressValid = MmIsAddressValid(m_pHookSource);
        if (isHookSourceAddressValid)
            memcpy(m_pHookSource, m_OriginalInstructions, m_OriginalLength);

        m_OriginalLength = 0;
        m_pHookSource = nullptr;

        return true;
    }

    return false;
}

size_t Detour::WriteFarBranch(void *pDestination, const void *pBranchTarget, bool linked, bool preserveRegister, POWERPC_INSTRUCTION branchOptions, uint8_t conditionRegisterBit, uint8_t registerIndex)
{
    const POWERPC_INSTRUCTION branchFarAsm[] = {
        POWERPC_LIS(registerIndex, POWERPC_HI(reinterpret_cast<uintptr_t>(pBranchTarget))),                // lis   %rX, pBranchTarget@hi
        POWERPC_ORI(registerIndex, registerIndex, POWERPC_LO(reinterpret_cast<uintptr_t>(pBranchTarget))), // ori   %rX, %rX, pBranchTarget@lo
        POWERPC_MTCTR(registerIndex),                                                                      // mtctr %rX
        POWERPC_BCCTR(branchOptions, conditionRegisterBit, linked)                                         // bcctr (bcctr 20, 0 == bctr)
    };

    const POWERPC_INSTRUCTION branchFarAsmPreserve[] = {
        POWERPC_STD(registerIndex, -0x30, 1),                                                              // std   %rX, -0x30(%r1)
        POWERPC_LIS(registerIndex, POWERPC_HI(reinterpret_cast<uintptr_t>(pBranchTarget))),                // lis   %rX, pBranchTarget@hi
        POWERPC_ORI(registerIndex, registerIndex, POWERPC_LO(reinterpret_cast<uintptr_t>(pBranchTarget))), // ori   %rX, %rX, pBranchTarget@lo
        POWERPC_MTCTR(registerIndex),                                                                      // mtctr %rX
        POWERPC_LD(registerIndex, -0x30, 1),                                                               // lwz   %rX, -0x30(%r1)
        POWERPC_BCCTR(branchOptions, conditionRegisterBit, linked)                                         // bcctr (bcctr 20, 0 == bctr)
    };

    const POWERPC_INSTRUCTION *pBranchAsm = preserveRegister ? branchFarAsmPreserve : branchFarAsm;
    const size_t branchAsmSize = preserveRegister ? sizeof(branchFarAsmPreserve) : sizeof(branchFarAsm);

    if (pDestination)
        memcpy(pDestination, pBranchAsm, branchAsmSize);

    return branchAsmSize;
}

size_t Detour::RelocateBranch(void *pDestination, const void *pSource)
{
    const POWERPC_INSTRUCTION instruction = *reinterpret_cast<const POWERPC_INSTRUCTION *>(pSource);
    const uintptr_t instructionAddress = reinterpret_cast<uintptr_t>(pSource);

    // Absolute branches don't need to be handled
    if (instruction & POWERPC_BRANCH_ABSOLUTE)
    {
        *reinterpret_cast<POWERPC_INSTRUCTION *>(pDestination) = instruction;

        return sizeof(POWERPC_INSTRUCTION);
    }

    size_t branchOffsetBitSize = 0;
    int branchOffsetBitBase = 0;
    uint32_t branchOptions = 0;
    uint8_t conditionRegisterBit = 0;

    switch (instruction & POWERPC_OPCODE_MASK)
    {
        // B - Branch
        // [Opcode]            [Address]           [Absolute] [Linked]
        //   0-5                 6-29                  30        31
        //
        // Example
        //  010010   0000 0000 0000 0000 0000 0001      0         0
    case POWERPC_OPCODE_B:
        branchOffsetBitSize = 24;
        branchOffsetBitBase = 2;
        branchOptions = POWERPC_BRANCH_OPTIONS_ALWAYS;
        conditionRegisterBit = 0;
        break;

        // BC - Branch Conditional
        // [Opcode]   [Branch Options]     [Condition Register]         [Address]      [Absolute] [Linked]
        //   0-5           6-10                    11-15                  16-29            30        31
        //
        // Example
        //  010000        00100                    00001             00 0000 0000 0001      0         0
    case POWERPC_OPCODE_BC:
        branchOffsetBitSize = 14;
        branchOffsetBitBase = 2;
        branchOptions = (instruction >> POWERPC_BIT32(10)) & MASK_N_BITS(5);
        conditionRegisterBit = (instruction >> POWERPC_BIT32(15)) & MASK_N_BITS(5);
        break;
    }

    // Even though the address part of the instruction begins from bit 29 in the case of bc and b.
    // The value of the first bit is 4 as all addresses are aligned to for 4 for code therefore,
    // the branch offset can be caluclated by anding in place and removing any suffix bits such as the
    // link register or absolute flags.
    size_t branchOffset = instruction & (MASK_N_BITS(branchOffsetBitSize) << branchOffsetBitBase);

    // Check if the MSB of the offset is set
    if (branchOffset >> ((branchOffsetBitSize + branchOffsetBitBase) - 1))
    {
        // Add the nessasary bits to our integer to make it negative
        branchOffset |= ~MASK_N_BITS(branchOffsetBitSize + branchOffsetBitBase);
    }

    const void *pBranchAddress = reinterpret_cast<void *>(instructionAddress + branchOffset);

    return WriteFarBranch(pDestination, pBranchAddress, instruction & POWERPC_BRANCH_LINKED, true, branchOptions, conditionRegisterBit);
}

size_t Detour::CopyInstruction(void *pDestination, const void *pSource)
{
    const POWERPC_INSTRUCTION instruction = *reinterpret_cast<const POWERPC_INSTRUCTION *>(pSource);

    switch (instruction & POWERPC_OPCODE_MASK)
    {
    case POWERPC_OPCODE_B:  // B BL BA BLA
    case POWERPC_OPCODE_BC: // BEQ BNE BLT BGE
        return RelocateBranch(pDestination, pSource);
    default:
        *reinterpret_cast<POWERPC_INSTRUCTION *>(pDestination) = instruction;
        return sizeof(POWERPC_INSTRUCTION);
    }
}

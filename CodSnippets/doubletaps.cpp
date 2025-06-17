// Double taps (MW2 mechanics) on MW3 (also works on Ghosts)

typedef enum _WeaponAnimNumber
{
    WEAP_FORCE_IDLE = 1,
    WEAP_SPRINT_LOOP = 24,
    WEAP_SPRINT_OUT = 25,
} WeaponAnimNumber;

typedef enum _weaponstate_t
{
    WEAPON_DROPPING = 3,
    WEAPON_DROPPING_QUICK = 4,
    WEAPON_DROPPING_ALT = 5,
} weaponstate_t;

typedef enum _PMFlags
{
    PMF_LADDER = 1 << 3,
} PMFlags;

typedef enum _WeapFlags
{
    PWF_DISABLE_WEAPONS = 1 << 7,
} WeapFlags;

struct PlayerActiveWeaponState
{
    WeaponAnimNumber weapAnim;
    char padding1[0xC];
    weaponstate_t weaponState;
    char padding2[0x8];
};

struct PlayerWeaponCommonState
{
    char padding1[0xC];
    int weapon;
    WeapFlags weapFlags;
    char padding2[0x164];
};

struct playerState_s
{
    char padding1[0xC];
    PMFlags pm_flags;
    char padding2[0x22C];
    PlayerActiveWeaponState weapState[2];
    char padding3[0xF0];
    PlayerWeaponCommonState weapCommon;
    char padding4[0x2E24];
};

struct usercmd_s
{
    char padding1[0x14];
    int weapon;
    char padding2[0x14];
};

struct pmove_t
{
    playerState_s *ps;
    usercmd_s cmd;
    char padding1[0x108];
};

Detour *PM_Weapon_CheckForChangeWeaponDetour = nullptr;

static void PM_Weapon_CheckForChangeWeaponHook(pmove_t *pm, uint32_t *holdrand)
{
    // Inspired by:
    // https://github.com/CBServers/iw6x-client/blob/main/src/client/component/mechanics.cpp#L39

    weaponstate_t weaponState = pm->ps->weapState[0].weaponState;
    bool wantsToCancelSwap = pm->ps->weapCommon.weapon == pm->cmd.weapon;
    bool canCancelSwap =
        (weaponState == WEAPON_DROPPING || weaponState == WEAPON_DROPPING_QUICK || weaponState == WEAPON_DROPPING_ALT) &&
        pm->ps->weapCommon.weapFlags != PWF_DISABLE_WEAPONS &&
        (pm->ps->pm_flags & PMF_LADDER) == 0;

    if (wantsToCancelSwap && canCancelSwap)
    {
        ZeroMemory(pm->ps->weapState, sizeof(pm->ps->weapState));

        for (size_t i = 0; i < 2; i++)
            if (pm->ps->weapState[i].weapAnim == WEAP_SPRINT_LOOP)
                pm->ps->weapState[i].weapAnim = WEAP_SPRINT_OUT;
            else
                pm->ps->weapState[i].weapAnim = WEAP_FORCE_IDLE;

        return;
    }

    PM_Weapon_CheckForChangeWeaponDetour->GetOriginal<decltype(&PM_Weapon_CheckForChangeWeaponHook)>()(pm, holdrand);
}

void main()
{
    PM_Weapon_CheckForChangeWeaponDetour = new Detour(0x820F0C80, PM_Weapon_CheckForChangeWeaponHook);
}

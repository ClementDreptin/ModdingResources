// Look for the care package brush model in MW2

struct entityState_s
{
    int number;
    char padding1[0xF8];
};

struct gentity_s
{
    entityState_s state;
    char padding1[0x184];
};

struct scr_entref_t
{
    uint16_t entnum;
    uint16_t classnum;
};

// Hook ScriptEntCmd_CloneBrushModelToScriptModel and Scr_GetEntity
Detour *ScriptEntCmd_CloneBrushModelToScriptModelDetour = nullptr;
Detour *Scr_GetEntityDetour = nullptr;
static bool called = false;

// Signal that ScriptEntCmd_CloneBrushModelToScriptModel was just called
void ScriptEntCmd_CloneBrushModelToScriptModelHook(scr_entref_t entref)
{
    if (!called)
        called = true;

    ScriptEntCmd_CloneBrushModelToScriptModelDetour->GetOriginal<decltype(&ScriptEntCmd_CloneBrushModelToScriptModelHook)>()(entref);
}

// Intercept the entity and log it only when ScriptEntCmd_CloneBrushModelToScriptModel was just called
gentity_s *Scr_GetEntityHook(uint32_t index)
{
    gentity_s *res = Scr_GetEntityDetour->GetOriginal<decltype(&Scr_GetEntityHook)>()(index);

    if (called)
    {
        Log::Info("Scr_GetEntity: num: %d, addr: %p", res->state.number, res);
        called = false;
    }

    return res;
}

void main()
{
    ScriptEntCmd_CloneBrushModelToScriptModelDetour = new Detour(0x82208140, ScriptEntCmd_CloneBrushModelToScriptModelHook);
    Scr_GetEntityDetour = new Detour(0x822094F8, Scr_GetEntityHook);
}

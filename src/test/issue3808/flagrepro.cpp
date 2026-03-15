#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "_scriptapi_flag.h"
#include "_scriptapi_misc.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;

    struct FlagSnapshot
    {
        bool regdumpOk = false;
        REGDUMP regdump{};
        bool zfExprOk = false;
        duint zfExpr = 0;
        bool cfExprOk = false;
        duint cfExpr = 0;
        bool zfApi = false;
        bool cfApi = false;
    };

    FlagSnapshot TakeSnapshot()
    {
        FlagSnapshot snapshot;
        snapshot.regdumpOk = DbgGetRegDumpEx(reinterpret_cast<REGDUMP_AVX512*>(&snapshot.regdump), sizeof(snapshot.regdump));
        snapshot.zfExprOk = Script::Misc::ParseExpression("_ZF", &snapshot.zfExpr);
        snapshot.cfExprOk = Script::Misc::ParseExpression("_CF", &snapshot.cfExpr);
        snapshot.zfApi = Script::Flag::GetZF();
        snapshot.cfApi = Script::Flag::GetCF();
        return snapshot;
    }

    void LogSnapshot(const char* tag, const FlagSnapshot & snapshot)
    {
        _plugin_logprintf(
            "[issue3808] %s regdump_ok=%d eflags=0x%llX reg_zf=%d reg_cf=%d expr_zf_ok=%d expr_zf=%llu expr_cf_ok=%d expr_cf=%llu api_zf=%d api_cf=%d\n",
            tag,
            snapshot.regdumpOk ? 1 : 0,
            snapshot.regdumpOk ? static_cast<unsigned long long>(snapshot.regdump.regcontext.eflags) : 0ULL,
            snapshot.regdumpOk && snapshot.regdump.flags.z ? 1 : 0,
            snapshot.regdumpOk && snapshot.regdump.flags.c ? 1 : 0,
            snapshot.zfExprOk ? 1 : 0,
            static_cast<unsigned long long>(snapshot.zfExpr),
            snapshot.cfExprOk ? 1 : 0,
            static_cast<unsigned long long>(snapshot.cfExpr),
            snapshot.zfApi ? 1 : 0,
            snapshot.cfApi ? 1 : 0
        );
    }

    bool IsPrimed(const FlagSnapshot & snapshot)
    {
        return snapshot.regdumpOk
               && snapshot.zfExprOk
               && snapshot.cfExprOk
               && snapshot.regdump.flags.z
               && !snapshot.regdump.flags.c
               && snapshot.zfExpr == 1
               && snapshot.cfExpr == 0
               && snapshot.zfApi
               && !snapshot.cfApi;
    }

    bool IsBrokenAfterSetters(const FlagSnapshot & snapshot)
    {
        return snapshot.regdumpOk
               && snapshot.zfExprOk
               && snapshot.cfExprOk
               && snapshot.regdump.flags.z
               && !snapshot.regdump.flags.c
               && snapshot.zfExpr == 1
               && snapshot.cfExpr == 0
               && snapshot.zfApi
               && !snapshot.cfApi;
    }

    bool cbFlagRepro3808(int, char**)
    {
        _plugin_logputs("[issue3808] Priming ZF=1 CF=0 through the command path");
        if(!DbgCmdExecDirect("_ZF=1") || !DbgCmdExecDirect("_CF=0"))
        {
            _plugin_logputs("[issue3808] ERROR failed to prime flags with command path");
            return false;
        }

        const auto primed = TakeSnapshot();
        LogSnapshot("after_command_path", primed);

        const bool setZfOk = Script::Flag::SetZF(false);
        const auto afterSetZf = TakeSnapshot();
        LogSnapshot("after_SetZF_false", afterSetZf);

        const bool setCfOk = Script::Flag::SetCF(true);
        const auto finalSnapshot = TakeSnapshot();
        LogSnapshot("after_SetCF_true", finalSnapshot);

        const bool primedOk = IsPrimed(primed);
        const bool broken = primedOk && IsBrokenAfterSetters(finalSnapshot);
        const bool exactIssue = broken && setZfOk && setCfOk;

        _plugin_logprintf(
            "[issue3808] RESULT broken=%d exact_issue=%d primed=%d setzf_ok=%d setcf_ok=%d final_reg_zf=%d final_reg_cf=%d final_expr_zf=%llu final_expr_cf=%llu final_api_zf=%d final_api_cf=%d\n",
            broken ? 1 : 0,
            exactIssue ? 1 : 0,
            primedOk ? 1 : 0,
            setZfOk ? 1 : 0,
            setCfOk ? 1 : 0,
            finalSnapshot.regdumpOk && finalSnapshot.regdump.flags.z ? 1 : 0,
            finalSnapshot.regdumpOk && finalSnapshot.regdump.flags.c ? 1 : 0,
            static_cast<unsigned long long>(finalSnapshot.zfExpr),
            static_cast<unsigned long long>(finalSnapshot.cfExpr),
            finalSnapshot.zfApi ? 1 : 0,
            finalSnapshot.cfApi ? 1 : 0
        );
        return true;
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), "FlagRepro3808", _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "flagrepro3808", cbFlagRepro3808, true);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

#pragma once

#include "_global.h"

enum SCRIPTSTATE
{
    SCRIPT_PAUSED = 0,
    SCRIPT_STEPPING,
    SCRIPT_RUNNING,
};

struct SCRIPTABORTSTATE
{
    SCRIPTSTATE state;
    bool gui;
};

bool ScriptLoadAwait(const char* filename, bool gui);
void ScriptUnloadAwait();
void ScriptRunAsync(int destline, bool gui);
bool ScriptRunAwait(int destline, bool gui);
void ScriptStepAsync(bool gui);
bool ScriptBpGetLocked(int line);
bool ScriptBpToggleLocked(int line);
bool ScriptCmdExecAwait(const char* command, bool gui, const SCRIPTSTATE* abortState);
void ScriptSetIpAwait(int line);
SCRIPTABORTSTATE ScriptAbortAwait();
SCRIPTLINETYPE ScriptGetLineTypeLocked(int line);
bool ScriptGetBranchInfoLocked(int line, SCRIPTBRANCH* info);
void ScriptLogLocked(const char* msg);
bool ScriptExecAwait(const char* filename, bool gui);

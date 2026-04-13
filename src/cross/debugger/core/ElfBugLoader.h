#pragma once

#include <cstdint>
#include <dlfcn.h>
#include <sys/types.h>

typedef struct ElfBugDebugger ElfBugDebugger;

typedef struct
{
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rbp, rsp, rsi, rdi;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t eflags;
    uint16_t cs, ds, es, fs, gs, ss;
    uint64_t fs_base, gs_base;
} ElfBugRegisters;

typedef void (*ElfBugCbCreateProcess)(pid_t pid, uint64_t entryPoint, void* userdata);
typedef void (*ElfBugCbExitProcess)(int exitCode, void* userdata);
typedef void (*ElfBugCbSystemBreakpoint)(void* userdata);
typedef void (*ElfBugCbBreakpoint)(uint64_t address, void* userdata);
typedef void (*ElfBugCbStep)(void* userdata);
typedef void (*ElfBugCbPaused)(void* userdata);
typedef void (*ElfBugCbError)(const char* error, void* userdata);
typedef void (*ElfBugCbDebugString)(const char* text, void* userdata);

typedef struct
{
    ElfBugCbCreateProcess onCreateProcess;
    ElfBugCbExitProcess onExitProcess;
    ElfBugCbSystemBreakpoint onSystemBreakpoint;
    ElfBugCbBreakpoint onBreakpoint;
    ElfBugCbStep onStep;
    ElfBugCbPaused onPaused;
    ElfBugCbError onError;
    ElfBugCbDebugString onDebugString;
    void* userdata;
} ElfBugCallbacks;


struct ElfBugApi
{
    ElfBugDebugger* (*Create)(const ElfBugCallbacks*) = nullptr;
    void (*Destroy)(ElfBugDebugger*) = nullptr;
    bool (*Init)(ElfBugDebugger*, const char*) = nullptr;
    void (*Start)(ElfBugDebugger*) = nullptr;
    void (*Continue)(ElfBugDebugger*) = nullptr;
    void (*StepInto)(ElfBugDebugger*) = nullptr;
    void (*Pause)(ElfBugDebugger*) = nullptr;
    bool (*Stop)(ElfBugDebugger*) = nullptr;
    bool (*GetRegisters)(const ElfBugDebugger*, ElfBugRegisters*) = nullptr;
    pid_t (*GetPid)(const ElfBugDebugger*) = nullptr;
    bool (*MemRead)(const ElfBugDebugger*, uint64_t, void*, uint64_t) = nullptr;
    bool (*MemFindBaseAddr)(const ElfBugDebugger*, uint64_t, uint64_t*, uint64_t*) = nullptr;
    bool (*MemIsCodePtr)(const ElfBugDebugger*, uint64_t) = nullptr;
    bool (*MemIsValidPtr)(const ElfBugDebugger*, uint64_t) = nullptr;
    bool (*SetBreakpoint)(ElfBugDebugger*, uint64_t) = nullptr;
    bool (*DeleteBreakpoint)(ElfBugDebugger*, uint64_t) = nullptr;
    bool (*HasBreakpoint)(const ElfBugDebugger*, uint64_t) = nullptr;

    void* handle = nullptr;

    bool load(const char* path)
    {
        handle = dlopen(path, RTLD_NOW);
        if(!handle)
            return false;

        auto sym = [this](const char* name) { return dlsym(handle, name); };

        Create = reinterpret_cast<decltype(Create)>(sym("ElfBugCreate"));
        Destroy = reinterpret_cast<decltype(Destroy)>(sym("ElfBugDestroy"));
        Init = reinterpret_cast<decltype(Init)>(sym("ElfBugInit"));
        Start = reinterpret_cast<decltype(Start)>(sym("ElfBugStart"));
        Continue = reinterpret_cast<decltype(Continue)>(sym("ElfBugContinue"));
        StepInto = reinterpret_cast<decltype(StepInto)>(sym("ElfBugStepInto"));
        Pause = reinterpret_cast<decltype(Pause)>(sym("ElfBugPause"));
        Stop = reinterpret_cast<decltype(Stop)>(sym("ElfBugStop"));
        GetRegisters = reinterpret_cast<decltype(GetRegisters)>(sym("ElfBugGetRegisters"));
        GetPid = reinterpret_cast<decltype(GetPid)>(sym("ElfBugGetPid"));
        MemRead = reinterpret_cast<decltype(MemRead)>(sym("ElfBugMemRead"));
        MemFindBaseAddr = reinterpret_cast<decltype(MemFindBaseAddr)>(sym("ElfBugMemFindBaseAddr"));
        MemIsCodePtr = reinterpret_cast<decltype(MemIsCodePtr)>(sym("ElfBugMemIsCodePtr"));
        MemIsValidPtr = reinterpret_cast<decltype(MemIsValidPtr)>(sym("ElfBugMemIsValidPtr"));
        SetBreakpoint = reinterpret_cast<decltype(SetBreakpoint)>(sym("ElfBugSetBreakpoint"));
        DeleteBreakpoint = reinterpret_cast<decltype(DeleteBreakpoint)>(sym("ElfBugDeleteBreakpoint"));
        HasBreakpoint = reinterpret_cast<decltype(HasBreakpoint)>(sym("ElfBugHasBreakpoint"));

        return Create && Destroy && Init && Start;
    }

    // Appends names of unresolved optional symbols to `out` (null-terminated).
    // Returns the number of missing functions.
    int missingOptional(const char* out[], int maxOut) const
    {
        int n = 0;
        auto check = [&](const void* ptr, const char* name)
        {
            if(!ptr && n < maxOut)
                out[n++] = name;
        };
        check((const void*)Continue, "ElfBugContinue");
        check((const void*)StepInto, "ElfBugStepInto");
        check((const void*)Pause, "ElfBugPause");
        check((const void*)Stop, "ElfBugStop");
        check((const void*)GetRegisters, "ElfBugGetRegisters");
        check((const void*)GetPid, "ElfBugGetPid");
        check((const void*)MemRead, "ElfBugMemRead");
        check((const void*)MemFindBaseAddr, "ElfBugMemFindBaseAddr");
        check((const void*)MemIsCodePtr, "ElfBugMemIsCodePtr");
        check((const void*)MemIsValidPtr, "ElfBugMemIsValidPtr");
        check((const void*)SetBreakpoint, "ElfBugSetBreakpoint");
        check((const void*)DeleteBreakpoint, "ElfBugDeleteBreakpoint");
        check((const void*)HasBreakpoint, "ElfBugHasBreakpoint");
        return n;
    }

    void unload()
    {
        if(handle)
        {
            dlclose(handle);
            *this = ElfBugApi{};
        }
    }

    [[nodiscard]] bool isLoaded() const { return handle != nullptr; }
};

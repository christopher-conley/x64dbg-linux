#include "core/DbgAdapter.h"
#include <cassert>
#include <QCoreApplication>

static REGDUMP toRegDump(const ElfBugRegisters& regs)
{
    REGDUMP dump{};
    dump.regcontext.cax = regs.rax;
    dump.regcontext.cbx = regs.rbx;
    dump.regcontext.ccx = regs.rcx;
    dump.regcontext.cdx = regs.rdx;
    dump.regcontext.cbp = regs.rbp;
    dump.regcontext.csp = regs.rsp;
    dump.regcontext.csi = regs.rsi;
    dump.regcontext.cdi = regs.rdi;
    dump.regcontext.r8  = regs.r8;
    dump.regcontext.r9  = regs.r9;
    dump.regcontext.r10 = regs.r10;
    dump.regcontext.r11 = regs.r11;
    dump.regcontext.r12 = regs.r12;
    dump.regcontext.r13 = regs.r13;
    dump.regcontext.r14 = regs.r14;
    dump.regcontext.r15 = regs.r15;
    dump.regcontext.cip = regs.rip;
    dump.regcontext.eflags = regs.eflags;
    dump.regcontext.cs = regs.cs;
    dump.regcontext.ds = regs.ds;
    dump.regcontext.es = regs.es;
    dump.regcontext.fs = regs.fs;
    dump.regcontext.gs = regs.gs;
    dump.regcontext.ss = regs.ss;
    dump.flags.c = (regs.eflags & 1) != 0;
    dump.flags.p = (regs.eflags & (1 << 2)) != 0;
    dump.flags.a = (regs.eflags & (1 << 4)) != 0;
    dump.flags.z = (regs.eflags & (1 << 6)) != 0;
    dump.flags.s = (regs.eflags & (1 << 7)) != 0;
    dump.flags.t = (regs.eflags & (1 << 8)) != 0;
    dump.flags.i = (regs.eflags & (1 << 9)) != 0;
    dump.flags.d = (regs.eflags & (1 << 10)) != 0;
    dump.flags.o = (regs.eflags & (1 << 11)) != 0;
    return dump;
}

std::atomic<DbgAdapter*> DbgAdapter::sInstance{nullptr};

DbgAdapter::DbgAdapter(QObject* parent)
    : QObject(parent)
{
    assert(!sInstance.load() && "Only one DbgAdapter instance is allowed");
    sInstance.store(this);
    DbgSetBreakpointQuery(&DbgAdapter::queryBreakpoint);
}

DbgAdapter::~DbgAdapter()
{
    DbgSetBreakpointQuery(nullptr);
    sInstance.store(nullptr);
    if(mDebugger && mApi.Destroy)
        mApi.Destroy(mDebugger);
    mApi.unload();
}

bool DbgAdapter::loadEngine()
{
    if(mApi.isLoaded())
        return true;

    const auto enginePath = QCoreApplication::applicationDirPath() + "/engine/libElfBug.so";
    if(!mApi.load(enginePath.toUtf8().constData()))
    {
        emit logMessage(QString("[x64dbg] Failed to load engine: %1").arg(dlerror()));
        return false;
    }

    ElfBugCallbacks cb = {};
    cb.onCreateProcess = &DbgAdapter::onCreateProcess;
    cb.onExitProcess = &DbgAdapter::onExitProcess;
    cb.onSystemBreakpoint = &DbgAdapter::onSystemBreakpoint;
    cb.onBreakpoint = &DbgAdapter::onBreakpoint;
    cb.onStep = &DbgAdapter::onStep;
    cb.onPaused = &DbgAdapter::onPaused;
    cb.onError = &DbgAdapter::onError;
    cb.onDebugString = &DbgAdapter::onDebugString;
    cb.userdata = this;

    mDebugger = mApi.Create(&cb);
    if(!mDebugger)
    {
        emit logMessage("[x64dbg] ElfBugCreate failed");
        mApi.unload();
        return false;
    }

    const char* missing[16];
    int nMissing = mApi.missingOptional(missing, 16);
    if(nMissing > 0)
    {
        QString list;
        for(int i = 0; i < nMissing; i++)
        {
            if(i > 0)
                list += ", ";
            list += missing[i];
        }
        emit logMessage(QString("[x64dbg] Engine loaded (missing optional: %1)").arg(list));
    }
    else
    {
        emit logMessage("[x64dbg] Engine loaded");
    }
    return true;
}

// -- MemoryProvider interface --

bool DbgAdapter::read(const duint addr, void* dest, const duint size)
{
    if(!mApi.MemRead)
        return false;
    return mApi.MemRead(mDebugger, addr, dest, size);
}

bool DbgAdapter::getRange(const duint addr, duint& base, duint& size)
{
    if(!mApi.MemFindBaseAddr)
        return false;
    uint64_t b, s;
    if(!mApi.MemFindBaseAddr(mDebugger, addr, &b, &s))
        return false;
    base = b;
    size = s;
    return true;
}

bool DbgAdapter::isCodePtr(const duint addr)
{
    if(!mApi.MemIsCodePtr)
        return false;
    return mApi.MemIsCodePtr(mDebugger, addr);
}

bool DbgAdapter::isValidPtr(const duint addr)
{
    if(!mApi.MemIsValidPtr)
        return false;
    return mApi.MemIsValidPtr(mDebugger, addr);
}

// -- Debugger control --

bool DbgAdapter::launch(const char* path) const
{
    if(!mApi.Init)
        return false;
    return mApi.Init(mDebugger, path);
}

void DbgAdapter::Start() const
{
    if(mApi.Start)
        mApi.Start(mDebugger);
}

void DbgAdapter::Continue() const
{
    if(mApi.Continue)
        mApi.Continue(mDebugger);
}

void DbgAdapter::StepInto() const
{
    if(mApi.StepInto)
        mApi.StepInto(mDebugger);
}

void DbgAdapter::Pause() const
{
    if(mApi.Pause)
        mApi.Pause(mDebugger);
}

bool DbgAdapter::Stop() const
{
    if(!mApi.Stop)
        return false;
    return mApi.Stop(mDebugger);
}

bool DbgAdapter::isActive() const
{
    if(!mApi.GetPid)
        return false;
    return mApi.GetPid(mDebugger) > 0;
}

bool DbgAdapter::toggleBreakpoint(const duint addr) const
{
    if(!isActive() || !mApi.HasBreakpoint || !mApi.SetBreakpoint || !mApi.DeleteBreakpoint)
        return false;

    if(mApi.HasBreakpoint(mDebugger, addr))
        return mApi.DeleteBreakpoint(mDebugger, addr);
    return mApi.SetBreakpoint(mDebugger, addr);
}

bool DbgAdapter::hasBreakpoint(const duint addr) const
{
    if(!mApi.HasBreakpoint)
        return false;
    return mApi.HasBreakpoint(mDebugger, addr);
}

BPXTYPE DbgAdapter::queryBreakpoint(duint addr)
{
    auto* instance = sInstance.load();
    if(!instance)
        return bp_none;
    return instance->hasBreakpoint(addr) ? bp_normal : bp_none;
}

void DbgAdapter::emitStoppedState(const QString& reason)
{
    ElfBugRegisters regs = {};
    if(mApi.GetRegisters)
        mApi.GetRegisters(mDebugger, &regs);
    auto dump = toRegDump(regs);
    emit registersUpdated(dump);
    emit stopped(dump.regcontext.cip, reason);
}

void DbgAdapter::onCreateProcess(const pid_t pid, const uint64_t entryPoint, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->mEntryPoint = entryPoint;
    emit self->logMessage(QString("[x64dbg] Process created: PID %1").arg(pid));
}

void DbgAdapter::onExitProcess(const int exitCode, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Process exited: %1").arg(exitCode));
    emit self->processExited(exitCode);
}

void DbgAdapter::onSystemBreakpoint(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    ElfBugRegisters regs = {};
    if(self->mApi.GetRegisters)
        self->mApi.GetRegisters(self->mDebugger, &regs);
    self->mEntryPoint = regs.rip;

    emit self->logMessage(QString("[x64dbg] Entry point: 0x%1").arg(self->mEntryPoint, 0, 16));
    emit self->registersUpdated(toRegDump(regs));
    emit self->processCreated(self->mEntryPoint);
    emit self->stopped(self->mEntryPoint, tr("System breakpoint"));
}

void DbgAdapter::onBreakpoint(const uint64_t address, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Breakpoint hit: 0x%1").arg(address, 0, 16));
    self->emitStoppedState(QString("Breakpoint at 0x%1").arg(address, 0, 16));
}

void DbgAdapter::onStep(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->emitStoppedState(tr("Step"));
}

void DbgAdapter::onPaused(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->emitStoppedState(tr("Paused"));
}

void DbgAdapter::onError(const char* error, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Error: %1").arg(error));
}

void DbgAdapter::onDebugString(const char* text, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[dbg] %1").arg(text));
}

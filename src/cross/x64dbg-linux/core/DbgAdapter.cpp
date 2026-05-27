#include "core/DbgAdapter.h"
#include "core/LinuxThreadManager.h"
#include <cassert>
#include <cstring>
#include <chrono>

// Zydis for instruction decoding
#include <Zydis/Zydis.h>

static REGDUMP toRegDump(const ElfBugRegisters & regs)
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
    if (mDebugger)
        ElfBugDestroy(mDebugger);
    // Clear all breakpoints before destroying managers
    if (mHwBpManager)
        mHwBpManager->clearAllBreakpoints();
    if (mMemBpManager)
        mMemBpManager->clearAllBreakpoints();
}

bool DbgAdapter::loadEngine()
{
    if(mDebugger)
        return true;

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

    mDebugger = ElfBugCreate(&cb);
    if(!mDebugger)
    {
        emit logMessage("[x64dbg] ElfBugCreate failed");
        return false;
    }

    // Initialize hardware and memory breakpoint managers
    mHwBpManager = std::make_unique<X64DbgLinux::HardwareBreakpointManager>();
    mMemBpManager = std::make_unique<X64DbgLinux::MemoryBreakpointManager>();

    emit logMessage("[x64dbg] Engine loaded");
    return true;
}

// -- MemoryProvider interface --

bool DbgAdapter::read(const duint addr, void* dest, const duint size)
{
    return ElfBugMemRead(mDebugger, addr, dest, size);
}

bool DbgAdapter::write(const duint addr, const void* src, const duint size)
{
    return ElfBugMemWrite(mDebugger, addr, src, size);
}

bool DbgAdapter::getRange(const duint addr, duint & base, duint & size)
{
    uint64_t b, s;
    if(!ElfBugMemFindBaseAddr(mDebugger, addr, &b, &s))
        return false;
    base = b;
    size = s;
    return true;
}

bool DbgAdapter::isCodePtr(const duint addr)
{
    return ElfBugMemIsCodePtr(mDebugger, addr);
}

bool DbgAdapter::isValidPtr(const duint addr)
{
    return ElfBugMemIsValidPtr(mDebugger, addr);
}

bool DbgAdapter::writeRegister(const char* name, const duint value)
{
    if(!ElfBugSetRegister(mDebugger, name, value))
        return false;

    ElfBugRegisters regs = {};
    if(ElfBugGetRegisters(mDebugger, &regs))
        emit registersUpdated(toRegDump(regs));
    return true;
}

bool DbgAdapter::modBaseFromAddr(const duint addr, duint & base)
{
    uint64_t b = 0;
    if(!ElfBugModBaseFromAddr(mDebugger, addr, &b))
        return false;
    base = b;
    return true;
}

bool DbgAdapter::modNameFromAddr(const duint addr, char* buf, const duint bufSize, const bool extension)
{
    return ElfBugModNameFromAddr(mDebugger, addr, buf, bufSize, extension);
}

// -- Debugger control --

bool DbgAdapter::launch(const char* path) const
{
    return ElfBugInit(mDebugger, path);
}

void DbgAdapter::Start() const
{
    ElfBugStart(mDebugger);
}

void DbgAdapter::Continue() const
{
    ElfBugContinue(mDebugger);
}

void DbgAdapter::StepInto() const
{
    ElfBugStepInto(mDebugger);
}

void DbgAdapter::StepOver()
{
    if(!mDebugger || !isActive())
        return;

    // Get current RIP
    ElfBugRegisters regs = {};
    if(!ElfBugGetRegisters(mDebugger, &regs))
    {
        ElfBugStepInto(mDebugger);
        return;
    }

    const uint64_t rip = regs.rip;

    // Check instruction cache first
    auto cached = getCachedInstruction(rip);
    if(cached && !cached->isCall)
    {
        // Not a call, use step into (no need to decode again)
        ElfBugStepInto(mDebugger);
        return;
    }

    // Read instruction bytes
    uint8_t buffer[16];
    if(!ElfBugMemRead(mDebugger, rip, buffer, sizeof(buffer)))
    {
        ElfBugStepInto(mDebugger);
        return;
    }

    // Decode instruction
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, buffer, sizeof(buffer), &instruction, operands)))
    {
        ElfBugStepInto(mDebugger);
        return;
    }

    // Cache the result
    const bool isCall = (instruction.meta.category == ZYDIS_CATEGORY_CALL) ||
                        (instruction.mnemonic == ZYDIS_MNEMONIC_CALL);
    cacheInstruction(rip, instruction.length, isCall);

    if(isCall)
    {
        const uint64_t nextAddr = rip + instruction.length;
        if(ElfBugSetBreakpoint(mDebugger, nextAddr))
        {
            ElfBugContinue(mDebugger);
        }
        else
        {
            ElfBugStepInto(mDebugger);
        }
    }
    else
    {
        ElfBugStepInto(mDebugger);
    }
}

void DbgAdapter::Pause() const
{
    ElfBugPause(mDebugger);
}

bool DbgAdapter::Stop() const
{
    return ElfBugStop(mDebugger);
}

bool DbgAdapter::isActive() const
{
    return ElfBugGetPid(mDebugger) > 0;
}

pid_t DbgAdapter::getPid() const
{
    if(!mDebugger)
        return 0;
    return ElfBugGetPid(mDebugger);
}

bool DbgAdapter::toggleBreakpoint(const duint addr) const
{
    if(!isActive())
        return false;

    if(ElfBugIsBreakpointEffective(mDebugger, addr))
        return ElfBugDeleteBreakpoint(mDebugger, addr);
    return ElfBugSetBreakpoint(mDebugger, addr);
}

bool DbgAdapter::hasBreakpoint(const duint addr) const
{
    return ElfBugIsBreakpointEffective(mDebugger, addr);
}

// Hardware breakpoint management
bool DbgAdapter::setHardwareBreakpoint(int slot, uint64_t addr,
                                       X64DbgLinux::HardwareBreakpointManager::Type type,
                                       X64DbgLinux::HardwareBreakpointManager::Size size)
{
    if (!mHwBpManager || !isActive())
        return false;
    mHwBpManager->setTarget(getPid());
    return mHwBpManager->setBreakpoint(slot, addr, type, size);
}

bool DbgAdapter::clearHardwareBreakpoint(int slot)
{
    if (!mHwBpManager)
        return false;
    return mHwBpManager->clearBreakpoint(slot);
}

bool DbgAdapter::enableHardwareBreakpoint(int slot)
{
    if (!mHwBpManager)
        return false;
    return mHwBpManager->enableBreakpoint(slot);
}

bool DbgAdapter::disableHardwareBreakpoint(int slot)
{
    if (!mHwBpManager)
        return false;
    return mHwBpManager->disableBreakpoint(slot);
}

std::optional<int> DbgAdapter::findFreeHardwareBreakpointSlot() const
{
    if (!mHwBpManager)
        return std::nullopt;
    return mHwBpManager->findFreeSlot();
}

void DbgAdapter::clearAllHardwareBreakpoints()
{
    if (mHwBpManager)
        mHwBpManager->clearAllBreakpoints();
}

// Memory breakpoint management
bool DbgAdapter::setMemoryBreakpoint(uint64_t addr, size_t size, X64DbgLinux::MemoryBreakpointType type)
{
    if (!mMemBpManager || !isActive())
        return false;
    mMemBpManager->setTarget(getPid());
    return mMemBpManager->setMemoryBreakpoint(addr, size, type);
}

bool DbgAdapter::removeMemoryBreakpoint(uint64_t addr)
{
    if (!mMemBpManager)
        return false;
    return mMemBpManager->removeMemoryBreakpoint(addr);
}

bool DbgAdapter::enableMemoryBreakpoint(uint64_t addr)
{
    if (!mMemBpManager)
        return false;
    return mMemBpManager->enableMemoryBreakpoint(addr);
}

bool DbgAdapter::disableMemoryBreakpoint(uint64_t addr)
{
    if (!mMemBpManager)
        return false;
    return mMemBpManager->disableMemoryBreakpoint(addr);
}

void DbgAdapter::clearAllMemoryBreakpoints()
{
    if (mMemBpManager)
        mMemBpManager->clearAllBreakpoints();
}

void DbgAdapter::setThreadManager(X64DbgLinux::ThreadManager* manager)
{
    mThreadManager = manager;
}

void DbgAdapter::setCurrentThread(pid_t tid)
{
    if (mThreadManager) {
        mThreadManager->setCurrentThread(tid);
    }
}

BPXTYPE DbgAdapter::queryBreakpoint(duint addr)
{
    auto* instance = sInstance.load();
    if(!instance)
        return bp_none;
    return instance->hasBreakpoint(addr) ? bp_normal : bp_none;
}

void DbgAdapter::emitStoppedState(const QString & reason)
{
    ElfBugRegisters regs = {};
    ElfBugGetRegisters(mDebugger, &regs);
    auto dump = toRegDump(regs);
    emit registersUpdated(dump);
    emit stopped(dump.regcontext.cip, reason);
}

void DbgAdapter::onCreateProcess(const pid_t pid, const uint64_t entryPoint, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->mEntryPoint = entryPoint;

    // Set target PID for breakpoint managers
    if (self->mHwBpManager)
        self->mHwBpManager->setTarget(pid);
    if (self->mMemBpManager)
        self->mMemBpManager->setTarget(pid);

    emit self->logMessage(QString("[x64dbg] Process created: PID %1").arg(pid));
}

void DbgAdapter::onExitProcess(const int exitCode, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);

    // Clear breakpoints and target PID
    if (self->mHwBpManager)
        self->mHwBpManager->setTarget(0);
    if (self->mMemBpManager)
        self->mMemBpManager->setTarget(0);

    // Clear instruction cache
    self->clearInstructionCache();

    emit self->logMessage(QString("[x64dbg] Process exited: %1").arg(exitCode));
    emit self->processExited(exitCode);
}

void DbgAdapter::onSystemBreakpoint(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    ElfBugRegisters regs = {};
    ElfBugGetRegisters(self->mDebugger, &regs);
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

// Instruction cache implementation
std::optional<DbgAdapter::CachedInstruction> DbgAdapter::getCachedInstruction(uint64_t addr) const
{
    std::lock_guard<std::mutex> lock(mCacheMutex);

    auto it = mInstructionCache.find(addr);
    if(it == mInstructionCache.end())
        return std::nullopt;

    // Check if cache entry is expired
    auto now = std::chrono::steady_clock::now().time_since_epoch().count() / 1000000; // ms
    if(now - it->second.timestamp > CACHE_TTL_MS)
    {
        mInstructionCache.erase(it);
        return std::nullopt;
    }

    return it->second;
}

void DbgAdapter::cacheInstruction(uint64_t addr, uint64_t length, bool isCall)
{
    std::lock_guard<std::mutex> lock(mCacheMutex);

    // Prune cache if too large
    if(mInstructionCache.size() >= MAX_CACHE_SIZE)
    {
        pruneInstructionCache();
    }

    auto now = std::chrono::steady_clock::now().time_since_epoch().count() / 1000000;
    mInstructionCache[addr] = {length, isCall, static_cast<uint64_t>(now)};
}

void DbgAdapter::clearInstructionCache()
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    mInstructionCache.clear();
}

void DbgAdapter::pruneInstructionCache()
{
    // Remove oldest 25% of entries
    if(mInstructionCache.empty())
        return;

    std::vector<std::pair<uint64_t, uint64_t>> entries; // addr, timestamp
    for(const auto& [addr, info] : mInstructionCache)
    {
        entries.emplace_back(addr, info.timestamp);
    }

    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    size_t toRemove = entries.size() / 4;
    for(size_t i = 0; i < toRemove && i < entries.size(); ++i)
    {
        mInstructionCache.erase(entries[i].first);
    }
}

#include "elfbug_api.h"
#include "../core/Debugger.h"
#include "../thread/Registers.h"

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <vector>
#include <fcntl.h>

struct ElfBugDebugger : ElfBug::Debugger
{
    ElfBugCallbacks cb = {};

    std::atomic<bool> active{false};
    std::atomic<pid_t> activePid{0};
    uint64_t entryPoint = 0;

    struct MemRegion
    {
        uint64_t start, end;
        bool executable;
    };
    mutable std::mutex mapMutex;
    std::vector<MemRegion> memoryMap;

    mutable std::mutex bpDataMutex;
    std::set<uint64_t> breakpointAddrs;

    std::mutex bpQueueMutex;
    struct BpRequest
    {
        uint64_t addr;
        bool setOrDelete; // true = set, false = delete
    };
    std::vector<BpRequest> pendingBpRequests;

    void refreshMemoryMap()
    {
        std::vector<MemRegion> newMaps;
        if(!mProcess)
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
            return;
        }

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", mProcess->pid);
        FILE* f = fopen(path, "r");
        if(!f)
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
            return;
        }

        char line[512];
        while(fgets(line, sizeof(line), f))
        {
            uint64_t start = 0, end = 0;
            char perms[8] = {};
            if(sscanf(line, "%" SCNx64 "-%" SCNx64 " %4s", &start, &end, perms) == 3)
                newMaps.push_back({start, end, perms[2] == 'x'});
        }
        fclose(f);

        std::lock_guard lock(mapMutex);
        memoryMap = std::move(newMaps);
    }

    const MemRegion* findRegion(const uint64_t addr) const
    {
        auto it = std::upper_bound(memoryMap.begin(), memoryMap.end(), addr,
        [](const uint64_t a, const MemRegion & r) { return a < r.start; });
        if(it != memoryMap.begin())
        {
            --it;
            if(addr >= it->start && addr < it->end)
                return &*it;
        }
        return nullptr;
    }

    void processPendingBreakpoints()
    {
        std::vector<BpRequest> pending;
        {
            std::lock_guard lock(bpQueueMutex);
            pending.swap(pendingBpRequests);
        }

        for(const auto & req : pending)
        {
            if(!mProcess)
                continue;

            const auto addr = static_cast<ElfBug::ptr>(req.addr);
            if(req.setOrDelete)
            {
                if(mProcess->SetBreakpoint(addr))
                {
                    std::lock_guard lock(bpDataMutex);
                    breakpointAddrs.insert(req.addr);
                }
            }
            else
            {
                if(mProcess->DeleteBreakpoint(addr))
                {
                    std::lock_guard lock(bpDataMutex);
                    breakpointAddrs.erase(req.addr);
                }
            }
        }
    }

    bool memRead(const uint64_t addr, void* dest, const uint64_t size) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mProcess)
        {
            memset(dest, 0, size);
            return false;
        }
        return mProcess->MemRead(static_cast<ElfBug::ptr>(addr), dest, static_cast<ElfBug::ptr>(size));
    }

    ElfBugArch getArch() const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mProcess)
            return ElfBugArch_Unknown;
        switch(mProcess->arch)
        {
        case ElfBug::Arch::X86_64:
            return ElfBugArch_X86_64;
        case ElfBug::Arch::I386:
            return ElfBugArch_I386;
        default:
            return ElfBugArch_Unknown;
        }
    }

    bool readRegisters(ElfBugRegisters* out) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mThread)
            return false;

        const auto native = mThread->registers.Native();

        out->rax = native.rax;
        out->rbx = native.rbx;
        out->rcx = native.rcx;
        out->rdx = native.rdx;
        out->rbp = native.rbp;
        out->rsp = native.rsp;
        out->rsi = native.rsi;
        out->rdi = native.rdi;
        out->r8  = native.r8;
        out->r9  = native.r9;
        out->r10 = native.r10;
        out->r11 = native.r11;
        out->r12 = native.r12;
        out->r13 = native.r13;
        out->r14 = native.r14;
        out->r15 = native.r15;
        out->rip = native.rip;
        out->eflags = native.eflags;
        out->cs = static_cast<uint16_t>(native.cs);
        out->ds = static_cast<uint16_t>(native.ds);
        out->es = static_cast<uint16_t>(native.es);
        out->fs = static_cast<uint16_t>(native.fs);
        out->gs = static_cast<uint16_t>(native.gs);
        out->ss = static_cast<uint16_t>(native.ss);
        out->fs_base = native.fs_base;
        out->gs_base = native.gs_base;
        return true;
    }

protected:
    void cbCreateProcessEvent(const pid_t pid, const ElfBug::ptr ep) override
    {
        activePid.store(pid, std::memory_order_release);
        entryPoint = ep;
        refreshMemoryMap();
        active.store(true, std::memory_order_release);
        if(cb.onCreateProcess)
            cb.onCreateProcess(pid, ep, cb.userdata);
    }

    void cbExitProcessEvent(const int exitCode) override
    {
        activePid.store(0, std::memory_order_release);
        active.store(false, std::memory_order_release);
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
        }
        {
            std::lock_guard lock(bpDataMutex);
            breakpointAddrs.clear();
        }
        {
            std::lock_guard lock(bpQueueMutex);
            pendingBpRequests.clear();
        }
        if(cb.onExitProcess)
            cb.onExitProcess(exitCode, cb.userdata);
    }

    void cbSystemBreakpoint() override
    {
        if(mThread)
        {
            mThread->registers.Read();
            entryPoint = mThread->registers.Gip();
            refreshMemoryMap();
            processPendingBreakpoints();
        }
        if(cb.onSystemBreakpoint)
            cb.onSystemBreakpoint(cb.userdata);
    }

    void cbBreakpoint(const ElfBug::BreakpointInfo & info) override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onBreakpoint)
            cb.onBreakpoint(info.address, cb.userdata);
    }

    void cbStep() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onStep)
            cb.onStep(cb.userdata);
    }

    void cbPaused() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onPaused)
            cb.onPaused(cb.userdata);
    }

    void cbPauseTick() override
    {
        processPendingBreakpoints();
    }

    void cbInternalError(const std::string & error) override
    {
        if(cb.onError)
            cb.onError(error.c_str(), cb.userdata);
    }

    void cbDebugStringEvent(const std::string & text) override
    {
        if(cb.onDebugString)
            cb.onDebugString(text.c_str(), cb.userdata);
    }
};

extern "C" {

    ElfBugDebugger* ElfBugCreate(const ElfBugCallbacks* callbacks)
    {
        auto* dbg = new ElfBugDebugger();
        if(callbacks)
            dbg->cb = *callbacks;
        return dbg;
    }

    void ElfBugDestroy(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        delete dbg;
    }

    bool ElfBugInit(ElfBugDebugger* dbg, const char* path)
    {
        if(!dbg)
            return false;
        return dbg->Init(path);
    }

    void ElfBugStart(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Start();
    }

    void ElfBugContinue(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Continue();
    }

    void ElfBugStepInto(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->StepInto();
    }

    void ElfBugPause(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Pause();
    }

    bool ElfBugStop(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return false;
        return dbg->Stop();
    }

    bool ElfBugGetRegisters(const ElfBugDebugger* dbg, ElfBugRegisters* regs)
    {
        if(!dbg || !regs)
            return false;
        return dbg->readRegisters(regs);
    }

    pid_t ElfBugGetPid(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return 0;
        return dbg->activePid.load(std::memory_order_acquire);
    }

    ElfBugArch ElfBugGetArch(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return ElfBugArch_Unknown;
        return dbg->getArch();
    }

    bool ElfBugMemRead(const ElfBugDebugger* dbg, const uint64_t addr, void* dest, const uint64_t size)
    {
        if(!dbg)
            return false;
        return dbg->memRead(addr, dest, size);
    }

    bool ElfBugMemFindBaseAddr(const ElfBugDebugger* dbg, const uint64_t addr, uint64_t* base, uint64_t* size)
    {
        if(!dbg || !base || !size)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        const auto* region = dbg->findRegion(addr);
        if(!region)
            return false;

        *base = region->start;
        *size = region->end - region->start;
        return true;
    }

    bool ElfBugMemIsCodePtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        const auto* region = dbg->findRegion(addr);
        return region && region->executable;
    }

    bool ElfBugMemIsValidPtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        return dbg->findRegion(addr) != nullptr;
    }

    bool ElfBugSetBreakpoint(ElfBugDebugger* dbg, uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->bpQueueMutex);
        dbg->pendingBpRequests.push_back({addr, true});
        return true;
    }

    bool ElfBugDeleteBreakpoint(ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->bpQueueMutex);
        dbg->pendingBpRequests.push_back({addr, false});
        return true;
    }

    bool ElfBugHasBreakpoint(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        std::lock_guard lock(dbg->bpDataMutex);
        return dbg->breakpointAddrs.count(addr) > 0;
    }

} // extern "C"

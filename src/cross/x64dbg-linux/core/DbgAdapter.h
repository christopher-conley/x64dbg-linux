#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <unordered_map>
#include <ElfBug/api/elfbug_api.h>
#include "RegisterContext.h"
#include "Bridge.h"
#include "HardwareBreakpointManager.h"
#include "MemoryBreakpointManager.h"

// Forward declaration
namespace X64DbgLinux {
    class ThreadManager;
}

Q_DECLARE_METATYPE(REGDUMP)

class DbgAdapter : public QObject, public MemoryProvider
{
    Q_OBJECT

public:
    explicit DbgAdapter(QObject* parent = nullptr);
    ~DbgAdapter() override;

    bool read(duint addr, void* dest, duint size) override;
    bool write(duint addr, const void* src, duint size) override;
    bool getRange(duint addr, duint & base, duint & size) override;
    bool isCodePtr(duint addr) override;
    bool isValidPtr(duint addr) override;
    bool writeRegister(const char* name, duint value) override;
    bool modBaseFromAddr(duint addr, duint & base) override;
    bool modNameFromAddr(duint addr, char* buf, duint bufSize, bool extension) override;

    bool loadEngine();
    bool launch(const char* path) const;
    void Start() const;
    void Continue() const;
    void StepInto() const;
    void StepOver();
    void Pause() const;
    bool Stop() const;

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isEngineLoaded() const { return mDebugger != nullptr; }
    [[nodiscard]] duint entryPoint() const { return mEntryPoint; }
    [[nodiscard]] pid_t getPid() const;

    [[nodiscard]] bool toggleBreakpoint(duint addr) const;
    [[nodiscard]] bool hasBreakpoint(duint addr) const;

    // Hardware breakpoint management
    bool setHardwareBreakpoint(int slot, uint64_t addr, X64DbgLinux::HardwareBreakpointManager::Type type,
                               X64DbgLinux::HardwareBreakpointManager::Size size);
    bool clearHardwareBreakpoint(int slot);
    bool enableHardwareBreakpoint(int slot);
    bool disableHardwareBreakpoint(int slot);
    std::optional<int> findFreeHardwareBreakpointSlot() const;
    void clearAllHardwareBreakpoints();

    // Memory breakpoint management
    bool setMemoryBreakpoint(uint64_t addr, size_t size, X64DbgLinux::MemoryBreakpointType type);
    bool removeMemoryBreakpoint(uint64_t addr);
    bool enableMemoryBreakpoint(uint64_t addr);
    bool disableMemoryBreakpoint(uint64_t addr);
    void clearAllMemoryBreakpoints();

    // Thread management
    void setThreadManager(X64DbgLinux::ThreadManager* manager);
    void setCurrentThread(pid_t tid);

signals:
    void processCreated(duint entryPoint);
    void processExited(int exitCode);
    void registersUpdated(const REGDUMP & regs);
    void logMessage(const QString & msg);
    void stopped(duint rip, const QString & reason);

private:
    static BPXTYPE queryBreakpoint(duint addr);
    static std::atomic<DbgAdapter*> sInstance;

    static void onCreateProcess(pid_t pid, uint64_t entryPoint, void* userdata);
    static void onExitProcess(int exitCode, void* userdata);
    static void onSystemBreakpoint(void* userdata);
    static void onBreakpoint(uint64_t address, void* userdata);
    static void onStep(void* userdata);
    static void onPaused(void* userdata);
    static void onError(const char* error, void* userdata);
    static void onDebugString(const char* text, void* userdata);

    void emitStoppedState(const QString & reason);

    // Instruction cache for performance optimization
    struct CachedInstruction {
        uint64_t length;
        bool isCall;
        uint64_t timestamp;
    };

    std::optional<CachedInstruction> getCachedInstruction(uint64_t addr) const;
    void cacheInstruction(uint64_t addr, uint64_t length, bool isCall);
    void clearInstructionCache();
    void pruneInstructionCache();

    // Configuration
    static constexpr size_t MAX_CACHE_SIZE = 1024;
    static constexpr uint64_t CACHE_TTL_MS = 5000; // 5 seconds

    ElfBugDebugger* mDebugger = nullptr;
    duint mEntryPoint = 0;
    X64DbgLinux::ThreadManager* mThreadManager = nullptr;

    // Hardware and memory breakpoint managers
    std::unique_ptr<X64DbgLinux::HardwareBreakpointManager> mHwBpManager;
    std::unique_ptr<X64DbgLinux::MemoryBreakpointManager> mMemBpManager;

    // Instruction cache for Step Over optimization
    mutable std::unordered_map<uint64_t, CachedInstruction> mInstructionCache;
    mutable std::mutex mCacheMutex;
};

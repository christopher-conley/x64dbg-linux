#pragma once

#include <atomic>
#include "core/ElfBugLoader.h"
#include "RegisterContext.h"
#include "Bridge.h"

Q_DECLARE_METATYPE(REGDUMP)

class DbgAdapter : public QObject, public MemoryProvider
{
    Q_OBJECT

public:
    explicit DbgAdapter(QObject* parent = nullptr);
    ~DbgAdapter() override;

    bool read(duint addr, void* dest, duint size) override;
    bool getRange(duint addr, duint & base, duint & size) override;
    bool isCodePtr(duint addr) override;
    bool isValidPtr(duint addr) override;

    bool loadEngine();
    bool launch(const char* path) const;
    void Start() const;
    void Continue() const;
    void StepInto() const;
    void Pause() const;
    bool Stop() const; //discardable

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isEngineLoaded() const { return mApi.isLoaded(); }
    [[nodiscard]] duint entryPoint() const { return mEntryPoint; }

    [[nodiscard]] bool toggleBreakpoint(duint addr) const;
    [[nodiscard]] bool hasBreakpoint(duint addr) const;

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

    ElfBugApi mApi;
    ElfBugDebugger* mDebugger = nullptr;
    duint mEntryPoint = 0;
};

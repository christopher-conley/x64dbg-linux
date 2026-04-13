#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <csignal>
#include <cstring>

namespace ElfBug
{
    Debugger::Debugger() = default;
    Debugger::~Debugger() = default;

    bool Debugger::Init(const char* szFilePath, const char* const* argv, const char* szCurrentDirectory)
    {
        const pid_t pid = fork();
        if(pid == -1)
        {
            cbInternalError("fork() failed: " + std::string(strerror(errno)));
            return false;
        }

        if(pid == 0)
        {
            if(szCurrentDirectory)
            {
                if(chdir(szCurrentDirectory) == -1)
                    _exit(1);
            }

            personality(ADDR_NO_RANDOMIZE);

            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1)
                _exit(1);

            if(argv)
                execv(szFilePath, const_cast<char* const*>(argv));
            else
            {
                const char* defaultArgv[] = { szFilePath, nullptr };
                execv(szFilePath, const_cast<char* const*>(defaultArgv));
            }

            _exit(1);
        }

        mMainPid = pid;
        mIsAttached = false;
        return true;
    }

    bool Debugger::Attach(const pid_t processId)
    {
        // TODO: implement ptrace attach
        (void)processId;
        return false;
    }

    void Debugger::Start()
    {
        mIsRunning.store(true, std::memory_order_release);
        debugLoop();
    }

    void Debugger::Continue()
    {
        mPaused.store(false, std::memory_order_release);
    }

    void Debugger::StepInto()
    {
        mStepPending.store(true, std::memory_order_release);
        mPaused.store(false, std::memory_order_release);
    }

    void Debugger::Pause()
    {
        if(mMainPid > 0)
        {
            mPauseRequested.store(true, std::memory_order_release);
            kill(mMainPid, SIGSTOP);
        }
    }

    bool Debugger::Stop()
    {
        if(mMainPid <= 0)
            return false;

        mPaused.store(false, std::memory_order_release);

        return kill(mMainPid, SIGKILL) == 0;
    }

    void Debugger::Detach()
    {
        // TODO: implement ptrace detach
    }

    void Debugger::cbCreateProcessEvent(const pid_t pid, const ptr entryPoint) { (void)pid; (void)entryPoint; }
    void Debugger::cbExitProcessEvent(const int exitCode) { (void)exitCode; }
    void Debugger::cbCreateThreadEvent(const pid_t tid) { (void)tid; }
    void Debugger::cbExitThreadEvent(const pid_t tid) { (void)tid; }
    void Debugger::cbLoadDllEvent(const ptr baseAddress, const std::string& path) { (void)baseAddress; (void)path; }
    void Debugger::cbUnloadDllEvent(const ptr baseAddress) { (void)baseAddress; }
    void Debugger::cbExceptionEvent(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbBreakpoint(const BreakpointInfo& info) { (void)info; }
    void Debugger::cbStep() {}
    void Debugger::cbSystemBreakpoint() {}
    void Debugger::cbAttachBreakpoint() {}
    void Debugger::cbUnhandledException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbInternalError(const std::string& error) { (void)error; }
    void Debugger::cbDebugStringEvent(const std::string& text) { (void)text; }
    void Debugger::cbPaused() {}
    void Debugger::cbPauseTick() {}
}

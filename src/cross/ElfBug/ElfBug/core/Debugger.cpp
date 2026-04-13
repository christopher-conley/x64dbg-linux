#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>

namespace ElfBug
{
    Debugger::Debugger() = default;

    Debugger::~Debugger()
    {
        if(mMainPid > 0)
        {
            kill(mMainPid, SIGKILL);
            waitpid(mMainPid, nullptr, __WALL);
        }
    }

    bool Debugger::Init(const char* szFilePath, const char* const* argv, const char* szCurrentDirectory)
    {
        int pipeFds[2];
        if(pipe2(pipeFds, O_CLOEXEC) == -1)
        {
            cbInternalError("pipe2() failed: " + std::string(strerror(errno)));
            return false;
        }

        const pid_t pid = fork();
        if(pid == -1)
        {
            close(pipeFds[0]);
            close(pipeFds[1]);
            cbInternalError("fork() failed: " + std::string(strerror(errno)));
            return false;
        }

        if(pid == 0)
        {
            close(pipeFds[0]);

            auto childError = [&](const char* msg)
            {
                write(pipeFds[1], msg, strlen(msg));
                _exit(1);
            };

            if(setpgid(0, 0) < 0)
                childError("setpgid failed");

            if(szCurrentDirectory)
            {
                if(chdir(szCurrentDirectory) == -1)
                    childError("chdir failed");
            }

            if(personality(ADDR_NO_RANDOMIZE) == -1)
                childError("personality(ADDR_NO_RANDOMIZE) failed");

            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1)
                childError("PTRACE_TRACEME failed");

            if(argv)
                execv(szFilePath, const_cast<char* const*>(argv));
            else
            {
                const char* defaultArgv[] = { szFilePath, nullptr };
                execv(szFilePath, const_cast<char* const*>(defaultArgv));
            }

            childError("execv failed");
        }

        close(pipeFds[1]);

        char errBuf[256] = {};
        const ssize_t n = read(pipeFds[0], errBuf, sizeof(errBuf) - 1);
        close(pipeFds[0]);

        if(n > 0)
        {
            waitpid(pid, nullptr, 0);
            cbInternalError("child process failed: " + std::string(errBuf));
            return false;
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
        {
            std::lock_guard lock(mPauseMutex);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
    }

    void Debugger::StepInto()
    {
        {
            std::lock_guard lock(mPauseMutex);
            mStepPending.store(true, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
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

        mIsRunning.store(false, std::memory_order_release);
        {
            std::lock_guard lock(mPauseMutex);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();

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
    void Debugger::cbLoadDllEvent(const ptr baseAddress, const std::string & path) { (void)baseAddress; (void)path; }
    void Debugger::cbUnloadDllEvent(const ptr baseAddress) { (void)baseAddress; }
    void Debugger::cbExceptionEvent(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbBreakpoint(const BreakpointInfo & info) { (void)info; }
    void Debugger::cbStep() {}
    void Debugger::cbSystemBreakpoint() {}
    void Debugger::cbAttachBreakpoint() {}
    void Debugger::cbUnhandledException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbInternalError(const std::string & error) { (void)error; }
    void Debugger::cbDebugStringEvent(const std::string & text) { (void)text; }
    void Debugger::cbPaused() {}
    void Debugger::cbPauseTick() {}
}

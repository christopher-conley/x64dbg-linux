#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <chrono>

namespace ElfBug
{
    // Publishes mPaused=true under the mutex before the event callback fires,
    // so a concurrent Continue()/Stop() can't race past and strand pauseAndResume().
    void Debugger::beginPause()
    {
        std::lock_guard lock(mPauseMutex);
        mPaused.store(true, std::memory_order_release);
    }

    bool Debugger::pauseAndResume(const pid_t pid)
    {
        std::unique_lock lock(mPauseMutex);

        while(mPaused.load(std::memory_order_acquire) && mIsRunning.load(std::memory_order_acquire))
        {
            if(mPauseCv.wait_for(lock, std::chrono::milliseconds(10)) == std::cv_status::timeout)
                cbPauseTick();
        }

        lock.unlock();

        if(!mIsRunning.load(std::memory_order_acquire))
            return false;

        if(mStepPending.load(std::memory_order_acquire) && mThread)
        {
            mStepPending.store(false, std::memory_order_release);
            mThread->StepInto();
        }
        else
        {
            const int sig = mPendingSignal;
            mPendingSignal = 0;
            if(ptrace(PTRACE_CONT, pid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
        }
        return true;
    }

    void Debugger::debugLoop()
    {
        if(!launchChild())
        {
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        const pid_t mainPid = mMainPid.load(std::memory_order_relaxed);

        int status = 0;
        pid_t pid = waitpid(mainPid, &status, __WALL);
        if(pid == -1)
        {
            cbInternalError("initial waitpid() failed: " + std::string(strerror(errno)));
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        if(!WIFSTOPPED(status))
        {
            int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
            cbInternalError("child exited before reaching first stop (code " + std::to_string(code) + ")");
            cbExitProcessEvent(code);
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        if(ptrace(PTRACE_SETOPTIONS, mainPid, nullptr,
                  PTRACE_O_TRACESYSGOOD |
                  PTRACE_O_TRACECLONE |
                  PTRACE_O_TRACEEXEC |
                  PTRACE_O_TRACEEXIT) == -1)
        {
            cbInternalError("PTRACE_SETOPTIONS failed: " + std::string(strerror(errno)));
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        createProcessEvent(mainPid);

        mSystemBreakpointHit = true;
        if(mThread)
        {
            mThread->registers.Read();
            beginPause();
            cbSystemBreakpoint();
        }

        if(!pauseAndResume(mainPid))
            return;

        while(mIsRunning)
        {
            pid = waitpid(-1, &status, __WALL);
            if(pid == -1)
            {
                if(errno == ECHILD)
                {
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                continue;
            }

            if(WIFEXITED(status))
            {
                if(pid == mMainPid.load(std::memory_order_relaxed))
                {
                    exitProcessEvent(pid, WEXITSTATUS(status));
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                exitThreadEvent(pid);
                continue;
            }

            if(WIFSIGNALED(status))
            {
                if(pid == mMainPid.load(std::memory_order_relaxed))
                {
                    exitProcessEvent(pid, -WTERMSIG(status));
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                exitThreadEvent(pid);
                continue;
            }

            if(WIFSTOPPED(status))
            {
                handleSignal(pid, status);
            }
        }
    }
}

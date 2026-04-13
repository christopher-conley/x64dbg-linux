#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

namespace ElfBug
{
    bool Debugger::pauseAndResume(const pid_t pid)
    {
        mPaused.store(true, std::memory_order_release);
        while(mPaused.load(std::memory_order_acquire) && mIsRunning.load(std::memory_order_acquire))
        {
            cbPauseTick();
            usleep(1000);
        }

        if(!mIsRunning.load(std::memory_order_acquire))
            return false;

        if(mStepPending.load(std::memory_order_acquire) && mThread)
        {
            mStepPending.store(false, std::memory_order_relaxed);
            mThread->StepInto();
        }
        else
        {
            ptrace(PTRACE_CONT, pid, nullptr, nullptr);
        }
        return true;
    }

    void Debugger::debugLoop()
    {
        int status = 0;
        pid_t pid = waitpid(mMainPid, &status, __WALL);
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

        if(ptrace(PTRACE_SETOPTIONS, mMainPid, nullptr,
                  PTRACE_O_TRACECLONE |
                  PTRACE_O_TRACEEXEC |
                  PTRACE_O_TRACEEXIT) == -1)
        {
            cbInternalError("PTRACE_SETOPTIONS failed: " + std::string(strerror(errno)));
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        createProcessEvent(mMainPid);

        mSystemBreakpointHit = true;
        if(mThread)
        {
            mThread->registers.Read();
            cbSystemBreakpoint();
        }

        if(!pauseAndResume(mMainPid))
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
                exitProcessEvent(pid, WEXITSTATUS(status));
                if(pid == mMainPid)
                {
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                continue;
            }

            if(WIFSIGNALED(status))
            {
                exitProcessEvent(pid, -WTERMSIG(status));
                if(pid == mMainPid)
                {
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                continue;
            }

            if(WIFSTOPPED(status))
            {
                handleSignal(pid, status);
            }
        }
    }
}

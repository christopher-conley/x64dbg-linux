#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

namespace ElfBug
{
    void Debugger::handleSignal(const pid_t pid, const int status)
    {
        const int sig = WSTOPSIG(status);

        if(mProcess)
        {
            const auto it = mProcess->threads.find(pid);
            if(it != mProcess->threads.end())
                mThread = it->second.get();
        }

        switch(sig)
        {
        case SIGTRAP:
            handleSigtrap(pid, status);
            break;

        case SIGSTOP:
        {
            if(mPauseRequested.load(std::memory_order_acquire))
            {
                mPauseRequested.store(false, std::memory_order_release);
                if(mThread)
                {
                    mThread->registers.Read();
                    cbPaused();

                    if(!pauseAndResume(pid))
                        break;
                }
                else
                {
                    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
                }
            }
            else
            {
                ptrace(PTRACE_CONT, pid, nullptr, nullptr);
            }
            break;
        }

        default:
            cbExceptionEvent(sig, 0);
            ptrace(PTRACE_CONT, pid, nullptr, reinterpret_cast<void*>(static_cast<uintptr_t>(sig)));
            break;
        }
    }

    void Debugger::handleSigtrap(const pid_t pid, const int status)
    {
        const int event = (status >> 16) & 0xffff;

        switch(event)
        {
        case PTRACE_EVENT_EXEC:
        {
            if(!mSystemBreakpointHit)
            {
                mSystemBreakpointHit = true;
                if(mThread)
                {
                    mThread->registers.Read();
                    cbSystemBreakpoint();
                }
                break;
            }
            ptrace(PTRACE_CONT, pid, nullptr, nullptr);
            break;
        }

        case PTRACE_EVENT_CLONE:
        {
            unsigned long newTid = 0;
            ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &newTid);
            createThreadEvent(static_cast<pid_t>(newTid));
            ptrace(PTRACE_CONT, pid, nullptr, nullptr);
            break;
        }

        case PTRACE_EVENT_EXIT:
        {
            exitThreadEvent(pid);
            ptrace(PTRACE_CONT, pid, nullptr, nullptr);
            break;
        }

        default:
        {
            if(!mThread)
            {
                ptrace(PTRACE_CONT, pid, nullptr, nullptr);
                break;
            }

            mThread->registers.Read();

            if(mThread->isSingleStepping())
            {
                mThread->clearSingleStep();
                cbStep();
                if(!pauseAndResume(pid))
                    break;
                break;
            }

            if(mProcess)
            {
                ptr bpAddr = mThread->registers.Gip() - 1;
                const BreakpointKey key{BreakpointType::Software, bpAddr};
                const auto it = mProcess->breakpoints.find(key);

                if(it != mProcess->breakpoints.end())
                {
                    mThread->registers.Gip() = bpAddr;
                    mThread->registers.Write();

                    const auto& info = it->second;
                    const bool singleshot = info.singleshot;

                    const auto cbIt = mProcess->breakpointCallbacks.find(key);
                    if(cbIt != mProcess->breakpointCallbacks.end())
                        cbIt->second(info);

                    cbBreakpoint(info);

                    if(singleshot)
                    {
                        mProcess->DeleteBreakpoint(bpAddr);
                    }
                    else
                    {
                        mProcess->DeleteBreakpoint(bpAddr);
                        mThread->StepInto();
                        int stepStatus = 0;
                        waitpid(pid, &stepStatus, __WALL);
                        mThread->clearSingleStep();

                        if(WIFEXITED(stepStatus) || WIFSIGNALED(stepStatus))
                        {
                            exitProcessEvent(mMainPid, WIFEXITED(stepStatus)
                                             ? WEXITSTATUS(stepStatus)
                                             : -WTERMSIG(stepStatus));
                            mIsRunning.store(false, std::memory_order_release);
                            break;
                        }

                        if(WIFSTOPPED(stepStatus))
                        {
                            const int stepSig = WSTOPSIG(stepStatus);
                            mThread->registers.Read();
                            mProcess->SetBreakpoint(bpAddr, false);

                            if(stepSig != SIGTRAP)
                            {
                                ptrace(PTRACE_CONT, pid, nullptr,
                                       reinterpret_cast<void*>(static_cast<uintptr_t>(stepSig)));
                                break;
                            }
                        }
                    }

                    if(!pauseAndResume(pid))
                        break;
                    break;
                }
            }

            ptrace(PTRACE_CONT, pid, nullptr, nullptr);
            break;
        }
        }
    }
}

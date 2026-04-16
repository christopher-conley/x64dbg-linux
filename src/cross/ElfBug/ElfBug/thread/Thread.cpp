#include <ElfBug/thread/Thread.h>
#include <sys/ptrace.h>

namespace ElfBug
{
    Thread::Thread(const pid_t tid)
        : tid(tid)
        , registers(tid)
    {
    }

    bool Thread::StepInto()
    {
        if(ptrace(PTRACE_SINGLESTEP, tid, nullptr, nullptr) == -1)
            return false;
        mIsSingleStepping = true;
        return true;
    }

    bool Thread::StepInto(const StepCallback & cbStep)
    {
        mStepCallback = cbStep;
        return StepInto();
    }
}

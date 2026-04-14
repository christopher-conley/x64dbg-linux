#include <ElfBug/thread/Thread.h>
#include <sys/ptrace.h>

namespace ElfBug
{
    Thread::Thread(const pid_t tid)
        : tid(tid)
        , registers(tid)
    {
    }

    void Thread::StepInto()
    {
        mIsSingleStepping = true;
        ptrace(PTRACE_SINGLESTEP, tid, nullptr, nullptr);
    }

    void Thread::StepInto(const StepCallback & cbStep)
    {
        mStepCallback = cbStep;
        StepInto();
    }
}

#include <ElfBug/core/Debugger.h>

namespace ElfBug
{
    void Debugger::createThreadEvent(pid_t tid)
    {
        if(!mProcess)
            return;

        mProcess->threads.emplace(tid, std::make_unique<Thread>(tid));
        mThread = mProcess->threads.at(tid).get();
        cbCreateThreadEvent(tid);
    }

    void Debugger::exitThreadEvent(const pid_t tid)
    {
        if(!mProcess)
            return;

        cbExitThreadEvent(tid);

        if(mThread && mThread->tid == tid)
            mThread = nullptr;

        mProcess->threads.erase(tid);
    }
}

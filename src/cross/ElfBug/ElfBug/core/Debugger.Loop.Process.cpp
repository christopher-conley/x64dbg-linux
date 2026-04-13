#include <ElfBug/core/Debugger.h>

namespace ElfBug
{
    void Debugger::createProcessEvent(pid_t pid)
    {
        auto [it, inserted] = mProcesses.try_emplace(pid, pid);
        mProcess = &it->second;

        mProcess->threads.emplace(pid, std::make_unique<Thread>(pid));
        mThread = mProcess->threads.at(pid).get();

        mThread->registers.Read();
        const ptr entryPoint = mThread->registers.Gip();
        cbCreateProcessEvent(pid, entryPoint);
    }

    void Debugger::exitProcessEvent(const pid_t pid, const int exitCode)
    {
        cbExitProcessEvent(exitCode);
        mProcesses.erase(pid);

        if(pid == mMainPid)
        {
            mProcess = nullptr;
            mThread = nullptr;
        }
    }
}

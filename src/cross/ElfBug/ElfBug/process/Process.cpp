#include <ElfBug/process/Process.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

namespace ElfBug
{
    Process::Process(const pid_t pid)
        : pid(pid)
    {
    }

    Process::Process(Process&& other) noexcept
        : pid(other.pid)
        , threads(std::move(other.threads))
        , breakpoints(std::move(other.breakpoints))
        , breakpointCallbacks(std::move(other.breakpointCallbacks))
        , softwareBreakpointReferences(std::move(other.softwareBreakpointReferences))
        , memoryBreakpointRanges(std::move(other.memoryBreakpointRanges))
        , memoryBreakpointPages(std::move(other.memoryBreakpointPages))
        , mMemFd(other.mMemFd)
    {
        other.mMemFd = -1;
    }

    Process::~Process()
    {
        if(mMemFd != -1)
            close(mMemFd);
    }

    int Process::memFd() const
    {
        if(mMemFd == -1)
        {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/mem", pid);
            mMemFd = open(path, O_RDWR);
            if(mMemFd == -1)
                mMemFd = open(path, O_RDONLY);
        }
        return mMemFd;
    }
}

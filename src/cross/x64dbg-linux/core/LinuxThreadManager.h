#pragma once
#include <sys/types.h>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>
#include "RegisterContext.h"

namespace X64DbgLinux {

struct ThreadInfo {
    pid_t tid;
    uint64_t startAddress;
    std::string name;
    bool suspended;
    REGISTERCONTEXT registers;
};

class ThreadManager {
public:
    void addThread(pid_t tid, uint64_t startAddr);
    void removeThread(pid_t tid);
    void updateThreadName(pid_t tid, const std::string& name);
    std::vector<ThreadInfo> getAllThreads() const;
    bool suspendThread(pid_t tid);
    bool resumeThread(pid_t tid);
    bool getThreadContext(pid_t tid, REGISTERCONTEXT* ctx);
    bool setThreadContext(pid_t tid, const REGISTERCONTEXT& ctx);
    pid_t getCurrentThreadId() const { return m_currentTid; }
    void setCurrentThread(pid_t tid) { m_currentTid = tid; }

private:
    std::unordered_map<pid_t, ThreadInfo> m_threads;
    mutable std::mutex m_mutex;
    pid_t m_currentTid = 0;
};

}

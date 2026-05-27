#include "LinuxThreadManager.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <cstring>

namespace X64DbgLinux {

void ThreadManager::addThread(pid_t tid, uint64_t startAddr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ThreadInfo info;
    info.tid = tid;
    info.startAddress = startAddr;
    info.name = "Thread " + std::to_string(tid);
    info.suspended = false;
    memset(&info.registers, 0, sizeof(info.registers));
    m_threads[tid] = info;
}

void ThreadManager::removeThread(pid_t tid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.erase(tid);
}

void ThreadManager::updateThreadName(pid_t tid, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threads.find(tid);
    if (it != m_threads.end()) {
        it->second.name = name;
    }
}

std::vector<ThreadInfo> ThreadManager::getAllThreads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ThreadInfo> result;
    for (const auto& [tid, info] : m_threads) {
        result.push_back(info);
    }
    return result;
}

bool ThreadManager::suspendThread(pid_t tid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threads.find(tid);
    if (it == m_threads.end()) {
        return false;
    }

    // Send interrupt signal to stop the thread
    if (ptrace(PTRACE_INTERRUPT, tid, nullptr, nullptr) == -1) {
        return false;
    }

    // Use WNOHANG to avoid blocking indefinitely
    // Try multiple times with short delay for the thread to stop
    int status;
    bool stopped = false;
    for (int retry = 0; retry < 100; retry++) {
        pid_t result = waitpid(tid, &status, WNOHANG | __WALL);
        if (result == tid) {
            if (WIFSTOPPED(status)) {
                stopped = true;
                break;
            }
        } else if (result == 0) {
            // Thread hasn't stopped yet, wait a bit
            usleep(1000); // 1ms
        } else {
            // Error
            return false;
        }
    }

    if (!stopped) {
        // Thread didn't stop in time
        return false;
    }

    it->second.suspended = true;
    return true;
}

bool ThreadManager::resumeThread(pid_t tid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threads.find(tid);
    if (it == m_threads.end()) {
        return false;
    }

    if (ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1) {
        return false;
    }

    it->second.suspended = false;
    return true;
}

bool ThreadManager::getThreadContext(pid_t tid, REGISTERCONTEXT* ctx) {
    if (!ctx) return false;

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == -1) {
        return false;
    }

    // Convert to REGISTERCONTEXT
    ctx->cax = regs.rax;
    ctx->cbx = regs.rbx;
    ctx->ccx = regs.rcx;
    ctx->cdx = regs.rdx;
    ctx->csi = regs.rsi;
    ctx->cdi = regs.rdi;
    ctx->cbp = regs.rbp;
    ctx->csp = regs.rsp;
    ctx->cip = regs.rip;
    ctx->r8 = regs.r8;
    ctx->r9 = regs.r9;
    ctx->r10 = regs.r10;
    ctx->r11 = regs.r11;
    ctx->r12 = regs.r12;
    ctx->r13 = regs.r13;
    ctx->r14 = regs.r14;
    ctx->r15 = regs.r15;
    ctx->eflags = regs.eflags;

    // Update cached registers
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_threads.find(tid);
        if (it != m_threads.end()) {
            it->second.registers = *ctx;
        }
    }

    return true;
}

bool ThreadManager::setThreadContext(pid_t tid, const REGISTERCONTEXT& ctx) {
    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == -1) {
        return false;
    }

    regs.rax = ctx.cax;
    regs.rbx = ctx.cbx;
    regs.rcx = ctx.ccx;
    regs.rdx = ctx.cdx;
    regs.rsi = ctx.csi;
    regs.rdi = ctx.cdi;
    regs.rbp = ctx.cbp;
    regs.rsp = ctx.csp;
    regs.rip = ctx.cip;
    regs.r8 = ctx.r8;
    regs.r9 = ctx.r9;
    regs.r10 = ctx.r10;
    regs.r11 = ctx.r11;
    regs.r12 = ctx.r12;
    regs.r13 = ctx.r13;
    regs.r14 = ctx.r14;
    regs.r15 = ctx.r15;
    regs.eflags = ctx.eflags;

    if (ptrace(PTRACE_SETREGS, tid, nullptr, &regs) == -1) {
        return false;
    }

    // Update cached registers
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_threads.find(tid);
        if (it != m_threads.end()) {
            it->second.registers = ctx;
        }
    }

    return true;
}

}

#pragma once
#include <csignal>
#include <string>
#include <optional>
#include <cstdint>

namespace X64DbgLinux {

// 将 Unix 信号映射到 x64dbg 异常类型
enum class ExceptionType {
    Breakpoint,          // SIGTRAP (int3)
    SingleStep,          // SIGTRAP (single step)
    AccessViolation,     // SIGSEGV
    IllegalInstruction,  // SIGILL
    DivideByZero,        // SIGFPE
    Abort,               // SIGABRT
    Interrupt,           // SIGINT
    Unknown,
};

struct ExceptionInfo {
    ExceptionType type;
    uint64_t address;
    uint32_t code;
    std::string description;
};

class ExceptionHandler {
public:
    static ExceptionInfo MapSignalToException(int sig, siginfo_t* info);
    static std::string GetExceptionName(ExceptionType type);
    static std::string GetExceptionDescription(ExceptionType type, uint64_t address);

    // Check if signal is a debug-related signal
    static bool IsDebugSignal(int sig);

    // Get the instruction pointer from signal context
    static std::optional<uint64_t> GetFaultAddress(siginfo_t* info);
};

}

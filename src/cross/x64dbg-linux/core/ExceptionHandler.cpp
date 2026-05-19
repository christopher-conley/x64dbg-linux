#include "ExceptionHandler.h"
#include <sstream>

namespace X64DbgLinux {

// Exception codes (matching Windows/x64dbg conventions)
constexpr uint32_t EXCEPTION_BREAKPOINT = 0x80000003;
constexpr uint32_t EXCEPTION_SINGLE_STEP = 0x80000004;
constexpr uint32_t EXCEPTION_ACCESS_VIOLATION = 0xC0000005;
constexpr uint32_t EXCEPTION_ILLEGAL_INSTRUCTION = 0xC000001D;
constexpr uint32_t EXCEPTION_DIVIDE_BY_ZERO = 0xC0000094;
constexpr uint32_t EXCEPTION_ABORT = 0xC0000409;
constexpr uint32_t EXCEPTION_INTERRUPT = 0xC000013A;

ExceptionInfo ExceptionHandler::MapSignalToException(int sig, siginfo_t* info) {
    ExceptionInfo result;
    result.address = 0;
    result.code = 0;

    auto addr = GetFaultAddress(info);
    if (addr.has_value()) {
        result.address = addr.value();
    }

    switch (sig) {
        case SIGTRAP:
            // Check if it's a breakpoint or single step
            if (info && info->si_code == SI_KERNEL) {
                result.type = ExceptionType::Breakpoint;
            } else if (info && info->si_code == TRAP_TRACE) {
                result.type = ExceptionType::SingleStep;
            } else {
                result.type = ExceptionType::Breakpoint;
            }
            result.code = EXCEPTION_BREAKPOINT;
            break;

        case SIGSEGV:
            result.type = ExceptionType::AccessViolation;
            result.code = EXCEPTION_ACCESS_VIOLATION;
            break;

        case SIGILL:
            result.type = ExceptionType::IllegalInstruction;
            result.code = EXCEPTION_ILLEGAL_INSTRUCTION;
            break;

        case SIGFPE:
            result.type = ExceptionType::DivideByZero;
            result.code = EXCEPTION_DIVIDE_BY_ZERO;
            break;

        case SIGABRT:
            result.type = ExceptionType::Abort;
            result.code = EXCEPTION_ABORT;
            break;

        case SIGINT:
            result.type = ExceptionType::Interrupt;
            result.code = EXCEPTION_INTERRUPT;
            break;

        default:
            result.type = ExceptionType::Unknown;
            result.code = 0;
            break;
    }

    result.description = GetExceptionDescription(result.type, result.address);
    return result;
}

std::string ExceptionHandler::GetExceptionName(ExceptionType type) {
    switch (type) {
        case ExceptionType::Breakpoint: return "Breakpoint";
        case ExceptionType::SingleStep: return "Single Step";
        case ExceptionType::AccessViolation: return "Access Violation";
        case ExceptionType::IllegalInstruction: return "Illegal Instruction";
        case ExceptionType::DivideByZero: return "Divide By Zero";
        case ExceptionType::Abort: return "Abort";
        case ExceptionType::Interrupt: return "Interrupt";
        default: return "Unknown";
    }
}

std::string ExceptionHandler::GetExceptionDescription(ExceptionType type, uint64_t address) {
    std::stringstream ss;
    ss << GetExceptionName(type);
    if (address != 0) {
        ss << " at 0x" << std::hex << address;
    }
    return ss.str();
}

bool ExceptionHandler::IsDebugSignal(int sig) {
    return sig == SIGTRAP || sig == SIGSEGV || sig == SIGILL ||
           sig == SIGFPE || sig == SIGABRT || sig == SIGINT;
}

std::optional<uint64_t> ExceptionHandler::GetFaultAddress(siginfo_t* info) {
    if (!info) return std::nullopt;

    switch (info->si_signo) {
        case SIGSEGV:
        case SIGBUS:
            return reinterpret_cast<uint64_t>(info->si_addr);
        case SIGTRAP:
            // For breakpoints, the address might be in si_addr or need to be retrieved from registers
            return reinterpret_cast<uint64_t>(info->si_addr);
        default:
            return std::nullopt;
    }
}

}

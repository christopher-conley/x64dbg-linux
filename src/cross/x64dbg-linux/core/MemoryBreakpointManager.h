#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace X64DbgLinux {

enum class MemoryBreakpointType {
    Access,      // rwx
    Read,        // r-x
    Write,       // -wx
    Execute      // --x
};

struct MemoryBreakpoint {
    uint64_t address;
    size_t size;
    MemoryBreakpointType type;
    int originalProtection;
    bool enabled;
};

class MemoryBreakpointManager {
public:
    // Set a memory breakpoint
    bool setMemoryBreakpoint(uint64_t addr, size_t size, MemoryBreakpointType type);

    // Remove a memory breakpoint
    bool removeMemoryBreakpoint(uint64_t addr);

    // Handle memory protection fault
    bool handleMemoryProtectionFault(uint64_t addr, bool isWrite);

    // Enable/disable breakpoint
    bool enableMemoryBreakpoint(uint64_t addr);
    bool disableMemoryBreakpoint(uint64_t addr);

    // Check if address has a breakpoint
    bool hasMemoryBreakpoint(uint64_t addr) const;

    // Get all breakpoints
    std::vector<MemoryBreakpoint> getAllBreakpoints() const;

    // Clear all breakpoints
    void clearAllBreakpoints();

private:
    // Convert MemoryBreakpointType to mprotect flags
    int getProtectionFlags(MemoryBreakpointType type) const;

    // Set memory protection
    bool setMemoryProtection(uint64_t addr, size_t size, int prot);

    // Get current memory protection
    bool getMemoryProtection(uint64_t addr, int& prot) const;

    // Find breakpoint containing address
    std::optional<MemoryBreakpoint> findBreakpoint(uint64_t addr) const;

    std::unordered_map<uint64_t, MemoryBreakpoint> m_breakpoints;
    mutable std::mutex m_mutex;
};

}

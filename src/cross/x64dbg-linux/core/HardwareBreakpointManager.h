#pragma once
#include <cstdint>
#include <optional>

namespace X64DbgLinux {

class HardwareBreakpointManager {
public:
    static constexpr int MAX_HW_BREAKPOINTS = 4;

    enum class Type {
        Execute = 0,
        Write = 1,
        ReadWrite = 3
    };

    enum class Size {
        Byte = 0,
        Word = 1,
        Dword = 3,
        Qword = 3  // x64
    };

    // Set hardware breakpoint in specified slot (0-3)
    bool setBreakpoint(int slot, uint64_t addr, Type type, Size size);

    // Clear hardware breakpoint from slot
    bool clearBreakpoint(int slot);

    // Enable/disable breakpoint
    bool enableBreakpoint(int slot);
    bool disableBreakpoint(int slot);

    // Check if breakpoint is set in slot
    bool isBreakpointSet(int slot) const;

    // Get breakpoint address
    std::optional<uint64_t> getBreakpointAddress(int slot) const;

    // Find free slot
    std::optional<int> findFreeSlot() const;

    // Clear all hardware breakpoints
    void clearAllBreakpoints();

private:
    // x86 debug registers
    struct DebugRegisters {
        uint64_t dr0;
        uint64_t dr1;
        uint64_t dr2;
        uint64_t dr3;
        uint64_t dr6;
        uint64_t dr7;
    };

    bool readDebugRegisters(DebugRegisters& regs) const;
    bool writeDebugRegisters(const DebugRegisters& regs) const;

    uint64_t encodeDr7(int slot, Type type, Size size, bool enabled) const;
    void decodeDr7(uint64_t dr7, int slot, bool& enabled, Type& type, Size& size) const;

    uint64_t m_breakpoints[4] = {0, 0, 0, 0};
    bool m_enabled[4] = {false, false, false, false};
};

}

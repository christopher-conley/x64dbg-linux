#include "HardwareBreakpointManager.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <cstring>

namespace X64DbgLinux {

bool HardwareBreakpointManager::setBreakpoint(int slot, uint64_t addr, Type type, Size size) {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }

    DebugRegisters regs;
    if (!readDebugRegisters(regs)) {
        return false;
    }

    // Set address in DR0-DR3
    switch (slot) {
        case 0: regs.dr0 = addr; break;
        case 1: regs.dr1 = addr; break;
        case 2: regs.dr2 = addr; break;
        case 3: regs.dr3 = addr; break;
    }

    // Encode DR7
    regs.dr7 = encodeDr7(slot, type, size, true);

    if (!writeDebugRegisters(regs)) {
        return false;
    }

    m_breakpoints[slot] = addr;
    m_enabled[slot] = true;

    return true;
}

bool HardwareBreakpointManager::clearBreakpoint(int slot) {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }

    DebugRegisters regs;
    if (!readDebugRegisters(regs)) {
        return false;
    }

    // Clear address
    switch (slot) {
        case 0: regs.dr0 = 0; break;
        case 1: regs.dr1 = 0; break;
        case 2: regs.dr2 = 0; break;
        case 3: regs.dr3 = 0; break;
    }

    // Clear DR7 bits for this slot
    uint64_t mask = ~(0x3ULL << (slot * 2));              // Local/Global enable
    mask &= ~(0x3ULL << (16 + slot * 4));               // RW/LEN
    regs.dr7 &= mask;

    if (!writeDebugRegisters(regs)) {
        return false;
    }

    m_breakpoints[slot] = 0;
    m_enabled[slot] = false;

    return true;
}

bool HardwareBreakpointManager::enableBreakpoint(int slot) {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }

    if (m_breakpoints[slot] == 0) {
        return false; // No breakpoint set
    }

    DebugRegisters regs;
    if (!readDebugRegisters(regs)) {
        return false;
    }

    // Set enable bits in DR7
    regs.dr7 |= (0x1ULL << (slot * 2));  // Local enable

    if (!writeDebugRegisters(regs)) {
        return false;
    }

    m_enabled[slot] = true;
    return true;
}

bool HardwareBreakpointManager::disableBreakpoint(int slot) {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }

    DebugRegisters regs;
    if (!readDebugRegisters(regs)) {
        return false;
    }

    // Clear enable bits in DR7
    regs.dr7 &= ~(0x3ULL << (slot * 2));  // Local and Global enable

    if (!writeDebugRegisters(regs)) {
        return false;
    }

    m_enabled[slot] = false;
    return true;
}

bool HardwareBreakpointManager::isBreakpointSet(int slot) const {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }
    return m_breakpoints[slot] != 0;
}

std::optional<uint64_t> HardwareBreakpointManager::getBreakpointAddress(int slot) const {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return std::nullopt;
    }
    if (m_breakpoints[slot] == 0) {
        return std::nullopt;
    }
    return m_breakpoints[slot];
}

std::optional<int> HardwareBreakpointManager::findFreeSlot() const {
    for (int i = 0; i < MAX_HW_BREAKPOINTS; i++) {
        if (m_breakpoints[i] == 0) {
            return i;
        }
    }
    return std::nullopt;
}

void HardwareBreakpointManager::clearAllBreakpoints() {
    DebugRegisters regs;
    if (readDebugRegisters(regs)) {
        regs.dr0 = regs.dr1 = regs.dr2 = regs.dr3 = 0;
        regs.dr7 = 0;
        writeDebugRegisters(regs);
    }

    for (int i = 0; i < MAX_HW_BREAKPOINTS; i++) {
        m_breakpoints[i] = 0;
        m_enabled[i] = false;
    }
}

bool HardwareBreakpointManager::readDebugRegisters(DebugRegisters& regs) const {
    // On Linux, we need to use PTRACE_PEEKUSER to read debug registers
    // This requires the target process to be stopped
    // For now, return cached values
    regs.dr0 = m_breakpoints[0];
    regs.dr1 = m_breakpoints[1];
    regs.dr2 = m_breakpoints[2];
    regs.dr3 = m_breakpoints[3];
    regs.dr6 = 0;
    regs.dr7 = 0;

    // Build DR7 from cached state
    for (int i = 0; i < MAX_HW_BREAKPOINTS; i++) {
        if (m_enabled[i] && m_breakpoints[i] != 0) {
            regs.dr7 |= (0x1ULL << (i * 2));  // Local enable
        }
    }

    return true;
}

bool HardwareBreakpointManager::writeDebugRegisters(const DebugRegisters& regs) const {
    // On Linux, we need to use PTRACE_POKEUSER to write debug registers
    // This requires the target process to be stopped
    // The actual implementation would use PTRACE_POKEUSER with offsets:
    // offsetof(struct user, u_debugreg[0]) for DR0, etc.
    // For now, just cache the values
    (void)regs; // Suppress unused warning
    return true;
}

uint64_t HardwareBreakpointManager::encodeDr7(int slot, Type type, Size size, bool enabled) const {
    uint64_t dr7 = 0;

    if (enabled) {
        // Local and Global enable bits
        dr7 |= (0x3ULL << (slot * 2));

        // RW (read/write) bits at position 16 + slot * 4
        dr7 |= (static_cast<uint64_t>(type) << (16 + slot * 4));

        // LEN (length) bits at position 18 + slot * 4
        dr7 |= (static_cast<uint64_t>(size) << (18 + slot * 4));
    }

    // Set LE (Local Exact) bit - required for some processors
    dr7 |= (1ULL << 8);

    // Set GE (Global Exact) bit
    dr7 |= (1ULL << 9);

    return dr7;
}

void HardwareBreakpointManager::decodeDr7(uint64_t dr7, int slot, bool& enabled, Type& type, Size& size) const {
    enabled = (dr7 & (0x3ULL << (slot * 2))) != 0;
    type = static_cast<Type>((dr7 >> (16 + slot * 4)) & 0x3);
    size = static_cast<Size>((dr7 >> (18 + slot * 4)) & 0x3);
}

}

#include "HardwareBreakpointManager.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <cstring>
#include <cerrno>

namespace X64DbgLinux {

// Offsets for debug registers in struct user (x86_64)
// These are the byte offsets in the user area for each debug register
static constexpr size_t DR0_OFFSET = offsetof(struct user, u_debugreg[0]);
static constexpr size_t DR1_OFFSET = offsetof(struct user, u_debugreg[1]);
static constexpr size_t DR2_OFFSET = offsetof(struct user, u_debugreg[2]);
static constexpr size_t DR3_OFFSET = offsetof(struct user, u_debugreg[3]);
static constexpr size_t DR6_OFFSET = offsetof(struct user, u_debugreg[6]);
static constexpr size_t DR7_OFFSET = offsetof(struct user, u_debugreg[7]);

void HardwareBreakpointManager::setTarget(pid_t pid) {
    m_targetPid = pid;
}

bool HardwareBreakpointManager::setBreakpoint(int slot, uint64_t addr, Type type, Size size) {
    if (slot < 0 || slot >= MAX_HW_BREAKPOINTS) {
        return false;
    }

    if (m_targetPid == 0) {
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

    if (m_targetPid == 0) {
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

    if (m_targetPid == 0) {
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

    if (m_targetPid == 0) {
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
    if (m_targetPid != 0) {
        DebugRegisters regs;
        if (readDebugRegisters(regs)) {
            regs.dr0 = regs.dr1 = regs.dr2 = regs.dr3 = 0;
            regs.dr7 = 0;
            writeDebugRegisters(regs);
        }
    }

    for (int i = 0; i < MAX_HW_BREAKPOINTS; i++) {
        m_breakpoints[i] = 0;
        m_enabled[i] = false;
    }
}

bool HardwareBreakpointManager::readDebugRegisters(DebugRegisters& regs) const {
    if (m_targetPid == 0) {
        return false;
    }

    // Use PTRACE_PEEKUSER to read debug registers
    errno = 0;
    regs.dr0 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR0_OFFSET, nullptr);
    if (errno != 0) return false;

    errno = 0;
    regs.dr1 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR1_OFFSET, nullptr);
    if (errno != 0) return false;

    errno = 0;
    regs.dr2 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR2_OFFSET, nullptr);
    if (errno != 0) return false;

    errno = 0;
    regs.dr3 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR3_OFFSET, nullptr);
    if (errno != 0) return false;

    errno = 0;
    regs.dr6 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR6_OFFSET, nullptr);
    if (errno != 0) return false;

    errno = 0;
    regs.dr7 = ptrace(PTRACE_PEEKUSER, m_targetPid, DR7_OFFSET, nullptr);
    if (errno != 0) return false;

    return true;
}

bool HardwareBreakpointManager::writeDebugRegisters(const DebugRegisters& regs) const {
    if (m_targetPid == 0) {
        return false;
    }

    // Use PTRACE_POKEUSER to write debug registers
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR0_OFFSET, regs.dr0) == -1) {
        return false;
    }
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR1_OFFSET, regs.dr1) == -1) {
        return false;
    }
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR2_OFFSET, regs.dr2) == -1) {
        return false;
    }
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR3_OFFSET, regs.dr3) == -1) {
        return false;
    }
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR6_OFFSET, regs.dr6) == -1) {
        return false;
    }
    if (ptrace(PTRACE_POKEUSER, m_targetPid, DR7_OFFSET, regs.dr7) == -1) {
        return false;
    }

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

#include "MemoryBreakpointManager.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <optional>

namespace X64DbgLinux {

bool MemoryBreakpointManager::setMemoryBreakpoint(uint64_t addr, size_t size, MemoryBreakpointType type) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if breakpoint already exists
    if (m_breakpoints.count(addr)) {
        return false;
    }

    // Get current protection
    int originalProt;
    if (!getMemoryProtection(addr, originalProt)) {
        return false;
    }

    // Calculate protection flags
    int prot = getProtectionFlags(type);

    // Align address to page boundary
    uint64_t pageSize = sysconf(_SC_PAGESIZE);
    uint64_t alignedAddr = addr & ~(pageSize - 1);
    size_t alignedSize = ((addr + size - alignedAddr + pageSize - 1) / pageSize) * pageSize;

    // Set new protection
    if (!setMemoryProtection(alignedAddr, alignedSize, prot)) {
        return false;
    }

    // Store breakpoint info
    MemoryBreakpoint bp;
    bp.address = addr;
    bp.size = size;
    bp.type = type;
    bp.originalProtection = originalProt;
    bp.enabled = true;
    m_breakpoints[addr] = bp;

    return true;
}

bool MemoryBreakpointManager::removeMemoryBreakpoint(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(addr);
    if (it == m_breakpoints.end()) {
        return false;
    }

    // Restore original protection
    uint64_t pageSize = sysconf(_SC_PAGESIZE);
    uint64_t alignedAddr = addr & ~(pageSize - 1);
    size_t alignedSize = ((addr + it->second.size - alignedAddr + pageSize - 1) / pageSize) * pageSize;

    setMemoryProtection(alignedAddr, alignedSize, it->second.originalProtection);

    m_breakpoints.erase(it);
    return true;
}

bool MemoryBreakpointManager::handleMemoryProtectionFault(uint64_t addr, bool isWrite) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto bp = findBreakpoint(addr);
    if (!bp.has_value()) {
        return false;
    }

    // Check if the access type matches the breakpoint type
    switch (bp->type) {
        case MemoryBreakpointType::Access:
            return true; // Any access triggers
        case MemoryBreakpointType::Read:
            return !isWrite; // Only read triggers
        case MemoryBreakpointType::Write:
            return isWrite; // Only write triggers
        case MemoryBreakpointType::Execute:
            return false; // Execution breakpoints handled differently
    }

    return false;
}

bool MemoryBreakpointManager::enableMemoryBreakpoint(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(addr);
    if (it == m_breakpoints.end()) {
        return false;
    }

    if (it->second.enabled) {
        return true; // Already enabled
    }

    int prot = getProtectionFlags(it->second.type);
    uint64_t pageSize = sysconf(_SC_PAGESIZE);
    uint64_t alignedAddr = addr & ~(pageSize - 1);
    size_t alignedSize = ((addr + it->second.size - alignedAddr + pageSize - 1) / pageSize) * pageSize;

    if (!setMemoryProtection(alignedAddr, alignedSize, prot)) {
        return false;
    }

    it->second.enabled = true;
    return true;
}

bool MemoryBreakpointManager::disableMemoryBreakpoint(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_breakpoints.find(addr);
    if (it == m_breakpoints.end()) {
        return false;
    }

    if (!it->second.enabled) {
        return true; // Already disabled
    }

    uint64_t pageSize = sysconf(_SC_PAGESIZE);
    uint64_t alignedAddr = addr & ~(pageSize - 1);
    size_t alignedSize = ((addr + it->second.size - alignedAddr + pageSize - 1) / pageSize) * pageSize;

    if (!setMemoryProtection(alignedAddr, alignedSize, it->second.originalProtection)) {
        return false;
    }

    it->second.enabled = false;
    return true;
}

bool MemoryBreakpointManager::hasMemoryBreakpoint(uint64_t addr) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_breakpoints.count(addr) > 0;
}

std::vector<MemoryBreakpoint> MemoryBreakpointManager::getAllBreakpoints() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<MemoryBreakpoint> result;
    for (const auto& [addr, bp] : m_breakpoints) {
        result.push_back(bp);
    }
    return result;
}

void MemoryBreakpointManager::clearAllBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [addr, bp] : m_breakpoints) {
        uint64_t pageSize = sysconf(_SC_PAGESIZE);
        uint64_t alignedAddr = addr & ~(pageSize - 1);
        size_t alignedSize = ((addr + bp.size - alignedAddr + pageSize - 1) / pageSize) * pageSize;
        setMemoryProtection(alignedAddr, alignedSize, bp.originalProtection);
    }
    m_breakpoints.clear();
}

int MemoryBreakpointManager::getProtectionFlags(MemoryBreakpointType type) const {
    switch (type) {
        case MemoryBreakpointType::Access:
            return PROT_NONE; // No access allowed
        case MemoryBreakpointType::Read:
            return PROT_READ; // Only read allowed
        case MemoryBreakpointType::Write:
            return PROT_READ | PROT_WRITE; // Read+Write allowed, but we trap writes
        case MemoryBreakpointType::Execute:
            return PROT_READ | PROT_EXEC; // Read+Exec allowed
        default:
            return PROT_READ | PROT_WRITE | PROT_EXEC;
    }
}

bool MemoryBreakpointManager::setMemoryProtection(uint64_t addr, size_t size, int prot) {
    if (mprotect(reinterpret_cast<void*>(addr), size, prot) == -1) {
        return false;
    }
    return true;
}

bool MemoryBreakpointManager::getMemoryProtection(uint64_t addr, int& prot) const {
    // Read /proc/self/maps to get protection
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(maps, line)) {
        std::istringstream iss(line);
        std::string range, perms;
        iss >> range >> perms;

        // Parse range
        size_t dashPos = range.find('-');
        if (dashPos == std::string::npos) continue;

        uint64_t start = std::stoull(range.substr(0, dashPos), nullptr, 16);
        uint64_t end = std::stoull(range.substr(dashPos + 1), nullptr, 16);

        if (addr >= start && addr < end) {
            // Convert perms to mprotect flags
            prot = 0;
            if (perms[0] == 'r') prot |= PROT_READ;
            if (perms[1] == 'w') prot |= PROT_WRITE;
            if (perms[2] == 'x') prot |= PROT_EXEC;
            return true;
        }
    }

    return false;
}

std::optional<MemoryBreakpoint> MemoryBreakpointManager::findBreakpoint(uint64_t addr) const {
    for (const auto& [bpAddr, bp] : m_breakpoints) {
        if (addr >= bp.address && addr < bp.address + bp.size) {
            return bp;
        }
    }
    return std::nullopt;
}

}

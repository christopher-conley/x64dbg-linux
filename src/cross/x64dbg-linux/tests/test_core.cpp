#include <iostream>
#include "LinuxThreadManager.h"
#include "ExceptionHandler.h"
#include "MemoryBreakpointManager.h"
#include "HardwareBreakpointManager.h"
#include "SymbolProvider.h"

using namespace X64DbgLinux;

int main() {
    std::cout << "x64dbg-linux Phase 2 Core Functionality Test" << std::endl;
    std::cout << "=============================================" << std::endl;

    // Test ThreadManager instantiation
    std::cout << "\n1. Testing ThreadManager..." << std::endl;
    ThreadManager threadMgr;
    std::cout << "   ThreadManager created successfully" << std::endl;

    // Test ExceptionHandler
    std::cout << "\n2. Testing ExceptionHandler..." << std::endl;
    ExceptionHandler excHandler;
    std::cout << "   ExceptionHandler created successfully" << std::endl;

    // Test MemoryBreakpointManager
    std::cout << "\n3. Testing MemoryBreakpointManager..." << std::endl;
    MemoryBreakpointManager memBpMgr;
    std::cout << "   MemoryBreakpointManager created successfully" << std::endl;

    // Test HardwareBreakpointManager
    std::cout << "\n4. Testing HardwareBreakpointManager..." << std::endl;
    HardwareBreakpointManager hwBpMgr;
    std::cout << "   HardwareBreakpointManager created successfully" << std::endl;

    // Test SymbolProvider
    std::cout << "\n5. Testing SymbolProvider..." << std::endl;
    SymbolProvider symProvider;
    std::cout << "   SymbolProvider created successfully" << std::endl;

    std::cout << "\n=============================================" << std::endl;
    std::cout << "All Phase 2 core components compiled and instantiated successfully!" << std::endl;

    return 0;
}

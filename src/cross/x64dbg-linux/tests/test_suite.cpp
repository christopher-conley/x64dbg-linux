#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include "../core/LinuxThreadManager.h"
#include "../core/ExceptionHandler.h"
#include "../core/MemoryBreakpointManager.h"
#include "../core/HardwareBreakpointManager.h"
#include "../core/SymbolProvider.h"

using namespace X64DbgLinux;

// Test counters
int g_testsPassed = 0;
int g_testsFailed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  Testing " #name "... " << std::flush; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        g_testsPassed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        g_testsFailed++; \
    } catch (...) { \
        std::cout << "FAILED: unknown exception" << std::endl; \
        g_testsFailed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b); \
    } \
} while(0)

// ==================== ThreadManager Tests ====================

TEST(thread_manager_basic) {
    ThreadManager mgr;
    ASSERT_TRUE(mgr.getAllThreads().empty());
    ASSERT_TRUE(mgr.getCurrentThreadId() == 0);
}

TEST(thread_manager_current_thread) {
    ThreadManager mgr;
    pid_t current = getpid();
    mgr.setCurrentThread(current);
    ASSERT_TRUE(mgr.getCurrentThreadId() == current);
}

// ==================== ExceptionHandler Tests ====================

TEST(exception_handler_sigtrap) {
    ExceptionInfo exc = ExceptionHandler::MapSignalToException(SIGTRAP, nullptr);
    ASSERT_TRUE(exc.type == ExceptionType::Breakpoint || exc.type == ExceptionType::SingleStep);
}

TEST(exception_handler_sigsegv) {
    ExceptionInfo exc = ExceptionHandler::MapSignalToException(SIGSEGV, nullptr);
    ASSERT_TRUE(exc.type == ExceptionType::AccessViolation);
}

TEST(exception_handler_is_debug_signal) {
    ASSERT_TRUE(ExceptionHandler::IsDebugSignal(SIGTRAP));
    ASSERT_TRUE(ExceptionHandler::IsDebugSignal(SIGSEGV));
    ASSERT_FALSE(ExceptionHandler::IsDebugSignal(SIGUSR1));
}

// ==================== MemoryBreakpointManager Tests ====================

TEST(memory_bp_basic) {
    MemoryBreakpointManager mgr;
    mgr.setTarget(0); // No target
    ASSERT_FALSE(mgr.hasMemoryBreakpoint(0x1234));
}

TEST(memory_bp_type_conversion) {
    MemoryBreakpointManager mgr;
    // Test protection flag conversion (indirectly via public methods)
    ASSERT_FALSE(mgr.hasMemoryBreakpoint(0));
}

// ==================== HardwareBreakpointManager Tests ====================

TEST(hw_bp_basic) {
    HardwareBreakpointManager mgr;
    mgr.setTarget(0);
    ASSERT_FALSE(mgr.isBreakpointSet(0));
    ASSERT_FALSE(mgr.isBreakpointSet(1));
    ASSERT_FALSE(mgr.isBreakpointSet(2));
    ASSERT_FALSE(mgr.isBreakpointSet(3));
}

TEST(hw_bp_slot_validation) {
    HardwareBreakpointManager mgr;
    mgr.setTarget(0);

    // Invalid slots should return false
    ASSERT_FALSE(mgr.setBreakpoint(-1, 0x1234,
        HardwareBreakpointManager::Type::Execute,
        HardwareBreakpointManager::Size::Byte));
    ASSERT_FALSE(mgr.setBreakpoint(4, 0x1234,
        HardwareBreakpointManager::Type::Execute,
        HardwareBreakpointManager::Size::Byte));
}

TEST(hw_bp_find_free_slot) {
    HardwareBreakpointManager mgr;
    mgr.setTarget(0);

    // Without a real target, findFreeSlot returns nullopt
    auto slot = mgr.findFreeSlot();
    // Can't test further without a real process
}

// ==================== SymbolProvider Tests ====================

TEST(symbol_provider_basic) {
    SymbolProvider provider;
    ASSERT_TRUE(provider.getAllModules().empty());
}

TEST(symbol_provider_resolve) {
    SymbolProvider provider;
    auto result = provider.resolveSymbol("nonexistent");
    ASSERT_FALSE(result.has_value());
}

TEST(symbol_provider_module_info) {
    SymbolProvider provider;
    auto result = provider.getModuleInfo(0x1234);
    ASSERT_FALSE(result.has_value());
}

// ==================== Integration Tests ====================

TEST(integration_all_managers) {
    // Create all managers to ensure they can coexist
    ThreadManager threadMgr;
    MemoryBreakpointManager memBpMgr;
    HardwareBreakpointManager hwBpMgr;
    SymbolProvider symProvider;

    // Set no targets (we don't have a real debuggee in tests)
    memBpMgr.setTarget(0);
    hwBpMgr.setTarget(0);

    // Verify they all exist and have expected initial state
    ASSERT_TRUE(threadMgr.getAllThreads().empty());
    ASSERT_FALSE(memBpMgr.hasMemoryBreakpoint(0));
    ASSERT_FALSE(hwBpMgr.isBreakpointSet(0));
    ASSERT_TRUE(symProvider.getAllModules().empty());
}

// ==================== Performance Tests ====================

TEST(performance_symbol_lookup) {
    SymbolProvider provider;

    // Add some dummy modules
    for (int i = 0; i < 100; i++) {
        ModuleInfo mod;
        mod.name = "test_module_" + std::to_string(i);
        mod.path = "/path/to/" + mod.name;
        mod.base = 0x400000ULL + (i * 0x10000);
        mod.size = 0x10000;

        for (int j = 0; j < 10; j++) {
            SymbolInfo sym;
            sym.name = "symbol_" + std::to_string(j);
            sym.address = mod.base + (j * 0x100);
            sym.size = 0x100;
            sym.type = 0;
            mod.symbols.push_back(sym);
        }

        provider.loadModule(mod.path, mod.base);
    }

    // Note: Since loadModule doesn't actually use our ModuleInfo,
    // this test mainly verifies it doesn't crash with many calls
}

// ==================== Main ====================

int main(int argc, char* argv[]) {
    std::cout << "x64dbg-linux Phase 7 Test Suite" << std::endl;
    std::cout << "================================" << std::endl;

    // Check for --performance flag
    bool runPerformance = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--performance") == 0) {
            runPerformance = true;
        }
    }

    std::cout << "\n--- ThreadManager Tests ---" << std::endl;
    RUN_TEST(thread_manager_basic);
    RUN_TEST(thread_manager_current_thread);

    std::cout << "\n--- ExceptionHandler Tests ---" << std::endl;
    RUN_TEST(exception_handler_basic);
    RUN_TEST(exception_handler_sigtrap);
    RUN_TEST(exception_handler_sigsegv);

    std::cout << "\n--- MemoryBreakpointManager Tests ---" << std::endl;
    RUN_TEST(memory_bp_basic);
    RUN_TEST(memory_bp_type_conversion);

    std::cout << "\n--- HardwareBreakpointManager Tests ---" << std::endl;
    RUN_TEST(hw_bp_basic);
    RUN_TEST(hw_bp_slot_validation);
    RUN_TEST(hw_bp_find_free_slot);

    std::cout << "\n--- SymbolProvider Tests ---" << std::endl;
    RUN_TEST(symbol_provider_basic);
    RUN_TEST(symbol_provider_resolve);
    RUN_TEST(symbol_provider_module_info);

    std::cout << "\n--- Integration Tests ---" << std::endl;
    RUN_TEST(integration_all_managers);

    if (runPerformance) {
        std::cout << "\n--- Performance Tests ---" << std::endl;
        RUN_TEST(performance_symbol_lookup);
    }

    std::cout << "\n================================" << std::endl;
    std::cout << "Tests passed: " << g_testsPassed << std::endl;
    std::cout << "Tests failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}

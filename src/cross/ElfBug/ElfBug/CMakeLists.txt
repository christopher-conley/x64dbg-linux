add_library(ElfBug STATIC
    core/Debugger.cpp
    core/Debugger.Loop.cpp
    core/Debugger.Loop.Signal.cpp
    core/Debugger.Loop.Process.cpp
    core/Debugger.Loop.Thread.cpp
    process/Process.cpp
    process/Memory.cpp
    process/Breakpoint.cpp
    thread/Thread.cpp
    thread/Registers.cpp
    thread/HardwareBreakpoint.cpp
    api/elfbug_api.cpp
    api/elfbug_api.h
)

target_include_directories(ElfBug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

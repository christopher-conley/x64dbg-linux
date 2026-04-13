cmake_minimum_required(VERSION 3.19)
project(x64dbg LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(CROSS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(widgets_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../gui/Src")
# Zydis + md4c
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zydis_wrapper
    ${CMAKE_CURRENT_BINARY_DIR}/zydis_wrapper
)
add_subdirectory(
    ${widgets_SOURCE_DIR}/ThirdPartyLibs/md4c
    ${CMAKE_CURRENT_BINARY_DIR}/md4c
)

include(${CROSS_DIR}/widgets/Qt.cmake)
include_directories(BEFORE ${CROSS_DIR}/widgets)

set(hook_SOURCES
    ${CROSS_DIR}/widgets/Bridge.cpp
    ${CROSS_DIR}/widgets/Bridge.h
    ${CROSS_DIR}/widgets/Configuration.cpp
    ${CROSS_DIR}/widgets/Configuration.h
    ${CROSS_DIR}/widgets/MagicMenu.h
    ${CROSS_DIR}/widgets/MiscUtil.cpp
    ${CROSS_DIR}/widgets/MiscUtil.h
    ${CROSS_DIR}/widgets/StringUtil.cpp
    ${CROSS_DIR}/widgets/StringUtil.h
    ${CROSS_DIR}/widgets/Types.h
    ${CROSS_DIR}/widgets/Imports.h
    ${CROSS_DIR}/widgets/RegisterContext.h
)

set(widgets_SOURCES
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.h
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.h
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.cpp
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.h
    ${widgets_SOURCE_DIR}/BasicView/HexDump.cpp
    ${widgets_SOURCE_DIR}/BasicView/HexDump.h
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.cpp
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.h
    ${widgets_SOURCE_DIR}/BasicView/StdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/StdTable.h
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.cpp
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.h
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.cpp
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.h
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.cpp
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.h
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.h
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.ui
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.cpp
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.ui
    ${widgets_SOURCE_DIR}/Gui/TypeWidget.cpp
    ${widgets_SOURCE_DIR}/Gui/TypeWidget.h
    ${widgets_SOURCE_DIR}/Gui/RichTextItemDelegate.cpp
    ${widgets_SOURCE_DIR}/Gui/RichTextItemDelegate.h
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.h
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ImageTextBrowser.cpp
    ${widgets_SOURCE_DIR}/Gui/ImageTextBrowser.h
    ${widgets_SOURCE_DIR}/Gui/RegistersView.cpp
    ${widgets_SOURCE_DIR}/Gui/RegistersView.h
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.cpp
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.h
    ${widgets_SOURCE_DIR}/Utils/ActionHelpers.h
    ${widgets_SOURCE_DIR}/Utils/CachedFontMetrics.h
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.cpp
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.h
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.cpp
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.h
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.cpp
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.h
    ${widgets_SOURCE_DIR}/Utils/MethodInvoker.h
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.cpp
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.h
    ${widgets_SOURCE_DIR}/Utils/VaHistory.h
)

add_library(x64dbg_widgets STATIC
    ${hook_SOURCES}
    ${widgets_SOURCES}
    ${CROSS_DIR}/widgets/resources.qrc
)

target_include_directories(x64dbg_widgets PUBLIC
    ${CROSS_DIR}/widgets
    ${widgets_SOURCE_DIR}
)

target_compile_definitions(x64dbg_widgets PRIVATE QT_NO_DEPRECATED_WARNINGS)

target_link_libraries(x64dbg_widgets PUBLIC
    ${QT_LIBRARIES}
    zydis_wrapper
    md4c-html
)

add_executable(x64dbg
    main.cpp
    core/DbgAdapter.cpp
    core/DbgAdapter.h
    core/ElfBugLoader.h
    core/LinuxArchitecture.h
    gui/MainWindow.cpp
    gui/MainWindow.h
    resources/icons.qrc
)

target_include_directories(x64dbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CROSS_DIR}/widgets
    ${widgets_SOURCE_DIR}
    ${widgets_SOURCE_DIR}/Gui
)

target_link_libraries(x64dbg PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    x64dbg_widgets
    ${CMAKE_DL_LIBS}
)

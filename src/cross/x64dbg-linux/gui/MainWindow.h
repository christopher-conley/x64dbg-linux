#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTextBrowser>
#include <BasicView/Disassembly.h>
#include <BasicView/HexDump.h>
#include "core/DbgAdapter.h"
#include "Gui/RegistersView.h"

// Forward declarations for Phase 3/6 views
namespace X64DbgLinux {
    class ThreadsView;
    class BreakpointsView;
    class SymbolsView;
    class MemoryMapView;
    class CallStackView;
}

class QThread;
class CPUStack;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    static void loadTheme();

private slots:
    void onOpen();
    void onContinue() const;
    void onPause() const;
    void onStepInto() const;
    void onStepOver() const;
    void onToggleBreakpoint() const;
    void onProcessCreated(duint entryPoint) const;
    void onProcessExited(int exitCode) const;
    void onStopped(duint rip, const QString & reason) const;
    void onLogMessage(const QString & msg) const;

private:
    void stopDebugThread();
    void setupToolBar();
    void setupTabs();
    QWidget* createCpuTab();

    DbgAdapter* mProvider = nullptr;
    QThread* mDebugThread = nullptr;
    QTabWidget* mTabWidget = nullptr;
    Disassembly* mDisassembly = nullptr;
    HexDump* mHexDump = nullptr;
    CPUStack* mStack = nullptr;
    RegistersView* mRegisters = nullptr;
    QTextBrowser* mLog = nullptr;

    // Phase 3 GUI components
    X64DbgLinux::ThreadsView* mThreadsView = nullptr;
    X64DbgLinux::BreakpointsView* mBreakpointsView = nullptr;
    X64DbgLinux::SymbolsView* mSymbolsView = nullptr;

    // Phase 6 GUI components
    X64DbgLinux::MemoryMapView* mMemoryMapView = nullptr;
    X64DbgLinux::CallStackView* mCallStackView = nullptr;
};

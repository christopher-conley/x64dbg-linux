#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTextBrowser>
#include <BasicView/HexDump.h>
#include <BasicView/Disassembly.h>
#include "core/DbgAdapter.h"
#include "RegistersView.h"

class QThread;

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
    RegistersView* mRegisters = nullptr;
    QTextBrowser* mLog = nullptr;
};

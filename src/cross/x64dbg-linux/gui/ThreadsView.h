#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include "core/LinuxThreadManager.h"

namespace X64DbgLinux {

class ThreadsView : public QTableView
{
    Q_OBJECT

public:
    explicit ThreadsView(QWidget* parent = nullptr);
    ~ThreadsView() override;

    // Set the thread manager to display threads from
    void setThreadManager(ThreadManager* manager);

    // Refresh the thread list display
    void refresh();

    // Get the currently selected thread ID
    pid_t selectedThreadId() const;

    // Select a specific thread by ID
    void selectThread(pid_t tid);

signals:
    void threadSelected(pid_t tid);
    void threadSuspended(pid_t tid);
    void threadResumed(pid_t tid);
    void contextMenuRequestedForThread(pid_t tid, const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupColumns();
    void createContextMenu();
    void onSuspendThread();
    void onResumeThread();
    void onSwitchToThread();

    ThreadManager* m_threadManager = nullptr;
    QStandardItemModel* m_model = nullptr;
    QMenu* m_contextMenu = nullptr;
    QAction* m_suspendAction = nullptr;
    QAction* m_resumeAction = nullptr;
    QAction* m_switchAction = nullptr;
    pid_t m_contextMenuTid = 0;
};

} // namespace X64DbgLinux

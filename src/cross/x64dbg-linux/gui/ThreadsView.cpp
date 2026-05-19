#include "gui/ThreadsView.h"
#include <QDebug>

namespace X64DbgLinux {

ThreadsView::ThreadsView(QWidget* parent)
    : QTableView(parent)
    , m_model(new QStandardItemModel(this))
{
    setModel(m_model);
    setupColumns();
    createContextMenu();

    // Visual settings
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

ThreadsView::~ThreadsView() = default;

void ThreadsView::setupColumns()
{
    m_model->setColumnCount(4);
    m_model->setHorizontalHeaderLabels({
        tr("TID"),
        tr("Name"),
        tr("Status"),
        tr("Start Address")
    });

    // Set column widths
    setColumnWidth(0, 80);   // TID
    setColumnWidth(1, 150);  // Name
    setColumnWidth(2, 80);   // Status
    setColumnWidth(3, 150);  // Start Address
}

void ThreadsView::createContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_suspendAction = new QAction(tr("Suspend"), this);
    m_resumeAction = new QAction(tr("Resume"), this);
    m_switchAction = new QAction(tr("Switch to Thread"), this);

    connect(m_suspendAction, &QAction::triggered, this, &ThreadsView::onSuspendThread);
    connect(m_resumeAction, &QAction::triggered, this, &ThreadsView::onResumeThread);
    connect(m_switchAction, &QAction::triggered, this, &ThreadsView::onSwitchToThread);

    m_contextMenu->addAction(m_suspendAction);
    m_contextMenu->addAction(m_resumeAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_switchAction);
}

void ThreadsView::setThreadManager(ThreadManager* manager)
{
    m_threadManager = manager;
    refresh();
}

void ThreadsView::refresh()
{
    m_model->removeRows(0, m_model->rowCount());

    if (!m_threadManager) {
        return;
    }

    const auto threads = m_threadManager->getAllThreads();
    for (const auto& thread : threads) {
        const int row = m_model->rowCount();
        m_model->insertRow(row);

        // TID
        auto* tidItem = new QStandardItem(QString::number(thread.tid));
        tidItem->setData(static_cast<qulonglong>(thread.tid), Qt::UserRole);
        m_model->setItem(row, 0, tidItem);

        // Name
        m_model->setItem(row, 1, new QStandardItem(QString::fromStdString(thread.name)));

        // Status
        const QString status = thread.suspended ? tr("Suspended") : tr("Running");
        auto* statusItem = new QStandardItem(status);
        if (thread.suspended) {
            statusItem->setForeground(Qt::red);
        } else {
            statusItem->setForeground(Qt::green);
        }
        m_model->setItem(row, 2, statusItem);

        // Start Address
        const QString addrStr = QString("0x%1").arg(thread.startAddress, 16, 16, QChar('0'));
        m_model->setItem(row, 3, new QStandardItem(addrStr));
    }
}

pid_t ThreadsView::selectedThreadId() const
{
    const QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return 0;
    }

    const QModelIndex index = selected.first();
    const QVariant data = m_model->item(index.row(), 0)->data(Qt::UserRole);
    return static_cast<pid_t>(data.toULongLong());
}

void ThreadsView::selectThread(pid_t tid)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (static_cast<pid_t>(data.toULongLong()) == tid) {
            selectRow(row);
            return;
        }
    }
}

void ThreadsView::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    m_contextMenuTid = static_cast<pid_t>(m_model->item(index.row(), 0)->data(Qt::UserRole).toULongLong());

    // Update menu items based on thread state
    if (m_threadManager) {
        const auto threads = m_threadManager->getAllThreads();
        for (const auto& thread : threads) {
            if (thread.tid == m_contextMenuTid) {
                m_suspendAction->setEnabled(!thread.suspended);
                m_resumeAction->setEnabled(thread.suspended);
                break;
            }
        }
    }

    emit contextMenuRequestedForThread(m_contextMenuTid, event->globalPos());
    m_contextMenu->exec(event->globalPos());
}

void ThreadsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        const pid_t tid = static_cast<pid_t>(m_model->item(index.row(), 0)->data(Qt::UserRole).toULongLong());
        emit threadSelected(tid);
    }
    QTableView::mouseDoubleClickEvent(event);
}

void ThreadsView::onSuspendThread()
{
    if (m_threadManager && m_contextMenuTid != 0) {
        if (m_threadManager->suspendThread(m_contextMenuTid)) {
            emit threadSuspended(m_contextMenuTid);
            refresh();
        }
    }
}

void ThreadsView::onResumeThread()
{
    if (m_threadManager && m_contextMenuTid != 0) {
        if (m_threadManager->resumeThread(m_contextMenuTid)) {
            emit threadResumed(m_contextMenuTid);
            refresh();
        }
    }
}

void ThreadsView::onSwitchToThread()
{
    if (m_threadManager && m_contextMenuTid != 0) {
        m_threadManager->setCurrentThread(m_contextMenuTid);
        emit threadSelected(m_contextMenuTid);
    }
}

} // namespace X64DbgLinux

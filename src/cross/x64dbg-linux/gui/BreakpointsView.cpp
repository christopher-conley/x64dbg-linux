#include "gui/BreakpointsView.h"
#include <QDebug>

namespace X64DbgLinux {

BreakpointsView::BreakpointsView(QWidget* parent)
    : QTableView(parent)
    , m_model(new QStandardItemModel(this))
{
    setModel(m_model);
    setupColumns();
    createContextMenu();

    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

BreakpointsView::~BreakpointsView() = default;

void BreakpointsView::setupColumns()
{
    m_model->setColumnCount(6);
    m_model->setHorizontalHeaderLabels({
        tr("Address"),
        tr("Type"),
        tr("Enabled"),
        tr("Module"),
        tr("Label"),
        tr("Hits")
    });

    setColumnWidth(0, 120);
    setColumnWidth(1, 80);
    setColumnWidth(2, 60);
    setColumnWidth(3, 120);
    setColumnWidth(4, 150);
    setColumnWidth(5, 60);
}

void BreakpointsView::createContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_toggleAction = new QAction(tr("Toggle"), this);
    m_deleteAction = new QAction(tr("Delete"), this);
    m_gotoAction = new QAction(tr("Go to"), this);
    m_deleteAllAction = new QAction(tr("Delete All"), this);

    connect(m_toggleAction, &QAction::triggered, this, &BreakpointsView::onToggleBreakpoint);
    connect(m_deleteAction, &QAction::triggered, this, &BreakpointsView::onDeleteBreakpoint);
    connect(m_gotoAction, &QAction::triggered, this, &BreakpointsView::onGotoBreakpoint);
    connect(m_deleteAllAction, &QAction::triggered, this, &BreakpointsView::onDeleteAllBreakpoints);

    m_contextMenu->addAction(m_toggleAction);
    m_contextMenu->addAction(m_deleteAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_gotoAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_deleteAllAction);
}

void BreakpointsView::addBreakpoint(const BreakpointInfo& bp)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (data.toULongLong() == bp.address) {
            updateBreakpoint(bp.address, bp.enabled);
            return;
        }
    }

    const int row = m_model->rowCount();
    m_model->insertRow(row);

    auto* addrItem = new QStandardItem(addressToString(bp.address));
    addrItem->setData(static_cast<qulonglong>(bp.address), Qt::UserRole);
    m_model->setItem(row, 0, addrItem);

    m_model->setItem(row, 1, new QStandardItem(typeToString(bp.type)));

    auto* enabledItem = new QStandardItem(bp.enabled ? tr("Yes") : tr("No"));
    enabledItem->setForeground(bp.enabled ? Qt::darkGreen : Qt::gray);
    m_model->setItem(row, 2, enabledItem);

    m_model->setItem(row, 3, new QStandardItem(QString::fromStdString(bp.module)));
    m_model->setItem(row, 4, new QStandardItem(QString::fromStdString(bp.label)));
    m_model->setItem(row, 5, new QStandardItem(QString::number(bp.hitCount)));
}

void BreakpointsView::removeBreakpoint(uint64_t address)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (data.toULongLong() == address) {
            m_model->removeRow(row);
            emit breakpointDeleted(address);
            return;
        }
    }
}

void BreakpointsView::clearAllBreakpoints()
{
    m_model->removeRows(0, m_model->rowCount());
}

void BreakpointsView::updateBreakpoint(uint64_t address, bool enabled)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (data.toULongLong() == address) {
            auto* enabledItem = new QStandardItem(enabled ? tr("Yes") : tr("No"));
            enabledItem->setForeground(enabled ? Qt::darkGreen : Qt::gray);
            m_model->setItem(row, 2, enabledItem);
            return;
        }
    }
}

void BreakpointsView::setBreakpointHitCount(uint64_t address, size_t count)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (data.toULongLong() == address) {
            m_model->setItem(row, 5, new QStandardItem(QString::number(count)));
            return;
        }
    }
}

uint64_t BreakpointsView::selectedBreakpointAddress() const
{
    const QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return 0;
    }

    const QModelIndex index = selected.first();
    const QVariant data = m_model->item(index.row(), 0)->data(Qt::UserRole);
    return data.toULongLong();
}

void BreakpointsView::refresh()
{
    m_model->sort(0);
}

bool BreakpointsView::hasBreakpoint(uint64_t address) const
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
        if (data.toULongLong() == address) {
            return true;
        }
    }
    return false;
}

void BreakpointsView::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    m_contextMenuAddress = static_cast<uint64_t>(m_model->item(index.row(), 0)->data(Qt::UserRole).toULongLong());

    const QString enabledText = m_model->item(index.row(), 2)->text();
    m_toggleAction->setText(enabledText == tr("Yes") ? tr("Disable") : tr("Enable"));

    m_contextMenu->exec(event->globalPos());
}

void BreakpointsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        const uint64_t addr = static_cast<uint64_t>(m_model->item(index.row(), 0)->data(Qt::UserRole).toULongLong());
        emit breakpointSelected(addr);
        emit gotoBreakpointRequested(addr);
    }
    QTableView::mouseDoubleClickEvent(event);
}

void BreakpointsView::onToggleBreakpoint()
{
    if (m_contextMenuAddress != 0) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const QVariant data = m_model->item(row, 0)->data(Qt::UserRole);
            if (data.toULongLong() == m_contextMenuAddress) {
                const QString enabledText = m_model->item(row, 2)->text();
                const bool newState = (enabledText != tr("Yes"));
                updateBreakpoint(m_contextMenuAddress, newState);
                emit breakpointToggled(m_contextMenuAddress, newState);
                return;
            }
        }
    }
}

void BreakpointsView::onDeleteBreakpoint()
{
    if (m_contextMenuAddress != 0) {
        removeBreakpoint(m_contextMenuAddress);
    }
}

void BreakpointsView::onGotoBreakpoint()
{
    if (m_contextMenuAddress != 0) {
        emit gotoBreakpointRequested(m_contextMenuAddress);
    }
}

void BreakpointsView::onDeleteAllBreakpoints()
{
    clearAllBreakpoints();
    emit breakpointDeleted(0);
}

QString BreakpointsView::typeToString(BreakpointType type) const
{
    switch (type) {
        case BreakpointType::Software: return tr("Software");
        case BreakpointType::Hardware: return tr("Hardware");
        case BreakpointType::Memory: return tr("Memory");
        default: return tr("Unknown");
    }
}

QString BreakpointsView::addressToString(uint64_t addr) const
{
    return QString("0x%1").arg(addr, 16, 16, QChar('0'));
}

} // namespace X64DbgLinux

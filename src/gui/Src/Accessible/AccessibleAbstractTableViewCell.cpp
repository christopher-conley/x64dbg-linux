// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableViewCell.h"
#include "AccessibleAbstractTableView.h"

AccessibleAbstractTableViewCell::AccessibleAbstractTableViewCell(AccessibleAbstractTableView* parent, duint row, int column) : mParent(parent), row(row), column(column)
{
}

QString AccessibleAbstractTableViewCell::text(QAccessible::Text t) const
{
    AbstractTableView* w = mParent->m_tableView;
    switch(t)
    {
    case QAccessible::Name:
        return QString("Row %1 Column %2").arg(row).arg(column);
    case QAccessible::Value:
        return mParent->getCellContent(row, column);
    default:
        return QString();
    }
}

QColor AccessibleAbstractTableViewCell::foregroundColor() const
{
    return mParent->m_tableView->mTextColor;
}

int AccessibleAbstractTableViewCell::childCount() const
{
    return 0;
}

QWindow* AccessibleAbstractTableViewCell::window() const
{
    return mParent->window();
}

QAccessibleInterface* AccessibleAbstractTableViewCell::parent() const
{
    return dynamic_cast<QAccessibleInterface*>(mParent);
}

QAccessibleInterface* AccessibleAbstractTableViewCell::child(int index) const
{
    return nullptr;
}

int AccessibleAbstractTableViewCell::indexOfChild(const QAccessibleInterface* child) const
{
    return -1;
}

QAccessible::Role AccessibleAbstractTableViewCell::role() const
{
    return QAccessible::Cell;
}

QAccessible::State AccessibleAbstractTableViewCell::state() const
{
    QAccessible::State state;
    state.focusable = mParent->m_tableView->getRowCount() > 0;
    state.active = state.focusable;
    state.selectable = state.focusable;
    state.selected = mParent->isRowSelected(row);
    state.focused = state.selected && mParent->isColumnSelected(column);
    state.readOnly = true;
    return state;
}

QAccessibleInterface* AccessibleAbstractTableViewCell::childAt(int x, int y) const
{
    return nullptr;
}

QObject* AccessibleAbstractTableViewCell::object() const
{
    return nullptr;
}

void AccessibleAbstractTableViewCell::setText(QAccessible::Text t, const QString & text)
{
}

QRect AccessibleAbstractTableViewCell::rect() const
{
    const auto & table = mParent->m_tableView;
    int height = table->getRowHeight();
    auto & actualColumn = table->mColumnOrder[column];
    QPoint pos(table->getColumnPosition(column), table->getHeaderHeight() + row * height);
    pos = table->mapToGlobal(pos);
    return QRect(pos, QSize(table->getColumnWidth(actualColumn), height));
}

bool AccessibleAbstractTableViewCell::isValid() const
{
    return true;
}

void* AccessibleAbstractTableViewCell::interface_cast(QAccessible::InterfaceType type)
{
    if(type == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    else
        return nullptr;
}

bool AccessibleAbstractTableViewCell::isSelected() const
{
    return false;
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCell::columnHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCell::rowHeaderCells() const
{
    return QList<QAccessibleInterface*>({ mParent->cellAt(row, 0) });
}

int AccessibleAbstractTableViewCell::columnIndex() const
{
    return column;
}

int AccessibleAbstractTableViewCell::rowIndex() const
{
    return row;
}

int AccessibleAbstractTableViewCell::columnExtent() const
{
    return 1;
}

int AccessibleAbstractTableViewCell::rowExtent() const
{
    return 1;
}

QAccessibleInterface* AccessibleAbstractTableViewCell::table() const
{
    return static_cast<QAccessibleInterface*>(mParent);
}

#endif
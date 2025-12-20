// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableViewCellTitle.h"
#include "AccessibleAbstractTableView.h"

AccessibleAbstractTableViewCellTitle::AccessibleAbstractTableViewCellTitle(AccessibleAbstractTableView* parent, int column) : AccessibleAbstractTableViewCell(parent, 0, column)
{
}

QString AccessibleAbstractTableViewCellTitle::text(QAccessible::Text t) const
{
    AbstractTableView* w = mParent->m_tableView;
    switch(t)
    {
    case QAccessible::Value:
        return w->getColTitle(column);
    default:
        return QString();
    }
}

QColor AccessibleAbstractTableViewCellTitle::foregroundColor() const
{
    return mParent->m_tableView->mHeaderTextColor;
}

QColor AccessibleAbstractTableViewCellTitle::backgroundColor() const
{
    return mParent->m_tableView->mHeaderBackgroundColor;
}

QAccessible::State AccessibleAbstractTableViewCellTitle::state() const
{
    QAccessible::State state;
    state.focusable = mParent->m_tableView->getRowCount() > 0;
    state.active = state.focusable;
    state.selectable = false;
    state.selected = false;
    state.focused = false;
    state.readOnly = true;
    return state;
}

QRect AccessibleAbstractTableViewCellTitle::rect() const
{
    const auto & table = mParent->m_tableView;
    int height = table->getHeaderHeight();
    auto & actualColumn = table->mColumnOrder[column];
    QPoint pos(table->getColumnPosition(column), 0);
    pos = table->mapToGlobal(pos);
    return QRect(pos, QSize(table->getColumnWidth(actualColumn), height));
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCellTitle::rowHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCellTitle::columnHeaderCells() const
{
    return QList<QAccessibleInterface*>({(QAccessibleInterface*)this});
}

#endif
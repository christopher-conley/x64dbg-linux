// This file implements accessibility interface for title row of AbstractTableView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableViewCellTitle.h"
#include "AccessibleAbstractTableView.h"

AccessibleAbstractTableViewCellTitle::AccessibleAbstractTableViewCellTitle(AccessibleAbstractTableView* parent, int column) : AccessibleAbstractTableViewCell(parent, 0, column)
{
}

QString AccessibleAbstractTableViewCellTitle::text(QAccessible::Text t) const
{
    AbstractTableView* w = mParent->getTable();
    if(t == QAccessible::Value)
    {
        return w->getColTitle(mParent->logicalColumn(column));
    }
    return QString();
}

QColor AccessibleAbstractTableViewCellTitle::foregroundColor() const
{
    return mParent->getTable()->mHeaderTextColor;
}

QColor AccessibleAbstractTableViewCellTitle::backgroundColor() const
{
    return mParent->getTable()->mHeaderBackgroundColor;
}

QAccessible::State AccessibleAbstractTableViewCellTitle::state() const
{
    QAccessible::State state;
    state.focusable = mParent->getTable()->getRowCount() > 0;
    state.active = state.focusable;
    state.selectable = false;
    state.selected = false;
    state.focused = false;
    state.readOnly = true;
    state.invisible = mParent->getTable()->getHeaderHeight() == 0;
    return state;
}

QRect AccessibleAbstractTableViewCellTitle::rect() const
{
    const auto table = mParent->getTable();
    int height = table->getHeaderHeight();
    QPoint pos(table->getColumnPosition(column), 0);
    pos = table->mapToGlobal(pos);
    return QRect(pos, QSize(table->getColumnWidth(mParent->logicalColumn(column)), height));
}

QAccessible::Role AccessibleAbstractTableViewCellTitle::role() const
{
    return QAccessible::ColumnHeader;
}

int AccessibleAbstractTableViewCellTitle::rowIndex() const
{
    return 0;
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

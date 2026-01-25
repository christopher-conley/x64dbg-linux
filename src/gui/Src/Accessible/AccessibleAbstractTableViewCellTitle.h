#pragma once
#ifndef QT_NO_ACCESSIBILITY
#include <QAccessibleWidget>
#include "AccessibleAbstractTableViewCell.h"

class AccessibleAbstractTableView;

class AccessibleAbstractTableViewCellTitle : public AccessibleAbstractTableViewCell
{
public:
    AccessibleAbstractTableViewCellTitle(AccessibleAbstractTableView* parent, int column);
    // QAccessibleInterface
    QString text(QAccessible::Text t) const override;
    QColor foregroundColor() const override;
    QColor backgroundColor() const override;
    QAccessible::State state() const override;
    QList<QAccessibleInterface*> rowHeaderCells() const override;
    QList<QAccessibleInterface*> columnHeaderCells() const override;
    QRect rect() const override;
    QAccessible::Role role() const override;
    int rowIndex() const override;
};

#endif

#pragma once
#ifndef QT_NO_ACCESSIBILITY
#include <QAccessibleWidget>
#include "../BasicView/AbstractTableView.h"

class AccessibleAbstractTableView;

class AccessibleAbstractTableViewCell : public QAccessibleInterface, QAccessibleTableCellInterface
{
protected:
    duint row; // The first visible row is index 0. Off by 1 compared with AccessibleAbstractTableView row due to column titles.
    int column;
    AccessibleAbstractTableView* mParent;
public:
    AccessibleAbstractTableViewCell(AccessibleAbstractTableView* parent, duint row, int column);
    // QAccessibleInterface
    QString text(QAccessible::Text t) const override;
    QColor foregroundColor() const override;
    QWindow* window() const override;
    QAccessibleInterface* parent() const override;
    QAccessibleInterface* child(int index) const override;
    QAccessibleInterface* childAt(int x, int y) const override;
    QObject* object() const override;
    void setText(QAccessible::Text t, const QString & text) override;
    QRect rect() const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    int childCount() const override;
    bool isValid() const override;
    void* interface_cast(QAccessible::InterfaceType type) override;
    // QAccessibleTableCellInterface
    bool isSelected() const override;
    QList<QAccessibleInterface*> columnHeaderCells() const override;
    QList<QAccessibleInterface*> rowHeaderCells() const override;
    int columnIndex() const override;
    int rowIndex() const override;
    int columnExtent() const override;
    int rowExtent() const override;
    QAccessibleInterface* table() const override;
};

#endif
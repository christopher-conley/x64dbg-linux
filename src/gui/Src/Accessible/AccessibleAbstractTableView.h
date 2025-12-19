#pragma once
#ifndef QT_NO_ACCESSIBILITY
#include <QAccessibleWidget>
#include "../BasicView/AbstractTableView.h"

class AccessibleAbstractTableView;

class AccessibleAbstractTableViewCell : public QAccessibleInterface, QAccessibleTableCellInterface
{
    duint row;
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

class AccessibleAbstractTableView : public QAccessibleWidget, public QAccessibleTableInterface
{
    mutable std::vector<QAccessible::Id> cellInterfaces;
    int rows, cols;
    friend class AccessibleAbstractTableViewCell;
    AbstractTableView* m_tableView;
public:
    AccessibleAbstractTableView(QWidget* w);
    ~AccessibleAbstractTableView();
    // QAccessibleInterface
    int childCount() const override;
    QAccessibleInterface* child(int index) const override;
    int indexOfChild(const QAccessibleInterface*) const override;
    QAccessibleInterface* QAccessibleInterface::childAt(int x, int y) const override;
    QAccessibleInterface* focusChild() const override;
    bool isValid() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    void* interface_cast(QAccessible::InterfaceType type) override;
    // QAccessibleTableInterface
    QAccessibleInterface* caption() const override;
    QAccessibleInterface* cellAt(int row, int column) const override;
    int columnCount() const override;
    QString columnDescription(int column) const override;
    bool isColumnSelected(int column) const override;
    bool isRowSelected(int row) const override;
    void modelChange(QAccessibleTableModelChangeEvent* event) override;
    int rowCount() const override;
    QString rowDescription(int row) const override;
    bool selectColumn(int column) override;
    bool selectRow(int row) override;
    int selectedCellCount() const override;
    QList<QAccessibleInterface*> selectedCells() const override;
    int selectedColumnCount() const override;
    QList<int> selectedColumns() const override;
    int selectedRowCount() const override;
    QList<int> selectedRows() const override;
    QAccessibleInterface* summary() const override;
    bool unselectColumn(int column) override;
    bool unselectRow(int row) override;
private:
    QAccessible::Id & cellArray(int row, int col) const; // throws std::out_of_range exception
};

#endif
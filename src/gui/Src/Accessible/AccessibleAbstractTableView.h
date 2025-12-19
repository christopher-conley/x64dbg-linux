#pragma once
#ifndef QT_NO_ACCESSIBILITY
#include <QAccessibleWidget>
#include "../BasicView/AbstractTableView.h"

class AccessibleAbstractTableView : public QAccessibleWidget, public QAccessibleTableInterface
{
    std::vector<QAccessible::Id> cellInterfaces;
    int rows, cols;
    friend class AccessibleAbstractTableViewCell;
    AbstractTableView* m_tableView;
public:
    AccessibleAbstractTableView(QWidget* w);
    ~AccessibleAbstractTableView();
    // QAccessibleInterface
    int childCount() const override;
    QAccessibleInterface* child(int index) const override;
    QAccessibleInterface* QAccessibleInterface::childAt(int x, int y) const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QAccessibleInterface* focusChild() const override;
    bool isValid() const override;
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
    QAccessible::Id & cellArray(int row, int col); // Get reference of accessible id, throws std::out_of_range exception
    const QAccessible::Id & cellArray(int row, int col) const;
    virtual QString getCellContent(int row, int col) const; // Get plain text of a cell
};

#endif
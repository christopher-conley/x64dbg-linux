#pragma once
#ifndef QT_NO_ACCESSIBILITY
#include <QAccessibleWidget>
#include "AccessibleAbstractTableView.h"
#include "AbstractStdTable.h"

class AccessibleStdTable : public AccessibleAbstractTableView
{
public:
    AccessibleStdTable(QWidget* w);
    ~AccessibleStdTable();

    bool isRowSelected(int row) const override;
    QList<int> selectedRows() const override;
    int selectedRowCount() const override;
private:
    virtual QString getCellContent(int row, int col) const; // Get plain text of a cell
    AbstractStdTable* table() const;
};

#endif
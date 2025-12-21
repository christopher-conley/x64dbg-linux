// This file implements accessibility interface for StdTable
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleStdTable.h"
#include "StdTable.h"

AccessibleStdTable::AccessibleStdTable(QWidget* w) : AccessibleAbstractTableView(w)
{
}

AccessibleStdTable::~AccessibleStdTable()
{
}

bool AccessibleStdTable::isRowSelected(int row) const
{
    // row includes title
    auto table = this->table();
    return table->getInitialSelection() == row + table->getTableOffset() - 1;
}

QString AccessibleStdTable::getCellContent(int row, int column) const
{
    // row excludes title
    auto table = this->table();
    return table->getCellContent(table->getTableOffset() + row, column);
}

AbstractStdTable* AccessibleStdTable::table() const
{
    return dynamic_cast<AbstractStdTable*>(m_tableView);
}

// TODO: multi-selection
int AccessibleStdTable::selectedRowCount() const
{
    auto table = this->table();
    dsint r = table->getInitialSelection() - table->getTableOffset();
    if(r >= 0 && r <= rowCount() - 1)
        return 1;
    else
        return 0;
}

QList<int> AccessibleStdTable::selectedRows() const
{
    auto table = this->table();
    dsint r = table->getInitialSelection() - table->getTableOffset();
    if(r >= 0 && r <= rowCount() - 1)
        return QList<int>({ (int)r });
    else
        return QList<int>();
}

int AccessibleStdTable::selectedCellCount() const
{
    auto table = this->table();
    dsint r = table->getInitialSelection() - table->getTableOffset();
    if(r >= 0 && r <= rowCount() - 1)
        return 1;
    else
        return 0;
}

QList<QAccessibleInterface*> AccessibleStdTable::selectedCells() const
{
    auto table = this->table();
    dsint r = table->getInitialSelection() - table->getTableOffset();
    if(r >= 0 && r <= rowCount() - 1)
        return QList<QAccessibleInterface*>({ cellAt(r + 1, selectedColumns().first())});
    else
        return QList<QAccessibleInterface*>();
}
#endif
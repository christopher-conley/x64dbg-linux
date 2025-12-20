// This file implements accessibility interface for RegistersView
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
    auto table = this->table();
    return table->getInitialSelection() == row + table->getTableOffset();
}

QString AccessibleStdTable::getCellContent(int row, int column) const
{
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
    if(r >= 0 && r <= rowCount())
        return 1;
    else
        return 0;
}

QList<int> AccessibleStdTable::selectedRows() const
{
    auto table = this->table();
    dsint r = table->getInitialSelection() - table->getTableOffset();
    if(r >= 0 && r <= rowCount())
        return QList<int>({ (int)r });
    else
        return QList<int>();
}

#endif
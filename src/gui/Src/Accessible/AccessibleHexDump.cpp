// This file implements accessibility interface for HexDump
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleHexDump.h"
#include "HexDump.h"
#include "Bridge.h"

AccessibleHexDump::AccessibleHexDump(QWidget* w) : AccessibleAbstractTableView(w)
{
}

AccessibleHexDump::~AccessibleHexDump()
{
}

HexDump* AccessibleHexDump::dump() const
{
    return dynamic_cast<HexDump*>(m_tableView);
}

static int findFirstSelection(HexDump* dump)
{
    auto sel = dump->getInitialSelection();
    if(sel >= dump->getTableOffsetRva() && sel <= dump->getTableOffsetRva() + dump->getViewableRowsCount() * dump->getBytePerRowCount())
        return (dump->getInitialSelection() - dump->getTableOffsetRva()) / dump->getBytePerRowCount();
    else
        return -1;
}

bool AccessibleHexDump::isRowSelected(int row) const
{
    // row includes title
    return row - 1 == findFirstSelection(dump());
}

// TODO: multi-selection
int AccessibleHexDump::selectedRowCount() const
{
    // row includes title
    auto dump = this->dump();
    auto sel = dump->getInitialSelection();
    if(sel >= dump->getTableOffsetRva() && sel <= dump->getTableOffsetRva() + dump->getViewableRowsCount() * dump->getBytePerRowCount())
        return 1;
    else
        return 0;
}

QList<int> AccessibleHexDump::selectedRows() const
{
    int selectedRow = findFirstSelection(dump());
    if(selectedRow != -1)
        return QList<int>({ selectedRow });
    else
        return QList<int>();
}

int AccessibleHexDump::selectedCellCount() const
{
    return selectedRowCount();
}

QList<QAccessibleInterface*> AccessibleHexDump::selectedCells() const
{
    int selectedRow = findFirstSelection(dump());
    if(selectedRow != -1)
        return QList<QAccessibleInterface*>({ cellAt(selectedRow, selectedColumns().first()) });
    else
        return QList<QAccessibleInterface*>();
}

QString AccessibleHexDump::getCellContent(int row, int col) const
{
    const HexDump & dump = *this->dump();
    RichTextPainter::List richText;
    // Compute RVA
    duint rva = row * dump.getBytePerRowCount() + dump.getTableOffsetRva();
    dump.getColumnRichText(col, rva, richText);
    QString str;
    for(const auto & i : richText)
    {
        str += i.text;
    }
    if(col == 0)
        return str.trimmed();
    return str.simplified();
}

#endif

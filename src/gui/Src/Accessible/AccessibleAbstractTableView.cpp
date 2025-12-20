// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableView.h"
#include "AccessibleAbstractTableViewCell.h"
#include "AccessibleAbstractTableViewCellTitle.h"

AccessibleAbstractTableView::AccessibleAbstractTableView(QWidget* w) : QAccessibleWidget(w, QAccessible::Table, dynamic_cast<AbstractTableView*>(w)->accessibleName())
{
    m_tableView = dynamic_cast<AbstractTableView*>(w);
    assert(m_tableView);
    rows = std::min(m_tableView->getViewableRowsCount(), m_tableView->getRowCount());
    cols = m_tableView->getColumnCount();
    assert(rows < 10000 && cols < 1000 && rows >= 0 && cols >= 0);
    if(rows >= 10000)
        rows = 10000;
    if(rows < 0)
        rows = 0;
    if(cols >= 1000)
        cols = 1000;
    if(cols < 0)
        cols = 0;
    int hiddenCols = 0;
    for(int i = 0; i < cols; i++)
    {
        if(m_tableView->getColumnHidden(i))
            hiddenCols++;
    }
    cols -= hiddenCols;
    if(rows > 0 && cols > 0)
    {
        cellInterfaces.resize(rows * cols, 0);
        for(auto i = 0; i < rows; i++)
        {
            for(int j = 0; j < cols; j++)
            {
                cellArray(i, j) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(this, i, j));
            }
        }
    }
    columnTitleInterfaces = std::vector<QAccessible::Id>(cols);
    for(int i = 0; i < cols; i++)
    {
        columnTitleInterfaces[i] = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCellTitle(this, i));
    }
    updateVisibleColumns();
    assert(cellInterfaces.size() == rows * cols);
    assert(columnTitleInterfaces.size() == cols);
}

AccessibleAbstractTableView::~AccessibleAbstractTableView()
{
    std::for_each(cellInterfaces.cbegin(), cellInterfaces.cend(), QAccessible::deleteAccessibleInterface);
}

QString AccessibleAbstractTableView::getCellContent(int row, int column) const
{
    return QString("Row %1 Column %2").arg(row).arg(column);
}

AbstractTableView* AccessibleAbstractTableView::getTable() const
{
    return m_tableView;
}

QAccessible::Id & AccessibleAbstractTableView::cellArray(int row, int column)
{
    if(row < 0 || column < 0 || row >= rows || column >= cols)
        throw std::out_of_range("Table cell row or column out of range");
    return cellInterfaces.at(row * cols + column);
}

const QAccessible::Id & AccessibleAbstractTableView::cellArray(int row, int column) const
{
    if(row < 0 || column < 0 || row >= rows || column >= cols)
        throw std::out_of_range("Table cell row or column out of range");
    return cellInterfaces.at(row * cols + column);
}

void AccessibleAbstractTableView::updateVisibleColumns()
{
    duint c = 0;
    m_visibleColumns.clear();
    m_visibleColumns.reserve(cols);
    for(duint j = 0; j < m_tableView->getColumnCount(); j++)
    {
        duint i = m_tableView->mColumnOrder[j];
        if(m_tableView->getColumnHidden(i))
            continue;
        m_visibleColumns.push_back(i);
    }
    assert(m_visibleColumns.size() == cols);
}

duint AccessibleAbstractTableView::logicalColumn(int physicalColumn) const
{
    return m_visibleColumns.at(physicalColumn);
}

int AccessibleAbstractTableView::childCount() const
{
    return cellInterfaces.size();
}

QAccessibleInterface* AccessibleAbstractTableView::child(int index) const
{
    if(index >= cols && index < cols + childCount())
    {
        return QAccessible::accessibleInterface(cellInterfaces[index - cols]);
    }
    else if(index >= 0 && index < cols)
    {
        return QAccessible::accessibleInterface(columnTitleInterfaces[index]);
    }
    else
    {
        return nullptr;
    }
}

QAccessibleInterface* AccessibleAbstractTableView::childAt(int x, int y) const
{
    int col = m_tableView->getColumnIndexFromX(x);
    try
    {
        if(y < 0 || x < 0)
            return nullptr;
        if(y < m_tableView->getHeaderHeight())
        {
            const QAccessible::Id & id = columnTitleInterfaces.at(col);
            return QAccessible::accessibleInterface(id);
        }
        y = m_tableView->transY(y);
        auto row = m_tableView->getIndexOffsetFromY(y);
        const QAccessible::Id & id = cellArray(row, col);
        return QAccessible::accessibleInterface(id);
    }
    catch(std::out_of_range)
    {
        return nullptr;
    }
}

int AccessibleAbstractTableView::indexOfChild(const QAccessibleInterface* child) const
{
    for(int i = 0; i < childCount(); i++)
    {
        const QAccessibleInterface* a = this->child(i);
        if(a == child)
        {
            return i;
        }
    }
    return -1;
}

QAccessibleInterface* AccessibleAbstractTableView::focusChild() const
{
    return nullptr;
}

bool AccessibleAbstractTableView::isValid() const
{
    return true;
}

QAccessible::State AccessibleAbstractTableView::state() const
{
    QAccessible::State state;
    state.focusable = true;
    if(m_tableView->hasFocus())
        state.focused = true;
    state.active = m_tableView->isEnabled();
    state.multiLine = true;
    state.multiSelectable = false;
    state.disabled = !m_tableView->isEnabled();
    state.hasPopup = true;
    return state;
}

void* AccessibleAbstractTableView::interface_cast(QAccessible::InterfaceType type)
{
    if(type == QAccessible::TableInterface)
        return static_cast<QAccessibleTableInterface*>(this);
    else
        return nullptr;
}

QAccessibleInterface* AccessibleAbstractTableView::caption() const
{
    return nullptr;
}

QAccessibleInterface* AccessibleAbstractTableView::cellAt(int row, int column) const
{
    try
    {
        QAccessible::Id id;
        if(row > 0)
        {
            id = cellArray(row - 1, column);
        }
        else if(row == 0)
        {
            id = columnTitleInterfaces.at(column);
        }
        else
            return nullptr;
        return QAccessible::accessibleInterface(id);
    }
    catch(std::out_of_range)
    {
        return nullptr;
    }
}

int AccessibleAbstractTableView::columnCount() const
{
    return cols;
}

QString AccessibleAbstractTableView::columnDescription(int column) const
{
    return m_tableView->getColTitle(logicalColumn(column));
}

bool AccessibleAbstractTableView::isColumnSelected(int column) const
{
    return m_tableView->accessibilitySelectedColumn == column;
}

bool AccessibleAbstractTableView::isRowSelected(int row) const
{
    return false;
}

void AccessibleAbstractTableView::modelChange(QAccessibleTableModelChangeEvent* event)
{
    int newRows = std::min(m_tableView->getViewableRowsCount(), m_tableView->getRowCount());
    int newCols = m_tableView->getColumnCount();
    int hiddenCols = 0;
    assert(!(newRows < 0 || newCols < 0 || newRows > 10000 || newCols > 1000));
    if(newRows > 10000)
        newRows = 10000;
    if(newCols > 1000)
        newCols = 1000;
    for(int i = 0; i < newCols; i++)
    {
        hiddenCols += (m_tableView->getColumnHidden(i)) ? 1 : 0;
    }
    newCols -= hiddenCols;
    // Resize array
    try
    {
        if(newCols == cols)
        {
            if(newRows < rows)
            {
                for(int i = newRows * cols; i < rows * cols; i++)
                {
                    QAccessible::deleteAccessibleInterface(cellInterfaces.at(i));
                }
            }
            if(newRows != rows)
            {
                cellInterfaces.resize(newRows * newCols, 0);
            }
            if(newRows < rows)
            {
                rows = newRows;
            }
            if(newRows > rows)
            {
                int oldRows = rows;
                rows = newRows;
                for(auto i = oldRows; i < newRows; i++)
                {
                    for(int j = 0; j < newCols; j++)
                    {
                        cellArray(i, j) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(this, i, j));
                    }
                }
            }
        }
        else
        {
            // column titles
            for(int i = newCols; i < cols; i++)
            {
                QAccessible::deleteAccessibleInterface(columnTitleInterfaces.at(i));
            }
            columnTitleInterfaces.resize(newCols);
            for(int i = cols; i < newCols; i++)
            {
                columnTitleInterfaces.at(i) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCellTitle(this, i));
            }
            // rows
            std::for_each(cellInterfaces.cbegin(), cellInterfaces.cend(), QAccessible::deleteAccessibleInterface);
            cellInterfaces = std::vector<QAccessible::Id>();
            rows = newRows;
            cols = newCols;
            cellInterfaces.resize(rows * cols, 0);
            for(auto i = 0; i < newRows; i++)
            {
                for(int j = 0; j < newCols; j++)
                {
                    cellArray(i, j) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(this, i, j));
                }
            }
        }
        updateVisibleColumns();
    }
    catch(std::out_of_range)
    {
        __debugbreak();
    }
    assert(cellInterfaces.size() == rows * cols);
    assert(columnTitleInterfaces.size() == cols);
}

int AccessibleAbstractTableView::rowCount() const
{
    // returned row includes title
    return rows + 1;
}

QString AccessibleAbstractTableView::rowDescription(int row) const
{
    if(row == 0)  // title row
        return QString();
    auto cell = cellAt(row + 1, 0);
    if(cell)
        return cell->text(QAccessible::Value);
    else
        return QString();
}

bool AccessibleAbstractTableView::selectColumn(int column)
{
    return false;
}

bool AccessibleAbstractTableView::selectRow(int row)
{
    return false;
}

int AccessibleAbstractTableView::selectedCellCount() const
{
    return 0;
}

QList<QAccessibleInterface*> AccessibleAbstractTableView::selectedCells() const
{
    return QList<QAccessibleInterface*>();
}

int AccessibleAbstractTableView::selectedColumnCount() const
{
    return 1;
}

QList<int> AccessibleAbstractTableView::selectedColumns() const
{
    return QList<int>({m_tableView->accessibilitySelectedColumn});
}

int AccessibleAbstractTableView::selectedRowCount() const
{
    return 0;
}

QList<int> AccessibleAbstractTableView::selectedRows() const
{
    return QList<int>();
}

QAccessibleInterface* AccessibleAbstractTableView::summary() const
{
    return nullptr;
}

bool AccessibleAbstractTableView::unselectColumn(int column)
{
    return false;
}

bool AccessibleAbstractTableView::unselectRow(int row)
{
    return false;
}

#endif
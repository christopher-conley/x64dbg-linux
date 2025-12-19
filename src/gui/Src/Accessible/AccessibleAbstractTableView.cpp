// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableView.h"
#include "AccessibleAbstractTableViewCell.h"

AccessibleAbstractTableView::AccessibleAbstractTableView(QWidget* w) : QAccessibleWidget(w, QAccessible::Table, dynamic_cast<AbstractTableView*>(w)->accessibleName())
{
    m_tableView = dynamic_cast<AbstractTableView*>(w);
    rows = m_tableView->getViewableRowsCount();
    cols = m_tableView->getColumnCount();
    int hiddenCols = 0;
    for(int i = 0; i < cols; i++)
    {
        if(m_tableView->getColumnHidden(i))
            hiddenCols++;
    }
    cols -= hiddenCols;
    if(rows > 0 && cols > 0)
    {
        cellInterfaces = std::vector<QAccessible::Id>((size_t)(rows * cols), (QAccessible::Id)0);
        for(auto i = 0; i < rows; i++)
        {
            for(int j = 0; j < cols; j++)
            {
                cellArray(i, j) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(this, i, j));
            }
        }
    }
    else
    {
        rows = 0;
        cols = 0;
    }
}

AccessibleAbstractTableView::~AccessibleAbstractTableView()
{
    std::for_each(cellInterfaces.cbegin(), cellInterfaces.cend(), QAccessible::deleteAccessibleInterface);
}

QString AccessibleAbstractTableView::getCellContent(int row, int column) const
{
    //return QString();
    return QString("Row %1 Column %2").arg(row).arg(column);
}

QAccessible::Id & AccessibleAbstractTableView::cellArray(int row, int column)
{
    if(row < 0 || column < 0 || row >= rows || column >= cols)
        throw std::out_of_range("");
    return cellInterfaces.at(row * cols + column);
}

const QAccessible::Id & AccessibleAbstractTableView::cellArray(int row, int column) const
{
    if(row < 0 || column < 0 || row >= rows || column >= cols)
        throw std::out_of_range("");
    return cellInterfaces.at(row * cols + column);
}

int AccessibleAbstractTableView::childCount() const
{
    return cellInterfaces.size();
}

QAccessibleInterface* AccessibleAbstractTableView::child(int index) const
{
    if(index >= 0 && index < childCount())
    {
        return QAccessible::accessibleInterface(cellInterfaces[index]);
    }
    else
    {
        return nullptr;
    }
}

QAccessibleInterface* AccessibleAbstractTableView::childAt(int x, int y) const
{
    try
    {
        int col = m_tableView->getColumnIndexFromX(x);
        auto row = m_tableView->getIndexOffsetFromY(m_tableView->transY(y));
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
        auto & id = cellArray(row, column);
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
    return m_tableView->getColTitle(column);
}

bool AccessibleAbstractTableView::isColumnSelected(int column) const
{
    return false;
}

bool AccessibleAbstractTableView::isRowSelected(int row) const
{
    return false;
}

void AccessibleAbstractTableView::modelChange(QAccessibleTableModelChangeEvent* event)
{
    int newRows = m_tableView->getViewableRowsCount();
    int newCols = m_tableView->getColumnCount();
    int hiddenCols = 0;
    for(int i = 0; i < cols; i++)
    {
        hiddenCols += (m_tableView->getColumnHidden(i)) ? 1 : 0;
    }
    newCols -= hiddenCols;
    // Resize array
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
            cellInterfaces.resize(newRows * newCols);
        }
        if(newRows > rows)
        {
            for(auto i = rows; i < newRows; i++)
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
        std::for_each(cellInterfaces.cbegin(), cellInterfaces.cend(), QAccessible::deleteAccessibleInterface);
        cellInterfaces = std::vector<QAccessible::Id>((size_t)(rows * cols), (QAccessible::Id)0);
        for(auto i = 0; i < newRows; i++)
        {
            for(int j = 0; j < newCols; j++)
            {
                cellArray(i, j) = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(this, i, j));
            }
        }
    }
    rows = newRows;
    cols = newCols;
}

int AccessibleAbstractTableView::rowCount() const
{
    return rows;
}

QString AccessibleAbstractTableView::rowDescription(int row) const
{
    auto cell = cellAt(row, 0);
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
    return 0;
}

QList<int> AccessibleAbstractTableView::selectedColumns() const
{
    return QList<int>();
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
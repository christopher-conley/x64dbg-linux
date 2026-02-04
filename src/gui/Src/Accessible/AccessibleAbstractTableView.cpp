// This file implements accessibility interface for AbstractTableView
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
    }
    columnTitleInterfaces = std::vector<QAccessible::Id>(cols, 0);
    updateVisibleColumns();
    assert(cellInterfaces.size() == rows * cols);
    assert(columnTitleInterfaces.size() == cols);
}

AccessibleAbstractTableView::~AccessibleAbstractTableView()
{
    for(const auto & id : columnTitleInterfaces)
    {
        if(id != 0)
            QAccessible::deleteAccessibleInterface(id);
    }
    for(const auto & id : cellInterfaces)
    {
        if(id != 0)
            QAccessible::deleteAccessibleInterface(id);
    }
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

int AccessibleAbstractTableView::physicalColumnFromLogical(int logicalColumn) const
{
    for(size_t i = 0; i < m_visibleColumns.size(); i++)
    {
        if(static_cast<int>(m_visibleColumns[i]) == logicalColumn)
            return static_cast<int>(i);
    }
    return -1;
}

int AccessibleAbstractTableView::childCount() const
{
    ensureModelUpToDate();
    return cols + static_cast<int>(cellInterfaces.size());
}

QAccessibleInterface* AccessibleAbstractTableView::child(int index) const
{
    ensureModelUpToDate();
    if(index >= cols && index < cols + static_cast<int>(cellInterfaces.size()))
    {
        const auto cellIndex = index - cols;
        auto & id = const_cast<std::vector<QAccessible::Id>&>(cellInterfaces)[cellIndex];
        if(id == 0)
        {
            const int row = cellIndex / cols;
            const int col = cellIndex % cols;
            id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(const_cast<AccessibleAbstractTableView*>(this), row, col));
        }
        return QAccessible::accessibleInterface(id);
    }
    else if(index >= 0 && index < cols)
    {
        auto & id = const_cast<std::vector<QAccessible::Id>&>(columnTitleInterfaces)[index];
        if(id == 0)
        {
            id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCellTitle(const_cast<AccessibleAbstractTableView*>(this), index));
        }
        return QAccessible::accessibleInterface(id);
    }
    else
    {
        return nullptr;
    }
}

QAccessibleInterface* AccessibleAbstractTableView::childAt(int x, int y) const
{
    ensureModelUpToDate();
    int col = m_tableView->getColumnIndexFromX(x);
    try
    {
        if(y < 0 || x < 0)
            return nullptr;
        col = physicalColumnFromLogical(col);
        if(col < 0 || col >= cols)
            return nullptr;
        if(y < m_tableView->getHeaderHeight())
        {
            auto & id = const_cast<std::vector<QAccessible::Id>&>(columnTitleInterfaces).at(col);
            if(id == 0)
            {
                id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCellTitle(const_cast<AccessibleAbstractTableView*>(this), col));
            }
            return QAccessible::accessibleInterface(id);
        }
        y = m_tableView->transY(y);
        auto row = m_tableView->getIndexOffsetFromY(y);
        auto & id = const_cast<AccessibleAbstractTableView*>(this)->cellArray(row, col);
        if(id == 0)
        {
            id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(const_cast<AccessibleAbstractTableView*>(this), row, col));
        }
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
    ensureModelUpToDate();
    try
    {
        QAccessible::Id id;
        if(row > 0)
        {
            id = const_cast<AccessibleAbstractTableView*>(this)->cellArray(row - 1, column);
            if(id == 0)
            {
                id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCell(const_cast<AccessibleAbstractTableView*>(this), row - 1, column));
            }
        }
        else if(row == 0)
        {
            id = const_cast<std::vector<QAccessible::Id>&>(columnTitleInterfaces).at(column);
            if(id == 0)
            {
                id = QAccessible::registerAccessibleInterface(new AccessibleAbstractTableViewCellTitle(const_cast<AccessibleAbstractTableView*>(this), column));
            }
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
    ensureModelUpToDate();
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
    if(newRows < 0)
        newRows = 0;
    if(newCols < 0)
        newCols = 0;
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
                    if(cellInterfaces.at(i) != 0)
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
                        cellArray(i, j) = 0;
                    }
                }
            }
        }
        else
        {
            // column titles
            for(int i = newCols; i < cols; i++)
            {
                if(columnTitleInterfaces.at(i) != 0)
                    QAccessible::deleteAccessibleInterface(columnTitleInterfaces.at(i));
            }
            columnTitleInterfaces.resize(newCols, 0);
            // rows
            for(const auto & id : cellInterfaces)
            {
                if(id != 0)
                    QAccessible::deleteAccessibleInterface(id);
            }
            cellInterfaces = std::vector<QAccessible::Id>();
            rows = newRows;
            cols = newCols;
            cellInterfaces.resize(rows * cols, 0);
            for(auto i = 0; i < newRows; i++)
            {
                for(int j = 0; j < newCols; j++)
                {
                    cellArray(i, j) = 0;
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
    ensureModelUpToDate();
    // returned row includes title
    return rows + 1;
}

void AccessibleAbstractTableView::ensureModelUpToDate() const
{
    int newRows = std::min(m_tableView->getViewableRowsCount(), m_tableView->getRowCount());
    int newCols = m_tableView->getColumnCount();
    int hiddenCols = 0;
    if(newRows > 10000)
        newRows = 10000;
    if(newCols > 1000)
        newCols = 1000;
    if(newRows < 0)
        newRows = 0;
    if(newCols < 0)
        newCols = 0;
    for(int i = 0; i < newCols; i++)
    {
        hiddenCols += (m_tableView->getColumnHidden(i)) ? 1 : 0;
    }
    newCols -= hiddenCols;
    if(newRows != rows || newCols != cols)
    {
        const_cast<AccessibleAbstractTableView*>(this)->modelChange(nullptr);
    }
}

QString AccessibleAbstractTableView::rowDescription(int row) const
{
    if(row == 0)  // title row
        return QString();
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

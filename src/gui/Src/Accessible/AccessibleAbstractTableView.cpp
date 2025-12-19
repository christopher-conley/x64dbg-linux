// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleAbstractTableView.h"

AccessibleAbstractTableViewCell::AccessibleAbstractTableViewCell(AccessibleAbstractTableView* parent, duint row, int column) : mParent(parent), row(row), column(column)
{
}

QString AccessibleAbstractTableViewCell::text(QAccessible::Text t) const
{
    AbstractTableView* w = mParent->m_tableView;
    switch(t)
    {
    case QAccessible::Name:
    case QAccessible::Value:
        return QString("Example Table Cell %1 row %2 column").arg(row).arg(column);
    default:
        return QString();
    }
}

QColor AccessibleAbstractTableViewCell::foregroundColor() const
{
    return mParent->m_tableView->mTextColor;
}

int AccessibleAbstractTableViewCell::childCount() const
{
    return 0;
}

QWindow* AccessibleAbstractTableViewCell::window() const
{
    return mParent->window();
}

QAccessibleInterface* AccessibleAbstractTableViewCell::parent() const
{
    return dynamic_cast<QAccessibleInterface*>(mParent);
}

QAccessibleInterface* AccessibleAbstractTableViewCell::child(int index) const
{
    return nullptr;
}

int AccessibleAbstractTableViewCell::indexOfChild(const QAccessibleInterface* child) const
{
    return -1;
}

QAccessible::Role AccessibleAbstractTableViewCell::role() const
{
    return QAccessible::Cell;
}

QAccessible::State AccessibleAbstractTableViewCell::state() const
{
    QAccessible::State state;
    state.focusable = mParent->m_tableView->getRowCount() > 0;
    state.active = state.focusable;
    state.selectable = state.focusable;
    state.selected = false;
    state.focused = false;
    return state;
}

QAccessibleInterface* AccessibleAbstractTableViewCell::childAt(int x, int y) const
{
    return nullptr;
}

QObject* AccessibleAbstractTableViewCell::object() const
{
    return nullptr;
}

void AccessibleAbstractTableViewCell::setText(QAccessible::Text t, const QString & text)
{
}

QRect AccessibleAbstractTableViewCell::rect() const
{
    const auto & table = mParent->m_tableView;
    int height = table->getRowHeight();
    auto & actualColumn = table->mColumnOrder[column];
    QPoint pos(table->getColumnPosition(column), table->getHeaderHeight() + row * height);
    pos = table->mapToGlobal(pos);
    return QRect(pos, QSize(table->getColumnWidth(actualColumn), height));
}

bool AccessibleAbstractTableViewCell::isValid() const
{
    return true;
}

void* AccessibleAbstractTableViewCell::interface_cast(QAccessible::InterfaceType type)
{
    if(type == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    else
        return nullptr;
}

bool AccessibleAbstractTableViewCell::isSelected() const
{
    return false;
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCell::columnHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

QList<QAccessibleInterface*> AccessibleAbstractTableViewCell::rowHeaderCells() const
{
    return QList<QAccessibleInterface*>();
}

int AccessibleAbstractTableViewCell::columnIndex() const
{
    return column;
}

int AccessibleAbstractTableViewCell::rowIndex() const
{
    return row;
}

int AccessibleAbstractTableViewCell::columnExtent() const
{
    return 1;
}

int AccessibleAbstractTableViewCell::rowExtent() const
{
    return 1;
}

QAccessibleInterface* AccessibleAbstractTableViewCell::table() const
{
    return static_cast<QAccessibleInterface*>(mParent);
}

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
    }
    else
    {
        rows = 0;
        cols = 0;
    }
}

AccessibleAbstractTableView::~AccessibleAbstractTableView()
{
    for(auto & i : cellInterfaces)
    {
        if(i != 0)
        {
            QAccessible::deleteAccessibleInterface(i);
        }
    }
}

QAccessible::Id & AccessibleAbstractTableView::cellArray(int row, int column) const
{
    if(row < 0 || column < 0 || row >= rows || column >= cols)
        throw std::out_of_range("");
    return cellInterfaces.at(row * cols + column);
}

int AccessibleAbstractTableView::childCount() const
{
    return rows * cols;
}

QAccessibleInterface* AccessibleAbstractTableView::child(int index) const
{
    if(index >= 0 && index < childCount())
    {
        if(cellInterfaces[index] != 0)
        {
            return QAccessible::accessibleInterface(cellInterfaces[index]);
        }
        else
        {
            int row = index / cols;
            int column = index % cols;
            auto child = new AccessibleAbstractTableViewCell((AccessibleAbstractTableView*)this, row, column);
            cellInterfaces[index] = QAccessible::registerAccessibleInterface(child);
            return child;
        }
    }
    else
        return nullptr;
}

int AccessibleAbstractTableView::indexOfChild(const QAccessibleInterface* child) const
{
    for(int i = 0; i < cellInterfaces.size(); i++)
    {
        unsigned int id = cellInterfaces[i];
        if(id != 0)
        {
            QAccessibleInterface* a = QAccessible::accessibleInterface(id);
            if(a == child)
            {
                return i;
            }
        }
    }
    return -1;
}

QAccessibleInterface* AccessibleAbstractTableView::childAt(int x, int y) const
{
    try
    {
        int col = m_tableView->getColumnIndexFromX(x);
        auto row = m_tableView->getIndexOffsetFromY(m_tableView->transY(y));
        QAccessible::Id & id = cellArray(row, col);
        if(id != 0)
        {
            return QAccessible::accessibleInterface(id);
        }
        else
        {
            auto child = new AccessibleAbstractTableViewCell((AccessibleAbstractTableView*)this, row, col);
            id = QAccessible::registerAccessibleInterface(child);
            return child;
        }
    }
    catch(std::out_of_range)
    {
        return nullptr;
    }
}

QAccessibleInterface* AccessibleAbstractTableView::focusChild() const
{
    return nullptr;
}

bool AccessibleAbstractTableView::isValid() const
{
    return true;
}

QAccessible::Role AccessibleAbstractTableView::role() const
{
    return QAccessible::Table;
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
        if(id == 0)
        {
            AccessibleAbstractTableViewCell* cell = new AccessibleAbstractTableViewCell((AccessibleAbstractTableView*)this, row, column);
            id = QAccessible::registerAccessibleInterface(cell);
            return cell;
        }
        else
        {
            return QAccessible::accessibleInterface(id);
        }
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
    return QString();
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
        if(m_tableView->getColumnHidden(i))
            hiddenCols++;
    }
    newCols -= hiddenCols;
    // Resize array
    if(newCols == cols)
    {
        if(newRows < rows)
        {
            for(int i = newRows * cols; i < rows * cols; i++)
            {
                auto & id = cellInterfaces.at(i);
                if(id != 0)
                {
                    QAccessible::deleteAccessibleInterface(id);
                }
            }
        }
        if(newRows != rows)
        {
            cellInterfaces.resize(newRows * newCols);
        }
        if(newRows > rows)
        {
            memset(&cellInterfaces.at(rows * cols), 0, sizeof(QAccessible::Id) * (newRows - rows) * cols);
        }
    }
    else
    {
        for(int i = 0; i < rows * cols; i++)
        {
            auto & id = cellInterfaces.at(i);
            if(id != 0)
            {
                QAccessible::deleteAccessibleInterface(id);
            }
        }
        cellInterfaces = std::vector<QAccessible::Id>((size_t)(rows * cols), (QAccessible::Id)0);
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
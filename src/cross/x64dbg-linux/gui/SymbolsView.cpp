#include "gui/SymbolsView.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>

namespace X64DbgLinux {

SymbolsView::SymbolsView(QWidget* parent)
    : QTreeView(parent)
    , m_model(new QStandardItemModel(this))
{
    setModel(m_model);
    setupColumns();
    createContextMenu();

    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    header()->setStretchLastSection(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformRowHeights(true);
}

SymbolsView::~SymbolsView() = default;

void SymbolsView::setupColumns()
{
    m_model->setColumnCount(4);
    m_model->setHorizontalHeaderLabels({
        tr("Name"),
        tr("Address"),
        tr("Size"),
        tr("Type")
    });

    setColumnWidth(0, 300);  // Name
    setColumnWidth(1, 120);  // Address
    setColumnWidth(2, 80);   // Size
    setColumnWidth(3, 80);   // Type
}

void SymbolsView::createContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_gotoAction = new QAction(tr("Go to Address"), this);
    m_copyNameAction = new QAction(tr("Copy Name"), this);
    m_copyAddressAction = new QAction(tr("Copy Address"), this);
    m_expandAllAction = new QAction(tr("Expand All"), this);
    m_collapseAllAction = new QAction(tr("Collapse All"), this);

    connect(m_gotoAction, &QAction::triggered, this, &SymbolsView::onGotoSymbol);
    connect(m_copyNameAction, &QAction::triggered, this, &SymbolsView::onCopySymbolName);
    connect(m_copyAddressAction, &QAction::triggered, this, &SymbolsView::onCopySymbolAddress);
    connect(m_expandAllAction, &QAction::triggered, this, &SymbolsView::expandAllModules);
    connect(m_collapseAllAction, &QAction::triggered, this, &SymbolsView::collapseAllModules);

    m_contextMenu->addAction(m_gotoAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_copyNameAction);
    m_contextMenu->addAction(m_copyAddressAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_expandAllAction);
    m_contextMenu->addAction(m_collapseAllAction);
}

void SymbolsView::addModule(const ModuleViewInfo& module)
{
    // Remove existing module with same base
    removeModule(module.base);

    // Create module item
    auto* moduleItem = new QStandardItem(QString::fromStdString(module.name));
    moduleItem->setData(static_cast<qulonglong>(module.base), Qt::UserRole);
    moduleItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    moduleItem->setEditable(false);

    // Store module info
    m_moduleItems[moduleItem] = module;

    // Module info columns
    QString addrStr = QString("0x%1").arg(module.base, 16, 16, QChar('0'));
    auto* addrItem = new QStandardItem(addrStr);
    addrItem->setEditable(false);

    auto* sizeItem = new QStandardItem(QString::number(module.size));
    sizeItem->setEditable(false);

    auto* typeItem = new QStandardItem(tr("Module"));
    typeItem->setEditable(false);

    m_model->appendRow({moduleItem, addrItem, sizeItem, typeItem});

    // Add symbols as children
    for (const auto& sym : module.symbols) {
        auto* symItem = new QStandardItem(QString::fromStdString(sym.name));
        symItem->setData(static_cast<qulonglong>(sym.address), Qt::UserRole);
        symItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        symItem->setEditable(false);

        // Store symbol info
        m_symbolItems[symItem] = sym;

        QString symAddr = QString("0x%1").arg(sym.address, 16, 16, QChar('0'));
        auto* symAddrItem = new QStandardItem(symAddr);
        symAddrItem->setEditable(false);

        auto* symSizeItem = new QStandardItem(QString::number(sym.size));
        symSizeItem->setEditable(false);

        QString symType = sym.type == 0 ? tr("Function") : tr("Data");
        auto* symTypeItem = new QStandardItem(symType);
        symTypeItem->setEditable(false);

        moduleItem->appendRow({symItem, symAddrItem, symSizeItem, symTypeItem});
    }

    // Expand module by default
    expand(m_model->indexFromItem(moduleItem));
}

void SymbolsView::removeModule(uint64_t base)
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* item = m_model->item(i);
        if (item && item->data(Qt::UserRole).toULongLong() == base) {
            // Remove symbol items
            for (int j = 0; j < item->rowCount(); ++j) {
                QStandardItem* child = item->child(j);
                if (child) {
                    m_symbolItems.erase(child);
                }
            }
            m_moduleItems.erase(item);
            m_model->removeRow(i);
            return;
        }
    }
}

void SymbolsView::clearAllModules()
{
    m_model->removeRows(0, m_model->rowCount());
    m_symbolItems.clear();
    m_moduleItems.clear();
}

std::optional<SymbolViewInfo> SymbolsView::selectedSymbol() const
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return std::nullopt;
    }

    QModelIndex index = selected.first();
    QStandardItem* item = m_model->itemFromIndex(index);

    // Check if it's a symbol item (has parent)
    if (item && item->parent()) {
        auto it = m_symbolItems.find(item);
        if (it != m_symbolItems.end()) {
            return it->second;
        }
    }

    return std::nullopt;
}

std::optional<ModuleViewInfo> SymbolsView::selectedModule() const
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return std::nullopt;
    }

    QModelIndex index = selected.first();
    QStandardItem* item = m_model->itemFromIndex(index);

    // If item has no parent, it's a module
    if (item && !item->parent()) {
        auto it = m_moduleItems.find(item);
        if (it != m_moduleItems.end()) {
            return it->second;
        }
    }

    // If item has parent, get the parent module
    if (item && item->parent()) {
        QStandardItem* parent = item->parent();
        auto it = m_moduleItems.find(parent);
        if (it != m_moduleItems.end()) {
            return it->second;
        }
    }

    return std::nullopt;
}

void SymbolsView::refresh()
{
    // Refresh is handled by model updates
}

std::vector<SymbolViewInfo> SymbolsView::findSymbols(const std::string& pattern) const
{
    std::vector<SymbolViewInfo> results;
    QString qPattern = QString::fromStdString(pattern).toLower();

    for (const auto& [item, sym] : m_symbolItems) {
        if (QString::fromStdString(sym.name).toLower().contains(qPattern)) {
            results.push_back(sym);
        }
    }

    return results;
}

void SymbolsView::contextMenuEvent(QContextMenuEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    m_contextMenuItem = m_model->itemFromIndex(index);

    // Enable/disable actions based on selection
    bool isSymbol = m_contextMenuItem && m_contextMenuItem->parent();
    m_gotoAction->setEnabled(isSymbol);
    m_copyNameAction->setEnabled(true);
    m_copyAddressAction->setEnabled(isSymbol);

    if (isSymbol) {
        auto it = m_symbolItems.find(m_contextMenuItem);
        if (it != m_symbolItems.end()) {
            emit contextMenuRequestedForSymbol(it->second, event->globalPos());
        }
    }

    m_contextMenu->exec(event->globalPos());
}

void SymbolsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        QStandardItem* item = m_model->itemFromIndex(index);
        if (item && item->parent()) {
            // It's a symbol
            auto it = m_symbolItems.find(item);
            if (it != m_symbolItems.end()) {
                emit symbolSelected(it->second);
                emit gotoSymbolRequested(it->second.address);
            }
        } else if (item) {
            // It's a module
            auto it = m_moduleItems.find(item);
            if (it != m_moduleItems.end()) {
                emit moduleSelected(it->second);
            }
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void SymbolsView::onGotoSymbol()
{
    if (m_contextMenuItem && m_contextMenuItem->parent()) {
        auto it = m_symbolItems.find(m_contextMenuItem);
        if (it != m_symbolItems.end()) {
            emit gotoSymbolRequested(it->second.address);
        }
    }
}

void SymbolsView::onCopySymbolName()
{
    if (m_contextMenuItem) {
        QString name = m_contextMenuItem->text();
        QApplication::clipboard()->setText(name);
    }
}

void SymbolsView::onCopySymbolAddress()
{
    if (m_contextMenuItem) {
        uint64_t addr = m_contextMenuItem->data(Qt::UserRole).toULongLong();
        QString addrStr = QString("0x%1").arg(addr, 16, 16, QChar('0'));
        QApplication::clipboard()->setText(addrStr);
    }
}

void SymbolsView::expandAllModules()
{
    expandAll();
}

void SymbolsView::collapseAllModules()
{
    collapseAll();
}

} // namespace X64DbgLinux

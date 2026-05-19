#pragma once

#include <QTreeView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <cstdint>
#include <string>
#include <vector>

namespace X64DbgLinux {

// Symbol information for GUI display
struct SymbolViewInfo {
    std::string name;
    uint64_t address;
    size_t size;
    int type;  // 0=function, 1=data
    std::string module;
};

// Module information for GUI display
struct ModuleViewInfo {
    std::string name;
    std::string path;
    uint64_t base;
    size_t size;
    std::vector<SymbolViewInfo> symbols;
};

class SymbolsView : public QTreeView
{
    Q_OBJECT

public:
    explicit SymbolsView(QWidget* parent = nullptr);
    ~SymbolsView() override;

    // Add module and its symbols
    void addModule(const ModuleViewInfo& module);
    void removeModule(uint64_t base);
    void clearAllModules();

    // Get selected symbol
    std::optional<SymbolViewInfo> selectedSymbol() const;
    std::optional<ModuleViewInfo> selectedModule() const;

    // Refresh display
    void refresh();

    // Find symbol by name
    std::vector<SymbolViewInfo> findSymbols(const std::string& pattern) const;

signals:
    void symbolSelected(const SymbolViewInfo& symbol);
    void moduleSelected(const ModuleViewInfo& module);
    void gotoSymbolRequested(uint64_t address);
    void contextMenuRequestedForSymbol(const SymbolViewInfo& symbol, const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupColumns();
    void createContextMenu();
    void onGotoSymbol();
    void onCopySymbolName();
    void onCopySymbolAddress();
    void expandAllModules();
    void collapseAllModules();

    QStandardItemModel* m_model = nullptr;
    QMenu* m_contextMenu = nullptr;
    QAction* m_gotoAction = nullptr;
    QAction* m_copyNameAction = nullptr;
    QAction* m_copyAddressAction = nullptr;
    QAction* m_expandAllAction = nullptr;
    QAction* m_collapseAllAction = nullptr;

    // Track items for lookup
    std::unordered_map<QStandardItem*, SymbolViewInfo> m_symbolItems;
    std::unordered_map<QStandardItem*, ModuleViewInfo> m_moduleItems;
    QStandardItem* m_contextMenuItem = nullptr;
};

} // namespace X64DbgLinux

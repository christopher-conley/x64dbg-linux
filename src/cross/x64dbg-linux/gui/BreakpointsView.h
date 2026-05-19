#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <cstdint>

namespace X64DbgLinux {

// Breakpoint types matching x64dbg conventions
enum class BreakpointType {
    Software = 0,      // int3 breakpoint
    Hardware = 1,      // Hardware breakpoint (DRx)
    Memory = 2,        // Memory protection breakpoint
    None = 3
};

// Breakpoint structure for GUI display
struct BreakpointInfo {
    uint64_t address;
    BreakpointType type;
    bool enabled;
    std::string module;
    std::string label;
    size_t hitCount;
    bool permanent;
    bool singleshoot;
};

class BreakpointsView : public QTableView
{
    Q_OBJECT

public:
    explicit BreakpointsView(QWidget* parent = nullptr);
    ~BreakpointsView() override;

    // Add/remove breakpoints
    void addBreakpoint(const BreakpointInfo& bp);
    void removeBreakpoint(uint64_t address);
    void clearAllBreakpoints();

    // Update breakpoint state
    void updateBreakpoint(uint64_t address, bool enabled);
    void setBreakpointHitCount(uint64_t address, size_t count);

    // Get selected breakpoint address
    uint64_t selectedBreakpointAddress() const;

    // Refresh display
    void refresh();

    // Check if breakpoint exists
    bool hasBreakpoint(uint64_t address) const;

signals:
    void breakpointSelected(uint64_t address);
    void breakpointToggled(uint64_t address, bool enabled);
    void breakpointDeleted(uint64_t address);
    void gotoBreakpointRequested(uint64_t address);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupColumns();
    void createContextMenu();
    void onToggleBreakpoint();
    void onDeleteBreakpoint();
    void onGotoBreakpoint();
    void onDeleteAllBreakpoints();

    QString typeToString(BreakpointType type) const;
    QString addressToString(uint64_t addr) const;

    QStandardItemModel* m_model = nullptr;
    QMenu* m_contextMenu = nullptr;
    QAction* m_toggleAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_gotoAction = nullptr;
    QAction* m_deleteAllAction = nullptr;
    uint64_t m_contextMenuAddress = 0;
};

} // namespace X64DbgLinux

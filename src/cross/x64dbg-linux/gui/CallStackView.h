#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <vector>
#include <cstdint>

namespace X64DbgLinux {

struct StackFrame {
    uint64_t address;      // Return address
    uint64_t frameAddr;    // Frame pointer (RBP)
    uint64_t stackAddr;    // Stack pointer (RSP) at this frame
    QString functionName;  // Function name (if symbol available)
    QString moduleName;    // Module name
    uint64_t offset;       // Offset within function
    int frameNumber;       // Frame number (0 = current)
};

class CallStackView : public QWidget {
    Q_OBJECT

public:
    explicit CallStackView(QWidget* parent = nullptr);
    ~CallStackView() override;

    void refresh();
    void setTargetPid(pid_t pid);

signals:
    void gotoAddressRequested(uint64_t addr);

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);
    void onDoubleClicked(const QModelIndex& index);
    void copyAddress();
    void gotoSelectedAddress();

private:
    void setupTable();
    void updateTable();
    std::vector<StackFrame> getCallStack();

    QTableWidget* m_table;
    pid_t m_targetPid = 0;
    QTimer* m_refreshTimer;
};

}

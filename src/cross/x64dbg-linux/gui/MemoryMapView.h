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

struct MemoryRegion {
    uint64_t start;
    uint64_t end;
    uint64_t size;
    QString permissions;
    uint64_t offset;
    QString device;
    uint64_t inode;
    QString pathname;
    QString type; // Code, Data, Heap, Stack, Library, etc.
};

class MemoryMapView : public QWidget {
    Q_OBJECT

public:
    explicit MemoryMapView(QWidget* parent = nullptr);
    ~MemoryMapView() override;

    void refresh();
    void setTargetPid(pid_t pid);

signals:
    void gotoAddressRequested(uint64_t addr);

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);
    void onDoubleClicked(const QModelIndex& index);
    void copyAddress();
    void copySelection();
    void gotoSelectedAddress();

private:
    void setupTable();
    void updateTable();
    std::vector<MemoryRegion> parseMemoryMap();
    QString getRegionType(const MemoryRegion& region) const;

    QTableWidget* m_table;
    pid_t m_targetPid = 0;
    QTimer* m_refreshTimer;
};

}

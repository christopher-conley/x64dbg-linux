#include "gui/MemoryMapView.h"
#include "Bridge.h"
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QSortFilterProxyModel>

namespace X64DbgLinux {

MemoryMapView::MemoryMapView(QWidget* parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_refreshTimer(new QTimer(this))
{
    setupTable();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    setLayout(layout);

    // Auto-refresh every 2 seconds when visible
    connect(m_refreshTimer, &QTimer::timeout, this, &MemoryMapView::refresh);
    m_refreshTimer->start(2000);
}

MemoryMapView::~MemoryMapView() = default;

void MemoryMapView::setupTable()
{
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Address"),
        tr("Size"),
        tr("Type"),
        tr("Protection"),
        tr("Offset"),
        tr("Module"),
        tr("Details")
    });

    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setSortingEnabled(true);

    // Set column widths
    m_table->setColumnWidth(0, 140); // Address
    m_table->setColumnWidth(1, 100); // Size
    m_table->setColumnWidth(2, 80);  // Type
    m_table->setColumnWidth(3, 80);  // Protection
    m_table->setColumnWidth(4, 100); // Offset
    m_table->setColumnWidth(5, 150); // Module

    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &MemoryMapView::onCustomContextMenuRequested);
    connect(m_table, &QTableWidget::doubleClicked,
            this, &MemoryMapView::onDoubleClicked);
}

void MemoryMapView::setTargetPid(pid_t pid)
{
    if(m_targetPid != pid) {
        m_targetPid = pid;
        refresh();
    }
}

void MemoryMapView::refresh()
{
    if(m_targetPid == 0) {
        m_table->setRowCount(0);
        return;
    }
    updateTable();
}

std::vector<MemoryRegion> MemoryMapView::parseMemoryMap()
{
    std::vector<MemoryRegion> regions;

    QString mapsPath = QString("/proc/%1/maps").arg(m_targetPid);
    QFile file(mapsPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return regions;
    }

    QTextStream in(&file);
    while(!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(' ', QString::SkipEmptyParts);
        if(parts.size() < 5) continue;

        // Parse address range
        QStringList addrRange = parts[0].split('-');
        if(addrRange.size() != 2) continue;

        bool ok;
        MemoryRegion region;
        region.start = addrRange[0].toULongLong(&ok, 16);
        if(!ok) continue;
        region.end = addrRange[1].toULongLong(&ok, 16);
        if(!ok) continue;
        region.size = region.end - region.start;

        // Parse permissions
        region.permissions = parts[1];

        // Parse offset
        region.offset = parts[2].toULongLong(&ok, 16);

        // Parse device
        region.device = parts[3];

        // Parse inode
        region.inode = parts[4].toULongLong();

        // Parse pathname (may contain spaces, so join remaining parts)
        if(parts.size() > 5) {
            QStringList pathParts;
            for(int i = 5; i < parts.size(); ++i) {
                pathParts.append(parts[i]);
            }
            region.pathname = pathParts.join(' ');
        }

        region.type = getRegionType(region);
        regions.push_back(region);
    }

    return regions;
}

QString MemoryMapView::getRegionType(const MemoryRegion& region) const
{
    // Determine region type based on content and permissions
    if(region.pathname.contains("[stack]")) {
        return tr("Stack");
    }
    if(region.pathname.contains("[heap]")) {
        return tr("Heap");
    }
    if(region.pathname.contains(".so") || region.pathname.contains(".so.")) {
        if(region.permissions.contains('x')) {
            return tr("Library Code");
        }
        return tr("Library Data");
    }
    if(region.pathname.contains("[vdso]")) {
        return tr("VDSO");
    }
    if(region.pathname.contains("[vvar]")) {
        return tr("VVAR");
    }
    if(region.pathname.contains("[vsyscall]")) {
        return tr("VSyscall");
    }
    if(region.pathname.isEmpty()) {
        if(region.permissions == "rw-p") {
            return tr("Private Data");
        }
        return tr("Anonymous");
    }
    if(region.permissions.contains('x')) {
        return tr("Code");
    }
    if(region.pathname.contains("[anon:")) {
        return tr("Anonymous");
    }

    return tr("Data");
}

void MemoryMapView::updateTable()
{
    auto regions = parseMemoryMap();

    // Block signals during update
    m_table->setSortingEnabled(false);
    m_table->setRowCount(static_cast<int>(regions.size()));

    for(size_t i = 0; i < regions.size(); ++i) {
        const auto& r = regions[i];
        const int row = static_cast<int>(i);

        // Address: 0xstart - 0xend
        auto* addrItem = new QTableWidgetItem(
            QString("%1 - %2")
                .arg(r.start, 0, 16)
                .arg(r.end, 0, 16)
        );
        addrItem->setData(Qt::UserRole, QVariant::fromValue(r.start));
        addrItem->setFont(QFont("Monospace", 9));
        m_table->setItem(row, 0, addrItem);

        // Size
        QString sizeStr;
        if(r.size >= 1024 * 1024 * 1024) {
            sizeStr = QString("%1 GB").arg(r.size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        } else if(r.size >= 1024 * 1024) {
            sizeStr = QString("%1 MB").arg(r.size / (1024.0 * 1024.0), 0, 'f', 2);
        } else if(r.size >= 1024) {
            sizeStr = QString("%1 KB").arg(r.size / 1024.0, 0, 'f', 2);
        } else {
            sizeStr = QString("%1 B").arg(r.size);
        }
        m_table->setItem(row, 1, new QTableWidgetItem(sizeStr));

        // Type
        auto* typeItem = new QTableWidgetItem(r.type);
        // Color code different types
        if(r.type == tr("Stack")) {
            typeItem->setForeground(QColor(0xE06C75)); // Red
        } else if(r.type == tr("Heap")) {
            typeItem->setForeground(QColor(0x98C379)); // Green
        } else if(r.type == tr("Code") || r.type == tr("Library Code")) {
            typeItem->setForeground(QColor(0x61AFEF)); // Blue
        }
        m_table->setItem(row, 2, typeItem);

        // Protection
        m_table->setItem(row, 3, new QTableWidgetItem(r.permissions));

        // Offset
        m_table->setItem(row, 4, new QTableWidgetItem(
            QString("0x%1").arg(r.offset, 0, 16)
        ));

        // Module/Pathname
        QString moduleName = r.pathname;
        if(!moduleName.isEmpty()) {
            // Extract just the filename for cleaner display
            int lastSlash = moduleName.lastIndexOf('/');
            if(lastSlash >= 0) {
                moduleName = moduleName.mid(lastSlash + 1);
            }
        }
        m_table->setItem(row, 5, new QTableWidgetItem(moduleName));

        // Details (full pathname or empty)
        m_table->setItem(row, 6, new QTableWidgetItem(r.pathname));
    }

    m_table->setSortingEnabled(true);
}

void MemoryMapView::onCustomContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);

    auto* gotoAction = menu.addAction(tr("Goto Address"));
    connect(gotoAction, &QAction::triggered, this, &MemoryMapView::gotoSelectedAddress);

    menu.addSeparator();

    auto* copyAddrAction = menu.addAction(tr("Copy Address"));
    connect(copyAddrAction, &QAction::triggered, this, &MemoryMapView::copyAddress);

    auto* copyRowAction = menu.addAction(tr("Copy Selection"));
    connect(copyRowAction, &QAction::triggered, this, &MemoryMapView::copySelection);

    menu.addSeparator();

    auto* refreshAction = menu.addAction(tr("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &MemoryMapView::refresh);

    menu.exec(m_table->mapToGlobal(pos));
}

void MemoryMapView::onDoubleClicked(const QModelIndex& index)
{
    if(!index.isValid()) return;

    int row = index.row();
    auto* item = m_table->item(row, 0);
    if(item) {
        uint64_t addr = item->data(Qt::UserRole).toULongLong();
        emit gotoAddressRequested(addr);
    }
}

void MemoryMapView::copyAddress()
{
    auto* item = m_table->currentItem();
    if(!item) return;

    int row = m_table->currentRow();
    auto* addrItem = m_table->item(row, 0);
    if(addrItem) {
        uint64_t addr = addrItem->data(Qt::UserRole).toULongLong();
        QString addrStr = QString("0x%1").arg(addr, 0, 16);
        QApplication::clipboard()->setText(addrStr);
    }
}

void MemoryMapView::copySelection()
{
    auto* item = m_table->currentItem();
    if(!item) return;

    QStringList parts;
    int row = m_table->currentRow();
    for(int col = 0; col < m_table->columnCount(); ++col) {
        auto* cell = m_table->item(row, col);
        if(cell) {
            parts.append(cell->text());
        }
    }
    QApplication::clipboard()->setText(parts.join("\t"));
}

void MemoryMapView::gotoSelectedAddress()
{
    auto* item = m_table->currentItem();
    if(!item) return;

    int row = m_table->currentRow();
    auto* addrItem = m_table->item(row, 0);
    if(addrItem) {
        uint64_t addr = addrItem->data(Qt::UserRole).toULongLong();
        emit gotoAddressRequested(addr);
    }
}

}

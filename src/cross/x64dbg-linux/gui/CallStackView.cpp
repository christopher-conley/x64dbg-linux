#include "gui/CallStackView.h"
#include "Bridge.h"
#include <QHeaderView>
#include <libunwind.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <cerrno>

namespace X64DbgLinux {

CallStackView::CallStackView(QWidget* parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_refreshTimer(new QTimer(this))
{
    setupTable();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    setLayout(layout);

    connect(m_refreshTimer, &QTimer::timeout, this, &CallStackView::refresh);
    m_refreshTimer->start(1000); // Refresh every second when visible
}

CallStackView::~CallStackView() = default;

void CallStackView::setupTable()
{
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        tr("Frame"),
        tr("Address"),
        tr("Function"),
        tr("Module"),
        tr("Offset")
    });

    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    m_table->setColumnWidth(0, 60);  // Frame
    m_table->setColumnWidth(1, 140); // Address
    m_table->setColumnWidth(2, 200); // Function
    m_table->setColumnWidth(3, 150); // Module
    m_table->setColumnWidth(4, 100); // Offset

    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &CallStackView::onCustomContextMenuRequested);
    connect(m_table, &QTableWidget::doubleClicked,
            this, &CallStackView::onDoubleClicked);
}

void CallStackView::setTargetPid(pid_t pid)
{
    if(m_targetPid != pid) {
        m_targetPid = pid;
        refresh();
    }
}

void CallStackView::refresh()
{
    if(m_targetPid == 0) {
        m_table->setRowCount(0);
        return;
    }
    updateTable();
}

std::vector<StackFrame> CallStackView::getCallStack()
{
    std::vector<StackFrame> frames;

    if(m_targetPid == 0)
        return frames;

    // Attach to target process to read its memory
    if(ptrace(PTRACE_ATTACH, m_targetPid, nullptr, nullptr) == -1)
        return frames;

    // Wait for process to stop
    int status;
    if(waitpid(m_targetPid, &status, 0) == -1) {
        ptrace(PTRACE_DETACH, m_targetPid, nullptr, nullptr);
        return frames;
    }

    // Read registers to get current RIP and RBP
    struct user_regs_struct regs;
    if(ptrace(PTRACE_GETREGS, m_targetPid, nullptr, &regs) == -1) {
        ptrace(PTRACE_DETACH, m_targetPid, nullptr, nullptr);
        return frames;
    }

    // Build stack trace by walking the frame pointers
    uint64_t rbp = regs.rbp;
    uint64_t rip = regs.rip;
    int frameNum = 0;

    // Current frame (frame 0)
    StackFrame currentFrame;
    currentFrame.address = rip;
    currentFrame.frameAddr = rbp;
    currentFrame.stackAddr = regs.rsp;
    currentFrame.functionName = "<current>";
    currentFrame.frameNumber = frameNum++;
    frames.push_back(currentFrame);

    // Walk the stack frames
    const int MAX_FRAMES = 64;
    while(rbp != 0 && frameNum < MAX_FRAMES) {
        // Read return address from [RBP + 8]
        uint64_t returnAddr = 0;
        errno = 0;
        long data = ptrace(PTRACE_PEEKDATA, m_targetPid, rbp + 8, nullptr);
        if(errno != 0) break;
        returnAddr = static_cast<uint64_t>(data);

        if(returnAddr == 0) break;

        // Read next frame pointer from [RBP]
        data = ptrace(PTRACE_PEEKDATA, m_targetPid, rbp, nullptr);
        if(errno != 0) break;
        uint64_t nextRbp = static_cast<uint64_t>(data);

        StackFrame frame;
        frame.address = returnAddr;
        frame.frameAddr = nextRbp;
        frame.stackAddr = rbp + 16;
        frame.functionName = QString("frame_%1").arg(frameNum);
        frame.frameNumber = frameNum++;
        frames.push_back(frame);

        rbp = nextRbp;
    }

    // Detach from target
    ptrace(PTRACE_DETACH, m_targetPid, nullptr, nullptr);

    return frames;
}

void CallStackView::updateTable()
{
    auto frames = getCallStack();
    m_table->setRowCount(static_cast<int>(frames.size()));

    for(size_t i = 0; i < frames.size(); ++i) {
        const auto& f = frames[i];
        int row = static_cast<int>(i);

        // Frame number
        auto* frameItem = new QTableWidgetItem(QString::number(f.frameNumber));
        frameItem->setTextAlignment(Qt::AlignCenter);
        if(f.frameNumber == 0) {
            frameItem->setBackground(QColor(0x61AFEF));
            frameItem->setForeground(Qt::black);
        }
        m_table->setItem(row, 0, frameItem);

        // Address
        auto* addrItem = new QTableWidgetItem(QString("0x%1").arg(f.address, 0, 16));
        addrItem->setFont(QFont("Monospace", 9));
        addrItem->setData(Qt::UserRole, QVariant::fromValue(f.address));
        m_table->setItem(row, 1, addrItem);

        // Function name
        m_table->setItem(row, 2, new QTableWidgetItem(f.functionName));

        // Module
        m_table->setItem(row, 3, new QTableWidgetItem(f.moduleName));

        // Offset
        auto* offsetItem = new QTableWidgetItem(QString("0x%1").arg(f.offset, 0, 16));
        offsetItem->setFont(QFont("Monospace", 9));
        m_table->setItem(row, 4, offsetItem);
    }
}

void CallStackView::onCustomContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);

    auto* gotoAction = menu.addAction(tr("Goto Address"));
    connect(gotoAction, &QAction::triggered, this, &CallStackView::gotoSelectedAddress);

    menu.addSeparator();

    auto* copyAddrAction = menu.addAction(tr("Copy Address"));
    connect(copyAddrAction, &QAction::triggered, this, &CallStackView::copyAddress);

    menu.addSeparator();

    auto* refreshAction = menu.addAction(tr("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &CallStackView::refresh);

    menu.exec(m_table->mapToGlobal(pos));
}

void CallStackView::onDoubleClicked(const QModelIndex& index)
{
    if(!index.isValid()) return;

    int row = index.row();
    auto* item = m_table->item(row, 1);
    if(item) {
        uint64_t addr = item->data(Qt::UserRole).toULongLong();
        emit gotoAddressRequested(addr);
    }
}

void CallStackView::copyAddress()
{
    auto* item = m_table->currentItem();
    if(!item) return;

    int row = m_table->currentRow();
    auto* addrItem = m_table->item(row, 1);
    if(addrItem) {
        QApplication::clipboard()->setText(addrItem->text());
    }
}

void CallStackView::gotoSelectedAddress()
{
    auto* item = m_table->currentItem();
    if(!item) return;

    int row = m_table->currentRow();
    auto* addrItem = m_table->item(row, 1);
    if(addrItem) {
        uint64_t addr = addrItem->data(Qt::UserRole).toULongLong();
        emit gotoAddressRequested(addr);
    }
}

}

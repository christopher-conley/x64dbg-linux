#include "CPUStack.h"

#include <vector>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>

#include <Gui/WordEditDialog.h>
#include <Memory/MemoryPage.h>
#include "Configuration.h"
#include "StringUtil.h"

CPUStack::CPUStack(Architecture* architecture, DbgAdapter* adapter, QWidget* parent)
    : HexDump(architecture, parent)
    , mAdapter(adapter)
{
    setWindowTitle("Stack");
    setShowHeader(false);

    mStackReturnToColor = ConfigColor("StackReturnToColor");

    setupColumns();
    setupContextMenu();

    connect(mAdapter, &DbgAdapter::registersUpdated,
            this, &CPUStack::onRegistersUpdated,
            Qt::QueuedConnection);

    connect(mAdapter, &DbgAdapter::processCreated,
            this, &CPUStack::onProcessStarted,
            Qt::QueuedConnection);
}

void CPUStack::setupColumns()
{
    const int charwidth = getCharWidth();

    mForceColumn = 1;

    ColumnDescriptor colDesc{};

    colDesc.isData = true;
    colDesc.itemCount = 1;
    colDesc.separator = 0;
    if constexpr(sizeof(duint) == 8)
    {
        colDesc.data.itemSize = Qword;
        colDesc.data.qwordMode = HexQword;
    }
    else
    {
        colDesc.data.itemSize = Dword;
        colDesc.data.dwordMode = HexDword;
    }
    appendDescriptor(24 + charwidth * 2 * sizeof(duint), "void*", false, colDesc);

    colDesc.isData = false;
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    appendDescriptor(2000, tr("Comments"), false, colDesc);
}

void CPUStack::setupContextMenu()
{
    mContextMenu = new QMenu(this);

    mRealignAction = mContextMenu->addAction(tr("Align Stack Pointer"), this, &CPUStack::realignSlot);
    mModifyAction = mContextMenu->addAction(tr("Modify"), this, &CPUStack::modifySlot);

    mContextMenu->addSeparator();

    mGotoCspAction = mContextMenu->addAction(tr("Go to RSP"), this, &CPUStack::gotoCspSlot);
    mGotoCbpAction = mContextMenu->addAction(tr("Go to RBP"), this, &CPUStack::gotoCbpSlot);

    mContextMenu->addSeparator();

    mFollowDisasmAction = mContextMenu->addAction(tr("Follow in Disassembly"), this, &CPUStack::followDisasmSlot);

    QMenu* copyMenu = mContextMenu->addMenu(tr("&Copy"));
    copyMenu->addAction(tr("&Selection"), this, &HexDump::copySelectionSlot);
    copyMenu->addAction(tr("&Address"), this, &HexDump::copyAddressSlot);
    const QString ptrName = sizeof(duint) == 8 ? tr("&QWORD") : tr("&DWORD");
    copyMenu->addAction(ptrName, this, &CPUStack::copyPtrColumnSlot);
    copyMenu->addAction(tr("&Comments"), this, &CPUStack::copyCommentsColumnSlot);

    mContextMenu->addSeparator();

    mFreezeAction = mContextMenu->addAction(tr("Freeze Stack"), this, &CPUStack::freezeStackSlot);
    mFreezeAction->setCheckable(true);
}

void CPUStack::refreshActionState() const
{
    const bool debugging = mCsp != 0;

    mRealignAction->setEnabled(debugging && (mCsp & (sizeof(duint) - 1)) != 0);
    mModifyAction->setEnabled(debugging);
    mGotoCspAction->setEnabled(debugging);
    mGotoCbpAction->setEnabled(debugging && mCbp != 0);
    mFollowDisasmAction->setEnabled(debugging);
    mFreezeAction->setEnabled(debugging);
    mFreezeAction->setChecked(mStackFrozen);
}

void CPUStack::stackDumpAt(const duint addr, const duint csp)
{
    mCsp = csp;

    // Window the page around addr so scrolling a few rows doesn't refetch.
    constexpr duint kWindowHalf = 0x1000;
    const duint windowBase = addr > kWindowHalf ? addr - kWindowHalf : 0;
    mMemPage->setAttributes(windowBase, kWindowHalf * 2);

    printDumpAt(addr, true, true, true);
}

void CPUStack::onRegistersUpdated(const REGDUMP & regs)
{
    mCbp = regs.regcontext.cbp;

    if(mStackFrozen && regs.regcontext.csp != 0)
    {
        // Keep mCsp live so "Go to RSP" still targets the live value.
        mCsp = regs.regcontext.csp;
        return;
    }

    stackDumpAt(regs.regcontext.csp, regs.regcontext.csp);
}

void CPUStack::onProcessStarted()
{
    mStackFrozen = false;
    if(mFreezeAction)
        mFreezeAction->setChecked(false);
    if(mCsp != 0)
        stackDumpAt(mCsp, mCsp);
}

void CPUStack::gotoCspSlot()
{
    if(mCsp != 0)
        stackDumpAt(mCsp, mCsp);
}

void CPUStack::gotoCbpSlot()
{
    if(mCbp != 0)
        stackDumpAt(mCbp, mCsp);
}

void CPUStack::followDisasmSlot()
{
    const duint selection = getInitialSelection();
    const duint alignedRva = selection - (selection % sizeof(duint));

    duint ptr = 0;
    if(!mMemPage->read(&ptr, alignedRva, sizeof(duint)))
        return;

    if(!mAdapter->isCodePtr(ptr))
        return;

    emit followDisasmRequested(ptr);
}

void CPUStack::freezeStackSlot()
{
    mStackFrozen = !mStackFrozen;
    mFreezeAction->setChecked(mStackFrozen);
}

void CPUStack::realignSlot()
{
    const duint aligned = mCsp & ~static_cast<duint>(sizeof(duint) - 1);
    mAdapter->writeRegister("csp", aligned);
}

void CPUStack::modifySlot()
{
    const duint selection = getInitialSelection();
    const duint alignedRva = selection - (selection % sizeof(duint));

    duint currentValue = 0;
    if(!mMemPage->read(&currentValue, alignedRva, sizeof(duint)))
        return;

    WordEditDialog editDialog(this);
    editDialog.setup(tr("Modify"), currentValue, sizeof(duint));
    if(editDialog.exec() != QDialog::Accepted)
        return;

    const duint newValue = editDialog.getVal();
    mMemPage->write(&newValue, alignedRva, sizeof(duint));
    reloadData();
}

void CPUStack::copyPtrColumnSlot() const
{
    constexpr duint wordSize = sizeof(duint);
    const dsint selStart = getSelectionStart();
    const dsint selLen = getSelectionEnd() - selStart + 1;
    const duint wordCount = selLen / wordSize;
    if(wordCount == 0)
        return;

    std::vector<duint> data(wordCount);
    mMemPage->read(data.data(), selStart, wordCount * wordSize);

    QString clipboard;
    for(duint i = 0; i < wordCount; i++)
    {
        if(i > 0)
            clipboard += "\r\n";
        clipboard += ToPtrString(data[i]);
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUStack::copyCommentsColumnSlot() const
{
    constexpr duint wordSize = sizeof(duint);
    const dsint selStart = getSelectionStart();
    const dsint selLen = getSelectionEnd() - selStart + 1;

    QString clipboard;
    for(dsint i = 0; i < selLen; i += wordSize)
    {
        constexpr duint commentsColumn = 2;
        RichTextPainter::List richText;
        getColumnRichText(commentsColumn, selStart + i, richText);
        QString colText;
        for(const auto & r : richText)
            colText += r.text;

        if(i > 0)
            clipboard += "\r\n";
        clipboard += colText;
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    refreshActionState();
    mContextMenu->popup(event->globalPos());
}

void CPUStack::mouseDoubleClickEvent(QMouseEvent* event)
{
    HexDump::mouseDoubleClickEvent(event);

    if(event->button() != Qt::LeftButton || mCsp == 0)
        return;

    // Column 0 is the auto-rendered address; 1 is the first user descriptor (void*).
    if(getColumnIndexFromX(event->pos().x()) != 1)
        return;

    const duint selection = getInitialSelection();
    const duint alignedRva = selection - (selection % sizeof(duint));
    duint ptr = 0;
    if(!mMemPage->read(&ptr, alignedRva, sizeof(duint)))
        return;

    if(mAdapter->isCodePtr(ptr))
        emit followDisasmRequested(ptr);
}

QString CPUStack::paintContent(QPainter* painter, const duint row, const duint column, const int x, const int y,
                               const int w, const int h)
{
    if(column == 0 && mCsp != 0)
    {
        const auto bytePerRowCount = getBytePerRowCount();
        const dsint rva = static_cast<dsint>(row) * static_cast<dsint>(bytePerRowCount) - mByteOffset;
        const duint rowVa = rvaToVa(static_cast<duint>(rva));
        if(rowVa == mCsp)
        {
            const QColor background = ConfigColor("StackCspBackgroundColor");
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background));
        }
    }

    return HexDump::paintContent(painter, row, column, x, y, w, h);
}

void CPUStack::getColumnRichText(const duint col, const duint rva, RichTextPainter::List & richText) const
{
    // Mirrors src/gui/Src/Gui/CPUStack.cpp::getColumnRichText.
    const duint va = rvaToVa(rva);
    const bool activeStack = (va >= mCsp);

    RichTextPainter::CustomRichText_t curData;
    curData.underline = false;
    curData.flags = RichTextPainter::FlagColor;
    curData.textColor = mTextColor;

    if(col && mDescriptor.at(col - 1).isData)
    {
        HexDump::getColumnRichText(col, rva, richText);
        if(!activeStack)
        {
            const QColor inactiveColor = ConfigColor("StackInactiveTextColor");
            for(auto & rt : richText)
            {
                rt.flags = RichTextPainter::FlagColor;
                rt.textColor = inactiveColor;
            }
        }
        return;
    }

    QString commentText;
    bool isReturnTo = false;
    if(col && resolveSlotComment(rva, commentText, isReturnTo))
    {
        if(activeStack)
            curData.textColor = isReturnTo ? mStackReturnToColor : mTextColor;
        else
            curData.textColor = ConfigColor("StackInactiveTextColor");
        curData.text = commentText;
        richText.push_back(curData);
        return;
    }

    HexDump::getColumnRichText(col, rva, richText);
}

bool CPUStack::resolveSlotComment(const duint rva, QString & out, bool & isReturnTo) const
{
    isReturnTo = false;

    duint ptr = 0;
    if(!mMemPage->read(&ptr, rva, sizeof(duint)))
        return false;

    char modName[MAX_MODULE_SIZE] = {};
    if(!DbgFunctions()->ModNameFromAddr(ptr, modName, false))
        return false;

    duint fromAddr = 0;
    isReturnTo = DbgResolveReturnTo(ptr, &fromAddr);

    const QString mod = QString::fromUtf8(modName);
    const QString addr = QString("%1").arg(ptr, sizeof(duint) * 2, 16, QLatin1Char('0')).toUpper();

    if(!isReturnTo)
    {
        out = QString("%1.%2").arg(mod, addr);
        return true;
    }

    QString fromPart = QStringLiteral("???");
    if(fromAddr != 0)
    {
        const QString fromAddrStr = QString("%1").arg(fromAddr, sizeof(duint) * 2, 16, QLatin1Char('0')).toUpper();
        fromPart = fromAddrStr;
        char fromModName[MAX_MODULE_SIZE] = {};
        if(DbgFunctions()->ModNameFromAddr(fromAddr, fromModName, false))
            fromPart = QString("%1.%2").arg(QString::fromUtf8(fromModName), fromAddrStr);
    }

    out = QString("return to %1.%2 from %3").arg(mod, addr, fromPart);
    return true;
}

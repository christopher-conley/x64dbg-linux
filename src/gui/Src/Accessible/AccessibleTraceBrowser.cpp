// This file implements accessibility interface for Disassembly
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleTraceBrowser.h"
#include "Tracer/TraceBrowser.h"
#include "Bridge.h"

AccessibleTraceBrowser::AccessibleTraceBrowser(QWidget* w) : AccessibleAbstractTableView(w)
{
}

AccessibleTraceBrowser::~AccessibleTraceBrowser()
{
}

TraceBrowser* AccessibleTraceBrowser::dis() const
{
    return dynamic_cast<TraceBrowser*>(m_tableView);
}

bool AccessibleTraceBrowser::isRowSelected(int row) const
{
    // row includes title
    if(row == 0)
        return false;
    auto dis = this->dis();
    return dis->accessibilitySelectedRow() == row - 1;
}

// TODO: multi-selection
int AccessibleTraceBrowser::selectedRowCount() const
{
    // row includes title
    auto dis = this->dis();
    dsint sel = dis->getInitialSelection() - dis->getTableOffset();
    if(sel >= 0 && sel <= dis->getViewableRowsCount())
        return 1;
    else
        return 0;
}

QList<int> AccessibleTraceBrowser::selectedRows() const
{
    auto dis = this->dis();
    int selectedRow = dis->accessibilitySelectedRow();
    if(selectedRow != -1)
        return QList<int>({ selectedRow });
    else
        return QList<int>();
}

int AccessibleTraceBrowser::selectedCellCount() const
{
    return selectedRowCount();
}

QList<QAccessibleInterface*> AccessibleTraceBrowser::selectedCells() const
{
    auto dis = this->dis();
    int selectedRow = dis->accessibilitySelectedRow();
    if(selectedRow != -1)
        return QList<QAccessibleInterface*>({ cellAt(selectedRow, selectedColumns().first()) });
    else
        return QList<QAccessibleInterface*>();
}

static QString getDisassemblyMnemonicBrief(const Instruction_t & inst)
{
    char brief[MAX_STRING_SIZE] = "";
    QString mnem;
    for(const ZydisTokenizer::SingleToken & token : inst.tokens.tokens)
    {
        if(token.type != ZydisTokenizer::TokenType::Space && token.type != ZydisTokenizer::TokenType::Prefix)
        {
            mnem = token.text;
            break;
        }
    }
    if(mnem.isEmpty())
        mnem = inst.instStr;

    int index = mnem.indexOf(' ');
    if(index != -1)
        mnem.truncate(index);
    DbgFunctions()->GetMnemonicBrief(mnem.toUtf8().constData(), MAX_STRING_SIZE, brief);
    return QString::fromUtf8(brief);
}

QString AccessibleTraceBrowser::getCellContent(int row, int col) const
{
    TraceBrowser & d = *dis();
    TRACEINDEX index = row + d.getTableOffset();
    QString reason;
    auto & traceFile = *d.mTraceFile;
    Instruction_t inst = traceFile.Instruction(index);
    REGDUMP reg;
    reg = traceFile.Registers(index);
    duint cur_addr = reg.regcontext.cip;
    if(traceFile.isError(reason))
    {
        return QString();
    }
    switch(col)
    {
    case TraceBrowser::TableColumnIndex::Index:
        return traceFile.getIndexText(index);
    case TraceBrowser::TableColumnIndex::Address:
    {
        return d.getAddrText(cur_addr, nullptr, true);
    }
    case TraceBrowser::TableColumnIndex::Opcode:
    {
        QString bytes;
        RichTextPainter::htmlRichText(d.getRichBytes(inst), nullptr, bytes);
        return bytes;
    }
    case TraceBrowser::TableColumnIndex::Disassembly:
    {
        return inst.tokens.toString();
    }
    case TraceBrowser::TableColumnIndex::Registers:
    {
        return d.registersTokens(index).toString();
    }
    case TraceBrowser::TableColumnIndex::Memory:
    {
        return d.memoryTokens(index).toString();
    }
    case TraceBrowser::TableColumnIndex::Comments:
    {
        QString comment;
        bool autocomment;
        GetCommentFormat(cur_addr, comment, &autocomment);
        if(autocomment || comment.isEmpty())  // prefer label over auto-comment
        {
            char label[MAX_LABEL_SIZE];
            if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label))
            {
                comment = QString::fromUtf8(label);
            }
        }
        if(d.mShowMnemonicBrief)
        {
            comment += ' ' + getDisassemblyMnemonicBrief(inst);
        }
        return comment;
    }
    default:
        return QString();
    }
}

#endif
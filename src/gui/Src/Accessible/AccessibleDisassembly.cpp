// This file implements accessibility interface for Disassembly
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleDisassembly.h"
#include "Disassembly.h"
#include "Bridge.h"

AccessibleDisassembly::AccessibleDisassembly(QWidget* w) : AccessibleAbstractTableView(w)
{
}

AccessibleDisassembly::~AccessibleDisassembly()
{
}

Disassembly* AccessibleDisassembly::dis() const
{
    return dynamic_cast<Disassembly*>(m_tableView);
}

bool AccessibleDisassembly::isRowSelected(int row) const
{
    // row includes title
    auto dis = this->dis();
    try
    {
        return dis->getInitialSelection() == dis->mInstBuffer.at(row - 1).rva;
    }
    catch(std::out_of_range)
    {
        return false;
    }
}

// TODO: multi-selection
int AccessibleDisassembly::selectedRowCount() const
{
    // row includes title
    auto dis = this->dis();
    const auto & inst = dis->mInstBuffer;
    auto sel = dis->getInitialSelection();
    if(sel >= inst.first().rva && sel <= inst.back().rva)
        return 1;
    else
        return 0;
}

static int findFirstSelection(duint sel, const QList<Instruction_t> & inst)
{
    if(sel >= inst.first().rva && sel <= inst.back().rva)
    {
        for(int i = 0; i < inst.size(); i++)
        {
            if(inst[i].rva == sel)
            {
                return i + 1;
            }
        }
    }
    return -1;
}

QList<int> AccessibleDisassembly::selectedRows() const
{
    auto dis = this->dis();
    int selectedRow = findFirstSelection(dis->getInitialSelection(), dis->mInstBuffer);
    if(selectedRow != -1)
        return QList<int>({ selectedRow });
    else
        return QList<int>();
}

int AccessibleDisassembly::selectedCellCount() const
{
    return selectedRowCount();
}

QList<QAccessibleInterface*> AccessibleDisassembly::selectedCells() const
{
    auto dis = this->dis();
    int selectedRow = findFirstSelection(dis->getInitialSelection(), dis->mInstBuffer);
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

QString AccessibleDisassembly::getCellContent(int row, int col) const
{
    const Disassembly & d = *dis();
    const Instruction_t & inst = d.mInstBuffer.at(row);
    duint cur_addr = d.rvaToVa(inst.rva);
    switch(col)
    {
    case Disassembly::ColAddress:
    {
        QString label;
        return d.getAddrText(cur_addr, label, true);
    }
    case Disassembly::ColBytes:
    {
        QString bytes;
        RichTextPainter::htmlRichText(d.getRichBytes(inst, false), nullptr, bytes);
        return bytes;
    }
    case Disassembly::ColDisassembly:
    {
        QString disassembly;
        for(const auto & token : inst.tokens.tokens)
            disassembly += token.text;
        return disassembly;
    }
    case Disassembly::ColMnemonicBrief:
    {
        if(d.mShowMnemonicBrief)
            return getDisassemblyMnemonicBrief(inst);
        else
            return QString();
    }
    case Disassembly::ColComment:
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
        if(d.mShowMnemonicBrief && d.getColumnHidden(Disassembly::ColMnemonicBrief))
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
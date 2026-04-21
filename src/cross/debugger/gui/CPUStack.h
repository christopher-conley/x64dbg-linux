#pragma once

#include <BasicView/HexDump.h>
#include "core/DbgAdapter.h"

class QMenu;
class QAction;
class QContextMenuEvent;
class QMouseEvent;
class QPainter;

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    CPUStack(Architecture* architecture, DbgAdapter* adapter, QWidget* parent = nullptr);

    QString paintContent(QPainter* painter, duint row, duint column, int x, int y, int w, int h) override;

signals:
    void followDisasmRequested(duint addr);

public slots:
    void stackDumpAt(duint addr, duint csp);
    void onRegistersUpdated(const REGDUMP & regs);
    void onProcessStarted();
    void gotoCspSlot();
    void gotoCbpSlot();
    void followDisasmSlot();
    void freezeStackSlot();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupColumns();
    void setupContextMenu();
    void refreshActionState();

    DbgAdapter* mAdapter = nullptr;
    duint mCsp = 0;
    duint mCbp = 0;
    bool mStackFrozen = false;

    QMenu* mContextMenu = nullptr;
    QAction* mGotoCspAction = nullptr;
    QAction* mGotoCbpAction = nullptr;
    QAction* mFollowDisasmAction = nullptr;
    QAction* mCopyAddressAction = nullptr;
    QAction* mFreezeAction = nullptr;
};

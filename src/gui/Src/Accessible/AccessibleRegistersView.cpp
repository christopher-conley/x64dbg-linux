// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleRegistersView.h"
#include "StringUtil.h"

AccessibleRegistersViewItem::AccessibleRegistersViewItem(AccessibleRegistersView* parent, RegistersView::REGISTER_NAME id) : mParent(parent), id(id)
{
}

QString AccessibleRegistersViewItem::text(QAccessible::Text t) const
{
    RegistersView* w = mParent->m_registersView;
    switch(t)
    {
    case QAccessible::Name:
    case QAccessible::Value:
        if(w->mLABELDISPLAY.contains(id))
            return QString(w->mRegisterMapping[id]) + " = " + w->GetRegStringValueFromValue(id, w->registerValue(&w->mRegDumpStruct, id)) + ' ' + w->getRegisterLabel(id);
        return QString(w->mRegisterMapping[id]) + " = " + w->GetRegStringValueFromValue(id, w->registerValue(&w->mRegDumpStruct, id));
    case QAccessible::Help:
        return w->helpRegister(id);
    default:
        return QString();
    }
}

QColor AccessibleRegistersViewItem::foregroundColor() const
{
    if(mParent->m_registersView->mRegisterUpdates.contains(id))
    {
        return ConfigColor("RegistersModifiedColor");
    }
    else
    {
        return ConfigColor("RegistersColor");
    }
}

int AccessibleRegistersViewItem::childCount() const
{
    return 0;
}

QWindow* AccessibleRegistersViewItem::window() const
{
    return mParent->window();
}

QAccessibleInterface* AccessibleRegistersViewItem::parent() const
{
    return mParent;
}

QAccessibleInterface* AccessibleRegistersViewItem::child(int index) const
{
    return nullptr;
}

int AccessibleRegistersViewItem::indexOfChild(const QAccessibleInterface* child) const
{
    return -1;
}

QAccessible::Role AccessibleRegistersViewItem::role() const
{
    return QAccessible::ListItem;
}

QAccessible::State AccessibleRegistersViewItem::state() const
{
    QAccessible::State state;
    const RegistersView* parent = mParent->m_registersView;
    state.focusable = parent->isActive;
    state.active = parent->isActive;
    state.selectable = parent->isActive;
    if(parent->mSelected == id)
    {
        state.selected = true;
        if(parent->hasFocus())
            state.focused = true;
    }
    return state;
}

QAccessibleInterface* AccessibleRegistersViewItem::childAt(int x, int y) const
{
    return nullptr;
}

QObject* AccessibleRegistersViewItem::object() const
{
    return nullptr;
}

void AccessibleRegistersViewItem::setText(QAccessible::Text t, const QString & text)
{
}

QRect AccessibleRegistersViewItem::rect() const
{
    QRect rect;
    const RegistersView* parent = mParent->m_registersView;
    const auto & it = parent->mRegisterPlaces.constFind(id);
    if(it == parent->mRegisterPlaces.cend())
    {
        return rect;
    }
    int top, bottom, left, right;
    top = it.value().line * parent->mRowHeight + parent->yTopSpacing;
    bottom = top + parent->mRowHeight;
    // These registers occupy a whole line
    if(it.key() >= RegistersView::CAX && it.key() <= RegistersView::EFLAGS
            || it.key() >= RegistersView::MM0 && it.key() <= RegistersView::MM7
            || it.key() >= RegistersView::DR0 && it.key() <= RegistersView::DR7
            || it.key() >= RegistersView::K0 && it.key() <= RegistersView::K7
            || it.key() >= RegistersView::XMM0 && it.key() <= ArchValue(RegistersView::XMM7, RegistersView::XMM31))
    {
        const QWidget* upperScrollArea = (const QWidget*)parent->parentWidget()->parentWidget();
        left = 0;
        right = upperScrollArea->width();
    }
    else
    {
        left = (1 + it.value().start) * parent->mCharWidth;
        right = left + ((it.value().labelwidth + it.value().valuesize) * parent->mCharWidth);
    }
    const QPoint TL = parent->mapToGlobal(QPoint(left, top));
    const QPoint BR = parent->mapToGlobal(QPoint(right, bottom));
    return QRect(TL, BR);
}

bool AccessibleRegistersViewItem::isValid() const
{
    return mParent->m_registersView->isActive;
}

AccessibleRegistersView::AccessibleRegistersView(QWidget* w) : QAccessibleWidget(w, QAccessible::List, dynamic_cast<RegistersView*>(w)->accessibleName())
{
    m_registersView = dynamic_cast<RegistersView*>(w);
    for(int i = 0; i < interfaces.size(); i++)
    {
        interfaces[i] = QAccessible::registerAccessibleInterface(new AccessibleRegistersViewItem(this, (RegistersView::REGISTER_NAME)i));
    }
}

AccessibleRegistersView::~AccessibleRegistersView()
{
    for(const auto & id : interfaces)
    {
        if(id != 0)
            QAccessible::deleteAccessibleInterface(id);
    }
}

int AccessibleRegistersView::childCount() const
{
    // TODO: interact with showFPU
    return m_registersView->mAVX512RegistersShown ? RegistersView::REGISTER_NAME::UNKNOWN : ArchValue(RegistersView::REGISTER_NAME::XMM7, RegistersView::REGISTER_NAME::XMM16);
}

QAccessibleInterface* AccessibleRegistersView::child(int index) const
{
    if(index >= 0 && index < childCount())
    {
        auto & id = interfaces[index];
        if(id == 0)
        {
            id = QAccessible::registerAccessibleInterface(new AccessibleRegistersViewItem(const_cast<AccessibleRegistersView*>(this), (RegistersView::REGISTER_NAME)index));
        }
        return QAccessible::accessibleInterface(id);
    }
    else
        return nullptr;
}

QAccessibleInterface* AccessibleRegistersView::childAt(int x, int y) const
{
    RegistersView::REGISTER_NAME clickedReg;
    QPoint local = m_registersView->mapFromGlobal(QPoint(x, y));
    if(m_registersView->identifyRegister((local.y() - m_registersView->yTopSpacing) / (double)m_registersView->mRowHeight, local.x() / (double)m_registersView->mCharWidth, &clickedReg))
        if(clickedReg < RegistersView::UNKNOWN)
            return child(static_cast<int>(clickedReg));
    return nullptr;
}

int AccessibleRegistersView::indexOfChild(const QAccessibleInterface* child) const
{
    for(int i = 0; i < childCount(); i++)
    {
        const QAccessibleInterface* a = this->child(i);
        if(a == child)
        {
            return i;
        }
    }
    return -1;
}

QAccessibleInterface* AccessibleRegistersView::focusChild() const
{
    if(m_registersView->mSelected < RegistersView::UNKNOWN)
        return child(m_registersView->mSelected);
    else
        return (QAccessibleInterface*)this;
}

bool AccessibleRegistersView::isValid() const
{
    return m_registersView->isActive;
}

QAccessible::State AccessibleRegistersView::state() const
{
    QAccessible::State state;
    state.focusable = true;
    if(m_registersView->hasFocus())
        state.focused = true;
    state.active = m_registersView->isActive;
    state.multiLine = true;
    state.multiSelectable = false;
    state.disabled = !m_registersView->isActive;
    state.hasPopup = true;
    return state;
}

QRect AccessibleRegistersView::rect() const
{
    QRect rect;
    const QScrollArea* upperScrollArea = (const QScrollArea*)m_registersView->parentWidget()->parentWidget();
    int top, bottom, left, right;
    top = 0;
    bottom = upperScrollArea->height();
    left = 0;
    right = upperScrollArea->width();
    const QPoint TL = upperScrollArea->mapToGlobal(QPoint(left, top));
    const QPoint BR = upperScrollArea->mapToGlobal(QPoint(right, bottom));
    return QRect(TL, BR);
}
#endif

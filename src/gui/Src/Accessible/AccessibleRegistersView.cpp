// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleRegistersView.h"

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
    state.focusable = mParent->m_registersView->isActive;
    state.active = mParent->m_registersView->isActive;
    state.selectable = mParent->m_registersView->isActive;
    if(mParent->m_registersView->mSelected == id)
    {
        state.selected = true;
        if(mParent->m_registersView->hasFocus())
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
    return QRect();
}

bool AccessibleRegistersViewItem::isValid() const
{
    return mParent->m_registersView->isActive;
}

AccessibleRegistersView::AccessibleRegistersView(QWidget* w) : QAccessibleWidget(w, QAccessible::List, dynamic_cast<RegistersView*>(w)->accessibleName())
{
    m_registersView = dynamic_cast<RegistersView*>(w);
    interfaces.fill(0);
}

AccessibleRegistersView::~AccessibleRegistersView()
{
    for(auto & i : interfaces)
    {
        if(i != 0)
        {
            QAccessible::deleteAccessibleInterface(i);
        }
    }
}

int AccessibleRegistersView::childCount() const
{
    return RegistersView::REGISTER_NAME::UNKNOWN;
}

QAccessibleInterface* AccessibleRegistersView::child(int index) const
{
    if(index >= 0 && index < childCount())
    {
        if(interfaces[index] != 0)
        {
            return QAccessible::accessibleInterface(interfaces[index]);
        }
        else
        {
            auto child = new AccessibleRegistersViewItem(const_cast<AccessibleRegistersView*>(this), (RegistersView::REGISTER_NAME)index);
            interfaces[index] = QAccessible::registerAccessibleInterface(child);
            return child;
        }
    }
    else
        return nullptr;
}

QAccessibleInterface* AccessibleRegistersView::childAt(int x, int y) const
{
    RegistersView::REGISTER_NAME clickedReg;
    QPoint local = m_registersView->mapFromGlobal(QPoint(x, y));
    if(m_registersView->identifyRegister(local.y(), local.x(), &clickedReg))
        if(clickedReg < RegistersView::UNKNOWN)
            return child(static_cast<int>(clickedReg));
    return (QAccessibleInterface*)this;
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

QAccessible::Role AccessibleRegistersView::role() const
{
    return QAccessible::List;
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
#endif
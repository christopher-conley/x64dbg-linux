// This file implements accessibility interface for RegistersView
#ifndef QT_NO_ACCESSIBILITY
#include "AccessibleRegistersView.h"
#include "StringUtil.h"

static QRect widgetGlobalRect(const QWidget* widget)
{
    if(!widget)
        return QRect();
    return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

static QRect registersViewportGlobalRect(const RegistersView* view)
{
    return widgetGlobalRect(view ? view->viewport() : nullptr);
}

static bool isFullLineRegister(RegistersView::REGISTER_NAME reg)
{
    return (reg >= RegistersView::CAX && reg <= RegistersView::EFLAGS)
           || (reg >= RegistersView::MM0 && reg <= RegistersView::MM7)
           || (reg >= RegistersView::DR0 && reg <= RegistersView::DR7)
           || (reg >= RegistersView::K0 && reg <= RegistersView::K7)
           || (reg >= RegistersView::XMM0 && reg <= ArchValue(RegistersView::XMM7, RegistersView::XMM31));
}

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
    {
        const auto it = w->mRegisterMapping.constFind(id);
        if(it == w->mRegisterMapping.cend())
            return QString();
        if(w->mLABELDISPLAY.contains(id))
            return QString(it.value()) + " = " + w->GetRegStringValueFromValue(id, w->registerValue(&w->mRegDumpStruct, id)) + ' ' + w->getRegisterLabel(id);
        return QString(it.value()) + " = " + w->GetRegStringValueFromValue(id, w->registerValue(&w->mRegDumpStruct, id));
    }
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
    const bool visible = parent->mRegisterPlaces.contains(id);
    state.focusable = parent->isActive && visible;
    state.active = parent->isActive;
    state.selectable = parent->isActive && visible;
    state.invisible = !visible;
    if(visible && parent->mSelected == id)
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
    const RegistersView* parent = mParent->m_registersView;
    const QWidget* contentWidget = parent ? parent->widget() : nullptr;
    if(!parent || !contentWidget)
        return QRect();

    const auto it = parent->mRegisterPlaces.constFind(id);
    if(it == parent->mRegisterPlaces.cend())
        return QRect();

    int ySpace = parent->yTopSpacing;
    if(parent->mVScrollOffset != 0)
        ySpace = 0;
    const int top = parent->mRowHeight * (it.value().line + parent->mVScrollOffset) + ySpace;

    int left = 0;
    int right = 0;
    if(isFullLineRegister(it.key()))
    {
        right = contentWidget->width();
    }
    else
    {
        left = (1 + it.value().start) * parent->mCharWidth;
        right = left + ((it.value().labelwidth + it.value().valuesize) * parent->mCharWidth);
    }

    QRect itemRect(QPoint(left, top), QSize(right - left, parent->mRowHeight));
    QRect globalRect(contentWidget->mapToGlobal(itemRect.topLeft()), itemRect.size());
    return globalRect.intersected(registersViewportGlobalRect(parent));
}

bool AccessibleRegistersViewItem::isValid() const
{
    return mParent->m_registersView->isActive && mParent->m_registersView->mRegisterPlaces.contains(id);
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
    int maxRegister = -1;
    for(auto it = m_registersView->mRegisterPlaces.cbegin(); it != m_registersView->mRegisterPlaces.cend(); ++it)
    {
        const int reg = static_cast<int>(it.key());
        if(reg > maxRegister)
            maxRegister = reg;
    }
    return maxRegister + 1;
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
    const QWidget* contentWidget = m_registersView ? m_registersView->widget() : nullptr;
    if(!m_registersView || !contentWidget)
        return nullptr;

    const QPoint globalPos(x, y);
    if(!registersViewportGlobalRect(m_registersView).contains(globalPos))
        return nullptr;

    RegistersView::REGISTER_NAME clickedReg = RegistersView::UNKNOWN;
    QPoint local = contentWidget->mapFromGlobal(globalPos);
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
    return widgetGlobalRect(m_registersView);
}
#endif

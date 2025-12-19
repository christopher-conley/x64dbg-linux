#ifndef QT_NO_ACCESSIBILITY
#include "Accessible.h"
#include "AccessibleRegistersView.h"

QAccessibleInterface* accessibleInterfaceFactory(const QString & classname, QObject* object)
{
    QAccessibleInterface* ptr = nullptr;
    if((classname == "RegistersView") && object && dynamic_cast<RegistersView*>(object) != nullptr)
    {
        ptr = new AccessibleRegistersView(dynamic_cast<QWidget*>(object));
    }
    return ptr;
}

#endif
#ifndef QT_NO_ACCESSIBILITY
#include "Accessible.h"
#include "AccessibleRegistersView.h"
#include "AccessibleAbstractTableView.h"

QAccessibleInterface* accessibleInterfaceFactory(const QString & classname, QObject* object)
{
    QAccessibleInterface* ptr = nullptr;
    if((classname == "CPUDisassembly") && object && dynamic_cast<AbstractTableView*>(object) != nullptr)
    {
        ptr = new AccessibleAbstractTableView(dynamic_cast<QWidget*>(object));
    }
    if((classname == "RegistersView") && object && dynamic_cast<RegistersView*>(object) != nullptr)
    {
        ptr = new AccessibleRegistersView(dynamic_cast<QWidget*>(object));
    }
    return ptr;
}

#endif
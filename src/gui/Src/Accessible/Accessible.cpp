#ifndef QT_NO_ACCESSIBILITY
#include "Accessible.h"
#include "AccessibleRegistersView.h"
#include "AccessibleAbstractTableView.h"
#include "AccessibleDisassembly.h"
#include "AccessibleStdTable.h"

QAccessibleInterface* accessibleInterfaceFactory(const QString & classname, QObject* object)
{
    QAccessibleInterface* ptr = nullptr;
    if((classname == "Disassembly") && object && dynamic_cast<Disassembly*>(object) != nullptr)
    {
        ptr = new AccessibleDisassembly(dynamic_cast<QWidget*>(object));
    }
    if((classname == "AbstractStdTable") && object && dynamic_cast<AbstractStdTable*>(object) != nullptr)
    {
        ptr = new AccessibleStdTable(dynamic_cast<QWidget*>(object));
    }
    if((classname == "RegistersView") && object && dynamic_cast<RegistersView*>(object) != nullptr)
    {
        ptr = new AccessibleRegistersView(dynamic_cast<QWidget*>(object));
    }
    return ptr;
}

#endif
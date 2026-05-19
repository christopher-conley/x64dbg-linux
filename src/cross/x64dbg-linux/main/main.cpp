#include "gui/MainWindow.h"
#include "Accessible/AccessibleRegistersView.h"
#include "Gui/RegistersView.h"
#include <QAccessible>

#ifndef QT_NO_ACCESSIBILITY
static QAccessibleInterface* crossAccessibleFactory(const QString & classname, QObject* object)
{
    if(!object)
        return nullptr;
    if(classname == "RegistersView" && dynamic_cast<RegistersView*>(object))
        return new AccessibleRegistersView(dynamic_cast<QWidget*>(object));
    return nullptr;
}
#endif

int main(int argc, char* argv[])
{
    qRegisterMetaType<REGDUMP>("REGDUMP");

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(crossAccessibleFactory);
#endif

    QApplication app(argc, argv);
    MainWindow::loadTheme();

    MainWindow w;
    w.show();
    return QApplication::exec();
}

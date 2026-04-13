#include "gui/MainWindow.h"

int main(int argc, char* argv[])
{
    // TODO: fix and remove this
    qputenv("QT_ACCESSIBILITY", "0");

    qRegisterMetaType<REGDUMP>("REGDUMP");

    QApplication app(argc, argv);
    MainWindow::loadTheme();

    MainWindow w;
    w.show();
    return QApplication::exec();
}

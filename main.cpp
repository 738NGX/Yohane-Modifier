#include "QApplication"
#include "yohanewindow.h"

int main(int argc, char *argv[])
{
    QApplication::addLibraryPath(R"(D:\Qt\6.8.0\mingw_64\plugins)");
    QApplication a(argc, argv);

    YohaneWindow w;
    w.setWindowTitle("幻日夜羽 -湛海耀光- 修改器");
    const QIcon icon("./assets/icon.ico");
    w.setWindowIcon(icon);
    w.show();

    return QApplication::exec();
}

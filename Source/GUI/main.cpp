#include <QApplication>
#include <QtPlugin>
#include <iostream>


#include <ZenLib/Ztring.h>
#include <ZenLib/ZtringListList.h>
using namespace ZenLib;
#define wstring2QString(_DATA) \
    QString::fromUtf8(Ztring(_DATA).To_UTF8().c_str())
#define QString2wstring(_DATA) \
    Ztring().From_UTF8(_DATA.toUtf8())

#include <QtCore/QtPlugin>
#if defined(WIN32) && QT_VERSION >= 0x00050000 //Qt5
    Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

#ifdef __MACOSX__
    #include <ApplicationServices/ApplicationServices.h>
#endif //__MACOSX__
 
using namespace std;

//#include "graphwidget.h"
#include "mainwindow.h"
//#include "scrollarea.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //GraphWidget w(NULL);
    MainWindow w(NULL);
    //ScrollArea w(NULL);
    w.show();
    return a.exec();
}

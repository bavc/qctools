/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <QApplication>
#include <QtPlugin>
#include <iostream>


#include <QtCore/QtPlugin>
#if defined(_WIN32) && QT_VERSION >= 0x00050000 //Qt5
    Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

#ifdef __MACOSX__
    #include <ApplicationServices/ApplicationServices.h>
#endif //__MACOSX__

using namespace std;

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(NULL);
    w.show();
    return a.exec();
}

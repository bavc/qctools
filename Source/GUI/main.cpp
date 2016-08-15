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
#ifdef __MACOSX__
    #include <ApplicationServices/ApplicationServices.h>
#endif //__MACOSX__

using namespace std;

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(NULL);
    for (int Pos=1; Pos<argc; Pos++)
        w.addFile(QString::fromLocal8Bit(argv[Pos]));
    w.addFile_finish();
    w.show();
    return a.exec();
}

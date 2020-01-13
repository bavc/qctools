/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <QApplication>
#include <QScreen>
#include <QtPlugin>
#include <iostream>


#include <QtCore/QtPlugin>
#ifdef __MACOSX__
    #include <ApplicationServices/ApplicationServices.h>
#endif //__MACOSX__

using namespace std;

#include "mainwindow.h"
#include "config.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(NULL);

    auto screenGeometry = w.screen()->geometry();
    w.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, screenGeometry.size() * 0.95, screenGeometry));

    for (int Pos=1; Pos<argc; Pos++)
    {
        if(strcmp(argv[Pos], "--debug") == 0)
        {
            Config::instance().setDebug(true);
        }
        else
        {
            w.addFile(QString::fromLocal8Bit(argv[Pos]));
        }
    }
    w.addFile_finish();
    w.show();
    return a.exec();
}

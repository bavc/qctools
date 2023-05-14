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

#include <Core/logging.h>
#include <Core/Preferences.h>
#ifdef __MACOSX__
    #include <ApplicationServices/ApplicationServices.h>
#endif //__MACOSX__

#include "mainwindow.h"
#include "config.h"

int main(int argc, char *argv[])
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");

    QApplication a(argc, argv);
    Logging logging;

    QStringList files;
    for (int Pos=1; Pos<argc; Pos++)
    {
        if(strcmp(argv[Pos], "--debug") == 0)
        {
            Config::instance().setDebug(true);
        }
        else if(strcmp(argv[Pos], "--log") == 0)
        {
            logging.enable();
        }
        else if(strcmp(argv[Pos], "--resetsettings") == 0)
        {
            Preferences().resetSettings();
        }
        else
        {
            files.append(QString::fromLocal8Bit(argv[Pos]));
        }
    }

    MainWindow w(NULL);

    auto screen = QApplication::primaryScreen();
    auto availableGeometry = screen->availableGeometry();
    auto newSize = availableGeometry.size() * 0.95;
    auto newGeometry = QStyle::alignedRect(Qt::LayoutDirectionAuto, Qt::AlignCenter, newSize, availableGeometry);

    qDebug() << "new size: " << newSize << "availableGeometry: " << availableGeometry << "new geometry: " << newGeometry;
    w.setGeometry(newGeometry);
    for(auto file : files)
    {
        w.addFile(file);
    }
    if(files.size() > 0)
        w.addFile_finish();

    qDebug() << "size: " << w.size() << "pos: " << w.pos();

    w.show();
    qDebug() << "size: " << w.size() << "pos: " << w.pos();

    return a.exec();
}

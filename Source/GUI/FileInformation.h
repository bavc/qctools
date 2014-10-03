/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_FileInformation_H
#define GUI_FileInformation_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/Core.h"
#include "Core/FFmpeg_Glue.h"

#include <QThread>
#include <QPixmap>

class MainWindow;
//---------------------------------------------------------------------------


class FileInformation : public QThread
{
    //thread part
    Q_OBJECT
    void run();

public:
    // Constructor/Destructor
                                FileInformation             (MainWindow* Main, const QString &FileName);
                                ~FileInformation            ();

    // Parsing
    void                        Parse                       ();

    // Dumps
    void                        Export_XmlGz                (const QString &ExportFileName);
    void                        Export_CSV                  (const QString &ExportFileName);
    
    // Infos
    QPixmap*                    Picture_Get                 (size_t Pos);
    QString                     FileName;
    int                         Frames_Pos_Get              ()                                      {return Frames_Pos;}
    void                        Frames_Pos_Set              (int Frames_Pos);
    void                        Frames_Pos_Minus            ();
    void                        Frames_Pos_Plus             ();
    bool                        PlayBackFilters_Available   ();

    // FFmpeg glue
    FFmpeg_Glue*                Glue;

private:
    // Info
    QPixmap                     Pixmap;
    int                         Frames_Pos;
    MainWindow*                 Main;

    // FFmpeg part
    bool                        WantToStop;
};

#endif // GUI_FileInformation_H

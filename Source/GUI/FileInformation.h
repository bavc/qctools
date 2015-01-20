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
class CommonStats;

//---------------------------------------------------------------------------
class FileInformation : public QThread
{
    //thread part
    Q_OBJECT
    void run();

public:
    // Constructor/Destructor
                                FileInformation             (MainWindow* Main, const QString &FileName, int TC_in=(int)-1, int TC_out=(int)-1, const string &Encoding_FileName=string());
                                ~FileInformation            ();

    // Parsing
    void                        Parse                       ();

    // Dumps
    void                        Export_XmlGz                (const QString &ExportFileName);
    void                        Export_CSV                  (const QString &ExportFileName);

    // Infos
    QPixmap*                    Picture_Get                 (size_t Pos);
    QString                     FileName;
    size_t                      ReferenceStream_Pos_Get     () {return ReferenceStream_Pos;}
    int                         Frames_Count_Get            (size_t Stats_Pos=(size_t)-1);
    int                         Frames_Pos_Get              (size_t Stats_Pos=(size_t)-1);
    void                        Frames_Pos_Set              (int Frames_Pos, size_t Stats_Pos=(size_t)-1);
    void                        Frames_Pos_Minus            ();
    void                        Frames_Pos_Plus             ();
    bool                        PlayBackFilters_Available   ();

    // Deck control information
    int                         TC_in;
    int                         TC_out;

    // FFmpeg glue
    FFmpeg_Glue*                Glue;
    std::vector<CommonStats*>   Stats;
    CommonStats*                ReferenceStat               () {return Stats[ReferenceStream_Pos];}

private:
    // Info
    QPixmap                     Pixmap;
    size_t                      ReferenceStream_Pos;
    int                         Frames_Pos;
    MainWindow*                 Main;

    // FFmpeg part
    bool                        WantToStop;
};

#endif // GUI_FileInformation_H

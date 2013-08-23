#ifndef PERFILE_H
#define PERFILE_H

#include "Core/Core.h"

#include "Core/ffmpeg_BasicInfo.h"
#include "Core/ffmpeg_Thumbnails.h"
#include "Core/ffmpeg_Stats.h"

#include <QDialog>
#include <QProcess>
#include <QTemporaryDir>
#include <QMainWindow>


class QLabel;
class QPixmap;
class MainWindow;


class PerFile
{
public:
    // Constructore/Destructor
                                PerFile                     ();
                                ~PerFile                    ();

    // Functions
    void                        Launch                      (MainWindow* SourceClass, const QString &FileName);

    // Dumps
    void                        Export_CSV                  (const QString &ExportFileName);

    // Callbacks
    void                        BasicInfo_Finished          ();
    void                        Thumbnails_Updated          ();
    void                        Thumbnails_Finished         ();
    void                        Stats_Updated               ();
    void                        Stats_Finished              ();
    
private:
    // Infos
    QString                     FileName;

    //For callback
    MainWindow*                 SourceClass;

public: //TEMP
    // Process handlers
    ffmpeg_BasicInfo*           BasicInfo;
    ffmpeg_Thumbnails*          Thumbnails;
    ffmpeg_Stats*               Stats;
};

#endif // PERFILE_H

#ifndef FFMPEG_THUMBNAILS_H
#define FFMPEG_THUMBNAILS_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>
#include <QTemporaryDir>
#include <QByteArray>

#include <vector>
using namespace std;

#include <Core/Core.h>

class PerFile;

class ffmpeg_Thumbnails : public QObject
{
    Q_OBJECT
    
public:
    // Constructore/Destructor
                                ffmpeg_Thumbnails   ();
                                ~ffmpeg_Thumbnails  ();

    // Functions
    void                        Launch                      (PerFile* SourceClass, const QString &FileName, size_t Frames_Total);
    void                        Stop                        ();

    // Infos
    QPixmap*                    Picture_Get                 (size_t Pos, size_t Pixmap_Pos);
    size_t                      Frames_Total;
    size_t                      Frames_Current;
    double                      Frames_Progress_Get();
    void                        Update();

private:
    // Info
    QString                     FileName;
    QByteArray**                Pictures;
    QPixmap                     Pixmaps[9];
    
    // Temp
    QTemporaryDir               TempDir;

    //For callback
    PerFile*                    SourceClass;

    // Process Temp
    QProcess*                   Process;
    QByteArray                  Data;


private Q_SLOTS:
    void                        ProcessMessage              ();
    void                        ProcessError                ();
    void                        ProcessFinished             (int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // FFMPEG_THUMBNAILS_H

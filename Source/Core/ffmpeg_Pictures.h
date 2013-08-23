#ifndef FFMPEG_PICTURES_H
#define FFMPEG_PICTURES_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>
#include <QTemporaryDir>
#include <QByteArray>

#include <vector>
using namespace std;

#include <Core/Core.h>

class PerPicture;

class ffmpeg_Pictures : public QObject
{
    Q_OBJECT
    
public:
    // Constructore/Destructor
                                ffmpeg_Pictures   ();
                                ~ffmpeg_Pictures  ();

    // Functions
    void                        Launch                      (PerPicture* SourceClass, const QString &FileName, size_t Frames_Total, double Duration, size_t Mode);

    // Infos
    QPixmap*                    Picture_Get                 (size_t Pos);
    size_t                      Frames_Total;
    double                      Duration;
    size_t                      Frames_Current;
    double                      Frames_Progress_Get();
    void                        Update();

private:
    // Info
    size_t                      Frames_Next;
    QString                     FileName;
    QByteArray**                Pictures;
    size_t                      Picture_BaseFrame;
    QPixmap                     Picture_Default;
    size_t                      Picture_Current;
    
    // Temp
    QTemporaryDir               TempDir;

    //For callback
    PerPicture*                 SourceClass;
    size_t                      Mode;

    // Process Temp
    QProcess*                   Process;
    QByteArray                  Data;
    void                        Process_Launch(size_t Pos);


private Q_SLOTS:
    void                        ProcessMessage              ();
    void                        ProcessError                ();
    void                        ProcessFinished             (int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // FFMPEG_PICTURES_H

#ifndef FFMPEG_BASICINFO_H
#define FFMPEG_BASICINFO_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>
#include <QTemporaryDir>

#include <vector>
using namespace std;

#include <Core/Core.h>

class PerFile;

class ffmpeg_BasicInfo : public QObject
{
    Q_OBJECT
    
public:
    // Constructore/Destructor
                                ffmpeg_BasicInfo    ();
                                ~ffmpeg_BasicInfo   ();

    // Functions
    void                        Launch                      (PerFile* SourceClass, const QString &FileName);

    // Infos
    size_t                      Frames_Total_Get()                                                                      {return Frames_Total;}
    double                      Duration_Get()                                                                          {return Duration;}

private:
    // Infos
    QString                     FileName;
    size_t                      Frames_Total;
    double                      Duration;

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

#endif // FFMPEG_BASICINFO_H

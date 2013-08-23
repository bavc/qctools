#ifndef FFMPEG_STATS_H
#define FFMPEG_STATS_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>
#include <QTemporaryDir>

#include <vector>
using namespace std;

#include <Core/Core.h>

class PerFile;

class ffmpeg_Stats : public QObject
{
    Q_OBJECT
    
public:
    // Constructore/Destructor
                                ffmpeg_Stats                ();
                                ~ffmpeg_Stats               ();

    // Functions
    void                        Launch                      (PerFile* SourceClass, const QString &FileName, size_t Frames_Total);
    void                        Stop                        ();

    // Dumps
    void                        Export_CSV                  (const QString &ExportFileName);
    
    // Infos
    size_t                      Frames_Total;
    size_t                      Frames_Current;
    double                      Frames_Progress_Get();
    double*                     x; //PTS
    double**                    y; //Data
    double                      y_Max[PlotType_Max];

    QByteArray*                 Data_Get()                                                                                          {return &Data;}
    int                         Data_Current_Get()                                                                                  {return Data_Current;}

private:
    // Infos
    QString                     FileName;

    // Process Temp
    QProcess*                   Process;
    QByteArray                  Data;
    int                         Data_Current;
    int                         Data_Plot_Pos;
    int                         Frames_Plot_Pos;

    //For callback
    PerFile*                    SourceClass;

private Q_SLOTS:
    void                        ProcessMessage              ();
    void                        ProcessError                ();
    void                        ProcessFinished             (int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // FFMPEG_STATS_H

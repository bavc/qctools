#include "Core/ffmpeg_Stats.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QSizePolicy>
#include <QScrollArea>
#include <QPrinter>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QColor>
#include <QPixmap>
#include <QLabel>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDialog>
#include <QToolButton>
#include <QProcess>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE
#include "GUI/mainwindow.h"
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

//---------------------------------------------------------------------------
ffmpeg_Stats::ffmpeg_Stats ()
{
    // Infos
    Frames_Total=0;
    Frames_Current=0;
    x=NULL;
    y=NULL;
    memset(y_Max, 0, sizeof(double));

    // Process Temp
    Process=NULL;
    Data_Current=0;
    Data_Plot_Pos=0;
    Frames_Plot_Pos=0;

    // For callback
    SourceClass=NULL;
}

//---------------------------------------------------------------------------
ffmpeg_Stats::~ffmpeg_Stats ()
{
    // Process Temp
    delete Process;
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::Export_CSV (const QString &ExportFileName)
{
    if (ExportFileName.isEmpty())
        return;

    if (Data.isEmpty())
        return;

    QFile F(ExportFileName);
    F.open(QIODevice::WriteOnly|QIODevice::Truncate);
    F.write(Data);
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::Launch (PerFile* SourceClass_, const QString &FileName_, size_t Frames_Total_)
{
    // Init - Infos
    Frames_Total=Frames_Total_;
    Frames_Current=0;
    Data_Current=0;
    FileName=FileName_;

    // Init - Process Temp
    Data_Current=0;
    Data_Plot_Pos=0;
    Frames_Plot_Pos=0;

    // Init - For callback
    SourceClass=SourceClass_;

    // Configure
    if (y)
    {
        delete x;
        for(size_t j=0; j<PlotName_Max; ++j)
            delete[] y[j];
        delete y;
    }

    x = new double[Frames_Total];
    memset(x, 0x00, Frames_Total*sizeof(double));
    y = new double*[PlotName_Max];
    for(size_t j=0; j<PlotName_Max; ++j)
    {
        y[j] = new double[Frames_Total];
        memset(y[j], 0x00, Frames_Total*sizeof(double));
    }
    
    y_Max[PlotType_Y]=255;
    y_Max[PlotType_U]=255;
    y_Max[PlotType_V]=255;
    y_Max[PlotType_YDiff]=0;
    y_Max[PlotType_YDiffX]=0;
    y_Max[PlotType_UDiff]=0;
    y_Max[PlotType_VDiff]=0;
    y_Max[PlotType_Diffs]=0;
    y_Max[PlotType_TOUT]=0;
    y_Max[PlotType_VREP]=0;
    y_Max[PlotType_HEAD]=0;
    y_Max[PlotType_RANG]=0;

    // Running CSV feed
    QFileInfo FileInfo(FileName);
    if (FileInfo.suffix()=="csv")
    {
        QFile F(FileName);
        F.open(QIODevice::ReadOnly);
        Data=F.readAll();
        SourceClass->Stats_Updated();
        SourceClass->Stats_Finished();
        return;
    }

    // Running ffprobe
    delete Process; Process=new QProcess();
    connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(ProcessMessage()));
    connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ProcessError()));
    connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ProcessFinished(int, QProcess::ExitStatus)));
    Data.clear();
    Process->setWorkingDirectory(FileInfo.absolutePath());
    Process->start(QCoreApplication::applicationDirPath()+"/ffprobe", QStringList() << "-f" << "lavfi" << "movie="+FileInfo.fileName()+",values=stat=tout|vrep|rang|head" << "-show_frames" << "-of" << "csv");
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::Stop ()
{
    if (Process)
        Process->kill();    
    delete Process; Process=NULL;
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::ProcessMessage ()
{
    Data+=Process->readAllStandardOutput();

    int Pos;
    for (;;)
    {
        Pos=Data.indexOf('\n', Data_Current);
        if (Pos==-1)
            break;
        Frames_Current++;
        Data_Current=Pos+1;
    }

    Ztring B; B.From_UTF8(Data.mid(Data_Plot_Pos, Data_Current-Data_Plot_Pos).data());
    ZtringListList List;
    List.Separator_Set(1, __T(","));
    List.Write(B);
    for (size_t Pos=0; Pos<List.size(); Pos++)
        if (List[Pos].size()!=PlotName_Begin+PlotName_Max)
            return;


    //i=line number
    //j=column
    for(unsigned i=0; i<Frames_Current-Frames_Plot_Pos; ++i)
    {
        x[Frames_Plot_Pos+i]=List[i][6].To_float64();
        for(unsigned j=0; j<PlotName_Max; ++j)
        {
            y[j][Frames_Plot_Pos+i]=List[i][PlotName_Begin+j].To_float64();
            switch (j)
            {
                /*
                case PlotName_YDIF :    if (y_Max[PlotType_YDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_UDIF :    if (y_Max[PlotType_UDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_UDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_VDIF :    if (y_Max[PlotType_VDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_VDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                */
                case PlotName_YDIF1 :   if (y_Max[PlotType_YDiffX]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiffX]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_YDIF2 :   if (y_Max[PlotType_YDiffX]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiffX]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_TOUT :    if (y_Max[PlotType_TOUT]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_TOUT]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_VREP :    if (y_Max[PlotType_VREP]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_VREP]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_RANG :    if (y_Max[PlotType_RANG]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_RANG]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_HEAD :    if (y_Max[PlotType_HEAD]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_HEAD]=y[j][Frames_Plot_Pos+i];
                                        break;
                default:                ;
            }
        }
    }

    Frames_Plot_Pos=Frames_Current;
    Data_Plot_Pos=Data_Current;

    if ((Frames_Current%10)==0)
        SourceClass->Stats_Updated();
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::ProcessError()
{
    //QByteArray Data = Process->readAllStandardError();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();
}

//---------------------------------------------------------------------------
void ffmpeg_Stats::ProcessFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    ProcessMessage();
    QByteArray ErrorPart = Process->readAllStandardError();
    Ztring ErrorPartZ; ErrorPartZ.From_UTF8(ErrorPart.data());

    SourceClass->Stats_Finished();
}

//---------------------------------------------------------------------------
double ffmpeg_Stats::Frames_Progress_Get ()
{
    return ((double)Frames_Current)/Frames_Total;
}

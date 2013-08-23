#include "Core/ffmpeg_Pictures.h"

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
#include "GUI/PerPicture.h"
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

const size_t numFiles=50;

//---------------------------------------------------------------------------
ffmpeg_Pictures::ffmpeg_Pictures ()
{
    // Infos
    Frames_Total=0;
    Frames_Current=0;
    Frames_Next=(size_t)-1;
    Pictures=NULL;
    Picture_BaseFrame=(size_t)-1;
    Picture_Current=(size_t)-1;
    
    // Process Temp
    Process=NULL;
}

//---------------------------------------------------------------------------
ffmpeg_Pictures::~ffmpeg_Pictures ()
{
    // Process Temp
    delete Pictures;
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::Launch (PerPicture* SourceClass, const QString &FileName, size_t Frames_Total, double Duration, size_t Mode)
{
    // Saving information
    ffmpeg_Pictures::Frames_Total=Frames_Total;
    ffmpeg_Pictures::Duration=Duration;
    ffmpeg_Pictures::FileName=FileName;
    ffmpeg_Pictures::SourceClass=SourceClass;
    ffmpeg_Pictures::Mode=Mode;
    
    //Thumbails
    if (Pictures)
        for (size_t Pos=0; Pos<ffmpeg_Pictures::Frames_Total; Pos++)
            delete Pictures[Pos];
    Pictures=new QByteArray*[1+numFiles*2];
    memset(Pictures, 0, sizeof(QPixmap*)*(1+numFiles*2));

    Picture_Default.load(":/icon/logo.jpg");
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::ProcessMessage ()
{
    //Data+=Process->readAllStandardOutput();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();

    Update();
    SourceClass->Load_Update();
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::ProcessError ()
{
    //QByteArray Data = Process->readAllStandardError();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();

    Update();
    SourceClass->Load_Update();
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::ProcessFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    //ProcessMessage();
    //QByteArray ErrorPart = Process->readAllStandardError();
    //Ztring ErrorPartZ; ErrorPartZ.From_UTF8(ErrorPart.data());
    

    Update();
    if (Frames_Current!=Picture_BaseFrame+1+numFiles*2)
        int A=0;    
    Frames_Current=Picture_BaseFrame+1+numFiles*2;
    SourceClass->Load_Finished();

    delete Process; Process=NULL;

    if (Frames_Next!=(size_t)-1)
    {
        Process_Launch(Frames_Next);
        Frames_Next=(size_t)-1;
    }
}

//---------------------------------------------------------------------------
QPixmap* ffmpeg_Pictures::Picture_Get (size_t Pos)
{
    Update();

    if (Picture_BaseFrame==(size_t)-1 || Pos<Picture_BaseFrame || Pos>=Picture_BaseFrame+1+numFiles*2)
    {
        if (Process)
            Frames_Next=Pos;
        else
            Process_Launch(Pos);
    }

    if (Frames_Next==(size_t)-1 && Pos>=Picture_BaseFrame && Pos<Picture_BaseFrame+1+numFiles*2 && Pictures && Pictures[Pos-Picture_BaseFrame])
    {
        Picture_Default.loadFromData(*Pictures[Pos-Picture_BaseFrame]);
        Picture_Current=Pos;
    }
    return &Picture_Default;    
}

//---------------------------------------------------------------------------
double ffmpeg_Pictures::Frames_Progress_Get ()
{
    Update();

    return ((double)(Frames_Current-Picture_BaseFrame))/(1+numFiles*2);
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::Update ()
{
    for (; Frames_Current<Picture_BaseFrame+1+numFiles*2; Frames_Current++)
    {
        QFile F(TempDir.path()+"/image"+QString::number(Frames_Current-Picture_BaseFrame+1)+".jpg");
        if (!F.exists())
            break;

        F.open(QIODevice::ReadOnly);
        Pictures[Frames_Current-Picture_BaseFrame]=new QByteArray(F.readAll());
        F.remove();
    }
}

//---------------------------------------------------------------------------
void ffmpeg_Pictures::Process_Launch (size_t Pos)
{
    //Removing old data
    for (size_t Pos2=0; Pos2<1+numFiles*2; Pos2++)
        delete Pictures[Pos2];
    memset(Pictures, 0, sizeof(QPixmap*)*(1+numFiles*2));

    // Computing
    Picture_BaseFrame=Pos;
    if (Picture_BaseFrame<numFiles)
        Picture_BaseFrame=0;
    else
        Picture_BaseFrame-=numFiles;
    Frames_Current=Picture_BaseFrame;

    // Running ffprobe
    delete Process; Process=new QProcess();
    connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(ProcessMessage()));
    connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ProcessError()));
    connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ProcessFinished(int, QProcess::ExitStatus)));
    QFileInfo FileInfo(FileName);
    Data.clear();
    Process->setWorkingDirectory(FileInfo.absolutePath());
    if (Mode==0)
        Process->start(QCoreApplication::applicationDirPath()+"/ffmpeg", QStringList() << "-ss" << QString::number(((float)Picture_BaseFrame)*Duration/Frames_Total) << "-i" << FileInfo.fileName() << "-s" << "702x525" << "-f" << "image2" << "-qscale" << "1" << "-vframes" << QString::number(1+numFiles*2) << TempDir.path()+"/image%d.jpg");
    else
        Process->start(QCoreApplication::applicationDirPath()+"/ffmpeg", QStringList() << "-ss" << QString::number(((float)Picture_BaseFrame)*Duration/Frames_Total) << "-i" << FileInfo.fileName() << "-s" << "702x525" << "-f" << "image2" << "-qscale" << "1" << "-vf" << "split=4[a][b][c][d];[a]pad=iw*4:ih[w];[b]lutyuv=u=128:v=128[x];[c]lutyuv=y=128:v=128,curves=strong_contrast[y];[d]lutyuv=y=128:u=128,curves=strong_contrast[z];[w][x]overlay=w:0[wx];[wx][y]overlay=w*2:0[wxy];[wxy][z]overlay=w*3:0" << "-vframes" << QString::number(1+numFiles*2) << TempDir.path()+"/image%d.jpg");
}
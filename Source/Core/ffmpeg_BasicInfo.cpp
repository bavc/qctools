#include "Core/ffmpeg_BasicInfo.h"

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
ffmpeg_BasicInfo::ffmpeg_BasicInfo ()
{
    // Process Temp
    Process=NULL;
}

//---------------------------------------------------------------------------
ffmpeg_BasicInfo::~ffmpeg_BasicInfo ()
{
    // Process Temp
    delete Process;
}

//---------------------------------------------------------------------------
void ffmpeg_BasicInfo::Launch (PerFile* SourceClass, const QString &FileName)
{
    // Saving information
    ffmpeg_BasicInfo::FileName=FileName;
    ffmpeg_BasicInfo::SourceClass=SourceClass;
    
    // Running ffprobe
    delete Process; Process=new QProcess();
    connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(ProcessMessage()));
    connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ProcessError()));
    connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ProcessFinished(int, QProcess::ExitStatus)));
    QFileInfo FileInfo(FileName);
    Data.clear();
    Process->setWorkingDirectory(FileInfo.absolutePath());
    Process->start(QCoreApplication::applicationDirPath()+"/ffprobe", QStringList() << "-show_format" << "-show_streams" << FileInfo.fileName());
}

//---------------------------------------------------------------------------
void ffmpeg_BasicInfo::ProcessMessage ()
{
    Data+=Process->readAllStandardOutput();
}

//---------------------------------------------------------------------------
void ffmpeg_BasicInfo::ProcessError ()
{
    //QByteArray Data = Process->readAllStandardError();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();
}

//---------------------------------------------------------------------------
void ffmpeg_BasicInfo::ProcessFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    ProcessMessage();
    QByteArray ErrorPart = Process->readAllStandardError();
    Ztring ErrorPartZ; ErrorPartZ.From_UTF8(ErrorPart.data());

    Ztring Data2 = Ztring().From_UTF8(Data.data());
    Ztring Frames=Data2.SubString(__T("nb_frames="), __T("\n"));
    Frames_Total=Frames.To_int32u();
    Ztring DurationZ=Data2.SubString(__T("duration="), __T("\n"));
    Duration=DurationZ.To_float32();
    Data.clear();

    SourceClass->BasicInfo_Finished();
}

#include "Core/ffmpeg_Thumbnails.h"

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
ffmpeg_Thumbnails::ffmpeg_Thumbnails ()
{
    // Infos
    Frames_Total=0;
    Frames_Current=0;
    Pictures=NULL;
    
    // Process Temp
    Process=NULL;
}

//---------------------------------------------------------------------------
ffmpeg_Thumbnails::~ffmpeg_Thumbnails ()
{
    // Process Temp
    delete Process;
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::Launch (PerFile* SourceClass, const QString &FileName, size_t Frames_Total)
{
    if (Pictures)
    {
        for (size_t Pos=0; Pos<ffmpeg_Thumbnails::Frames_Total; Pos++)
            delete Pictures[Pos];
        delete Pictures; Pictures=NULL;
    }
    
    // Saving information
    ffmpeg_Thumbnails::Frames_Total=Frames_Total;
    ffmpeg_Thumbnails::FileName=FileName;
    ffmpeg_Thumbnails::SourceClass=SourceClass;
    
    //Thumbails
    Pictures=new QByteArray*[Frames_Total];
    memset(Pictures, 0, sizeof(QPixmap*)*Frames_Total);
    Frames_Current=0;

    // Running ffprobe
    delete Process; Process=new QProcess();
    connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(ProcessMessage()));
    connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ProcessError()));
    connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ProcessFinished(int, QProcess::ExitStatus)));
    QFileInfo FileInfo(FileName);
    Data.clear();
    Process->setWorkingDirectory(FileInfo.absolutePath());
    Process->start(QCoreApplication::applicationDirPath()+"/ffmpeg", QStringList() << "-i" << FileInfo.fileName() << "-s" << "72x72" << "-f" << "image2" << TempDir.path()+"/thumbnail%d.jpg");
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::Stop ()
{
    delete Process; Process=NULL;
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::ProcessMessage ()
{
    //Data+=Process->readAllStandardOutput();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();

    Update();

    SourceClass->Thumbnails_Updated();
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::ProcessError ()
{
    //QByteArray Data = Process->readAllStandardError();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();

    Update();

    SourceClass->Thumbnails_Updated();
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::ProcessFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    //ProcessMessage();
    //QByteArray ErrorPart = Process->readAllStandardError();
    //Ztring ErrorPartZ; ErrorPartZ.From_UTF8(ErrorPart.data());

    Update();

    SourceClass->Thumbnails_Finished();
}

//---------------------------------------------------------------------------
QPixmap* ffmpeg_Thumbnails::Picture_Get (size_t Pos, size_t Pixmap_Pos)
{
    Update();

    if (Pos>Frames_Total || Pictures==NULL || Pictures[Pos]==NULL)
    {
        Pixmaps[Pixmap_Pos].load(":/icon/logo.jpg");
        Pixmaps[Pixmap_Pos]=Pixmaps[Pixmap_Pos].scaled(72, 72);
    }
    else
        Pixmaps[Pixmap_Pos].loadFromData(*Pictures[Pos]);
    return &Pixmaps[Pixmap_Pos];    
}

//---------------------------------------------------------------------------
double ffmpeg_Thumbnails::Frames_Progress_Get ()
{
    Update();

    return ((double)Frames_Current)/Frames_Total;
}

//---------------------------------------------------------------------------
void ffmpeg_Thumbnails::Update ()
{
    Ztring A; A.From_UTF8(TempDir.path().toUtf8());
    
    for (; Frames_Current<Frames_Total; Frames_Current++)
    {
        QFile F(TempDir.path()+"/thumbnail"+QString::number(Frames_Current+1)+".jpg");
        if (!F.exists())
            break;

        F.open(QIODevice::ReadOnly);
        Pictures[Frames_Current]=new QByteArray(F.readAll());
        F.remove();
    }
}

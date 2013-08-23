#include "PerPicture.h"
#include "ui_PerPicture.h"

#include "GUI/PerFile.h"
#include "Core/ffmpeg_Pictures.h"

#include <QProcess>
#include <QFileInfo>

#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

PerPicture::PerPicture(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PerPicture)
{
    ui->setupUi(this);

    for (size_t Pos=0; Pos<PlotName_Max; Pos++)
    {
        Values[Pos]=new QLabel(this);
        Values[Pos]->setText(Names[Pos]+QString("= XXX"));
        ui->verticalLayout->addWidget(Values[Pos]);
    }

    Process=NULL;
    PicturePos_Current=(size_t)-1;
    PicturePos_Computing_Base=(size_t)-1;
    for (size_t Pos=0; Pos<1+100*2; Pos++)
    {
        PicturePos_Pixmaps[Pos]=NULL;
    }


    ui->Picture->setPixmap(QPixmap(":/icon/logo.jpg"));
    //ui->Keep->setVisible(false);
    ui->Keep->setText("Switch to chroma");
    //connect(ui->Keep, SIGNAL(clicked(bool)), this, SLOT(on_Keep_clicked(bool)));

    Pictures[0]=NULL;
    Pictures[1]=NULL;
    Pictures_Current=0;
}

PerPicture::~PerPicture()
{
    delete ui;

    delete Pictures[0];
    delete Pictures[1];
}

void PerPicture::ShowPicture (size_t PicturePos, double** y, QString FileName, PerFile* Source)
{
    if (!isVisible())
        return;

    // Stats
    PerPicture::PicturePos=PicturePos;
    FileName_Current=FileName;
    if (y)
        for (size_t Pos=0; Pos<PlotName_Max; Pos++)
        {
            if (PicturePos<Source->Stats->Frames_Current)
                Values[Pos]->setText(Names[Pos]+QString("= ")+QString::number((int)y[Pos][PicturePos]));
            else
                Values[Pos]->setText(Names[Pos]+QString("= XXX"));
        }

    // Picture
    if (Pictures[Pictures_Current]==NULL)
    {
        Pictures[0]=new ffmpeg_Pictures();
        Pictures[0]->Launch(this, FileName, Source->BasicInfo->Frames_Total_Get(), Source->BasicInfo->Duration_Get(), 0);
        Pictures[1]=new ffmpeg_Pictures();
        Pictures[1]->Launch(this, FileName, Source->BasicInfo->Frames_Total_Get(), Source->BasicInfo->Duration_Get(), 1);
    }
    ui->Picture->setPixmap(*Pictures[Pictures_Current]->Picture_Get(PicturePos));

    // Displaying the frame
    double Progress=Pictures[Pictures_Current]->Frames_Progress_Get();
    if (Progress==1)
    {
        setWindowTitle("QC Tools - Frame "+QString::number(PicturePos)+" - "+FileName_Current);
        return;
    }

    // Computing the frame
    Load_Update();
}

//---------------------------------------------------------------------------
void PerPicture::ProcessMessage()
{
}

//---------------------------------------------------------------------------
void PerPicture::ProcessError()
{
}

//---------------------------------------------------------------------------
void PerPicture::ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
}

//---------------------------------------------------------------------------
void PerPicture::Load_Update()
{
    setWindowTitle("QC Tools - Frame "+QString::number(PicturePos)+" (Computing "+QString::number(Pictures[Pictures_Current]->Frames_Progress_Get()*100, 'f', 0)+"%) - "+FileName_Current);
}

//---------------------------------------------------------------------------
void PerPicture::Load_Finished()
{
    ui->Picture->setPixmap(*Pictures[Pictures_Current]->Picture_Get(PicturePos));
    setWindowTitle("QC Tools - Frame "+QString::number(PicturePos)+" - "+FileName_Current);
}

void PerPicture::on_Keep_clicked(bool checked)
{
    if (Pictures_Current)
    {
        Pictures_Current=0;
        ui->Keep->setText("Switch to chroma");
    }
    else
    {
        Pictures_Current=1;
        ui->Keep->setText("Switch to normal");
    }
    ShowPicture (PicturePos, NULL, FileName_Current, NULL);
}
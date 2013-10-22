#include "PerPicture.h"
#include "ui_PerPicture.h"

#include "GUI/PerFile.h"
#include "Core/ffmpeg_Pictures.h"

#include <QProcess>
#include <QFileInfo>

#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;


const size_t FiltersListDefault_Count = 10;

const char* FiltersListDefault_Names [FiltersListDefault_Count] =
{
    "Normal",
    "Field split",
    "Field diff",
    "Field waveform",
    "Range waveform",
    "Waveform parade",
    "Field histogram",
    "Field vectroscope",
    "Chroma split",
    "Interlace",
};

const char* FiltersListDefault_Value [FiltersListDefault_Count] =
{
    "",
    "split[a][b]; [a]pad=iw:ih*2,field=top[src]; [b]field=bottom[filt]; [src][filt]overlay=0:h",
    "split=4[a][b][c][d]; [a]pad=iw:ih*3,field=top[src]; [b]field=bottom[filt];[c]field=bottom[bb];[d]field=top,negate[tb];[src][filt]overlay=0:h[upper];[bb][tb]blend=all_mode=average[blend];[upper][blend]overlay=0:h*2",
    "split[a][b];[a]field=top,split[topa][topb];[b]field=bottom,split[bota][botb];[topb]histogram=mode=waveform:waveform_mode=column:waveform_mirror=1[topbh];[botb]histogram=mode=waveform:waveform_mode=column:waveform_mirror=1[botbh];[topa]pad=iw*2:ih+(256*3)[topapad];[topapad][bota]overlay=w[rowa];[rowa][topbh]overlay=0:H-(256*3)[rowab1];[rowab1][botbh]overlay=w:H-(256*3)",
    "split[a][b];[a]format=gray,histogram=mode=waveform:waveform_mode=column:waveform_mirror=1,split[c][d];[b]pad=iw:ih+256[padded];[c]geq=g=1:b=1[red];[d]geq=r=1:b=1,crop=in_w:220:0:16[mid];[red][mid]overlay=0:16[wave];[padded][wave]overlay=0:H-h",
    "split[a][b];[a]histogram=mode=waveform:waveform_mode=column:waveform_mirror=1:display_mode=overlay[c];[b]pad=iw:ih+256[padded];[padded][c]overlay=0:H-h",
    "split[a][b];[a]field=top,split[topa][topb];[b]field=bottom,split[bota][botb];[topb]histogram[topbh];[botb]histogram[botbh];[topa]pad=iw*2:ih+636[topapad];[topapad][bota]overlay=w[rowa];[rowa][topbh]overlay=W/2-256:H-636[rowab1];[rowab1][botbh]overlay=W/2:H-636",
    "split[a][b];[a]field=top,split[topa][topb];[b]field=bottom,split[bota][botb];[topb]histogram=mode=color[topbh];[botb]histogram=mode=color[botbh];[topa]pad=iw*2:ih+256[topapad];[topapad][bota]overlay=w[rowa];[rowa][topbh]overlay=W/2-256:H-256[rowab1];[rowab1][botbh]overlay=W/2:H-256",
    "split=4[a][b][c][d];[a]pad=iw*2:ih*2[w];[b]lutyuv=u=128:v=128[x];[c]lutyuv=y=128:v=128,curves=strong_contrast[y];[d]lutyuv=y=128:u=128,curves=strong_contrast[z];[w][x]overlay=w:0[wx];[wx][y]overlay=0:h[wxy];[wxy][z]overlay=w:h",
    "kerndeint=map=1",
};


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
    Pictures=new ffmpeg_Pictures*[FiltersListDefault_Count];
    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
    {
        ui->FiltersList->addItem(FiltersListDefault_Names[Pos]);
        Pictures[Pos]=NULL;
    }
    connect(ui->FiltersList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_FiltersList_currentIndexChanged(int)));

    Pictures_Current=0;
}

PerPicture::~PerPicture()
{
    delete ui;

    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
        delete Pictures[Pos];
    delete[] Pictures;
}

void PerPicture::ShowPicture (size_t PicturePos, double** y, QString FileName, PerFile* Source)
{
    if (!isVisible())
        return;

    // Stats
    PerPicture::PicturePos=PicturePos;
    if (Source)
        PerPicture::Source=Source;
    FileName_Current=FileName;
    if (y)
    {
        for (size_t Pos=0; Pos<PlotName_Max; Pos++)
        {
            if (PicturePos<Source->Stats->Frames_Current)
                Values[Pos]->setText(Names[Pos]+QString("= ")+QString::number((int)y[Pos][PicturePos]));
            else
                Values[Pos]->setText(Names[Pos]+QString("= XXX"));
        }
    }

    // Picture
    if (Pictures[Pictures_Current]==NULL)
    {
        Pictures[Pictures_Current]=new ffmpeg_Pictures();
        Pictures[Pictures_Current]->Launch(this, FileName, Source->BasicInfo->Frames_Total_Get(), Source->BasicInfo->Duration_Get(), FiltersListDefault_Value[Pictures_Current]);
    }
    QPixmap* NewPicture=Pictures[Pictures_Current]->Picture_Get(PicturePos);
    ui->Picture->setPixmap(*NewPicture);

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

void PerPicture::on_FiltersList_currentIndexChanged(int index)
{
    if (Pictures_Current==index)
        return;

    Pictures_Current=index;
    ShowPicture (PicturePos, NULL, FileName_Current, Source);
}
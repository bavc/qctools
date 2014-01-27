/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "BigDisplay.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"
#include "GUI/Help.h"
#include "GUI/FileInformation.h"
#include "Core/FFmpeg_Glue.h"

#include <QDesktopWidget>
#include <QGridLayout>
#include <QPixmap>
#include <QComboBox>
#include <QSlider>
#include <QMenu>
#include <QCheckBox>
#include <QLabel>
#include <QToolButton>
#include <QActionGroup>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QPainter>
#include <QSpacerItem>
#include <QPushButton>

#ifdef _WIN32
    #include <string>
    #include <algorithm>
#endif
//---------------------------------------------------------------------------


//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
const size_t FiltersListDefault_Count = 2+24;

const char* FiltersListDefault_Names [FiltersListDefault_Count] =
{
    "Help about playback filters",
    "No display",
    "Normal",

    "Field split",
    "Field diff",

    "Histogram",
    "Histogram - Field Split",

    "Waveform",
    "Waveform - Field 1",
    "Waveform - Field 2",
    "Waveform - Field Split",

    "Broadcast Range Pixels",
    "Broadcast Range Pixels - Field Split",

    "Vectroscope",
    "Vectroscope - Field 1",
    "Vectroscope - Field 2",
    "Vectroscope - Field Split",

    "Temporal Outlier Pixels",
    "Temporal Outlier Pixels (field-split)",

    "Extract Y",
    "Extract U",
    "Extract V",
    "Extract UV",

    "Extract Y - Equalized",
    "Extract U - Equalized",
    "Extract V - Equalized",
};

const char* FiltersListDefault_Value [FiltersListDefault_Count] =
{
    "",
    "",
    "",

    "split[a][b]; [a]pad=iw:ih*2,field=top[src]; [b]field=bottom[filt]; [src][filt]overlay=0:h",
    "split[a][b];[a]field=bottom[bb];[b]field=top,negate[tb];[bb][tb]blend=all_mode=average",


    "histogram",
    "split[a][b];[a]field=top,histogram,pad=iw*2[a2];[b]field=bottom,histogram[b2];[a2][b2]overlay=w",

    "histogram=step=16:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawgrid=y=(256-16):c=white@0.4,drawgrid=y=(256-235):c=white@0.4,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,scale=720x486",
    "field=top,histogram=step=16:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawgrid=y=(256-16):c=white@0.4,drawgrid=y=(256-235):c=white@0.4,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,scale=720x486",
    "field=bottom,histogram=step=16:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawgrid=y=(256-16):c=white@0.4,drawgrid=y=(256-235):c=white@0.4,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16,scale=720x486",
    "split[a][b];[a]field=top[c];[b]field=bottom[d];[c]histogram=step=16:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawgrid=y=(256-16):c=white@0.4,drawgrid=y=(256-235):c=white@0.4,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[e];[d]histogram=step=16:mode=waveform:waveform_mode=column:waveform_mirror=1,crop=iw:256:0:0,drawgrid=y=(256-16):c=white@0.4,drawgrid=y=(256-235):c=white@0.4,drawbox=y=(256-16):w=iw:h=16:color=aqua@0.2:t=16,drawbox=w=iw:h=(256-235):color=crimson@0.2:t=16[f];[e]pad=0:ih*2[g];[g][f]overlay=0:h",

    "values=out=rang",
    "split[a][b];[a]field=top[c];[b]field=bottom[d];[c]values=out=rang[e];[d]values=out=rang[f];[e]pad=0:ih*2[g];[g][f]overlay=0:h",

    "histogram=mode=color2,transpose=dir=2,drawgrid=w=16:h=16:t=1:c=white@0.2,drawgrid=w=128:h=128:t=1:c=white@0.3,drawgrid=w=4:h=4:t=1:c=white@0.1,drawbox=w=5:h=5:t=1:x=90-2:y=256-240-3:c=red@0.5,drawbox=w=5:h=5:t=1:x=54-2:y=256-34-3:c=green@0.5,drawbox=w=5:h=5:t=1:x=240-2:y=256-110-3:c=blue@0.5,drawbox=w=5:h=5:t=1:x=166-2:y=256-16-3:c=cyan@0.5,drawbox=w=5:h=5:t=1:x=202-2:y=256-222-3:c=magenta@0.5,drawbox=w=5:h=5:t=1:x=16-2:y=256-146-3:c=yellow@0.5,scale=486:486",
    "field=top,histogram=mode=color2,transpose=dir=2,drawgrid=w=16:h=16:t=1:c=white@0.2,drawgrid=w=128:h=128:t=1:c=white@0.3,drawgrid=w=4:h=4:t=1:c=white@0.1,drawbox=w=5:h=5:t=1:x=90-2:y=256-240-3:c=red@0.5,drawbox=w=5:h=5:t=1:x=54-2:y=256-34-3:c=green@0.5,drawbox=w=5:h=5:t=1:x=240-2:y=256-110-3:c=blue@0.5,drawbox=w=5:h=5:t=1:x=166-2:y=256-16-3:c=cyan@0.5,drawbox=w=5:h=5:t=1:x=202-2:y=256-222-3:c=magenta@0.5,drawbox=w=5:h=5:t=1:x=16-2:y=256-146-3:c=yellow@0.5,scale=486:486",
    "field=bottom,histogram=mode=color2,transpose=dir=2,drawgrid=w=16:h=16:t=1:c=white@0.2,drawgrid=w=128:h=128:t=1:c=white@0.3,drawgrid=w=4:h=4:t=1:c=white@0.1,drawbox=w=5:h=5:t=1:x=90-2:y=256-240-3:c=red@0.5,drawbox=w=5:h=5:t=1:x=54-2:y=256-34-3:c=green@0.5,drawbox=w=5:h=5:t=1:x=240-2:y=256-110-3:c=blue@0.5,drawbox=w=5:h=5:t=1:x=166-2:y=256-16-3:c=cyan@0.5,drawbox=w=5:h=5:t=1:x=202-2:y=256-222-3:c=magenta@0.5,drawbox=w=5:h=5:t=1:x=16-2:y=256-146-3:c=yellow@0.5,scale=486:486",
    "split[a][b];[a]field=top[c];[b]field=bottom[d];[c]histogram=mode=color2,transpose=dir=2,drawgrid=w=16:h=16:t=1:c=white@0.2,drawgrid=w=128:h=128:t=1:c=white@0.3,drawgrid=w=4:h=4:t=1:c=white@0.1,drawbox=w=5:h=5:t=1:x=90-2:y=256-240-3:c=red@0.5,drawbox=w=5:h=5:t=1:x=54-2:y=256-34-3:c=green@0.5,drawbox=w=5:h=5:t=1:x=240-2:y=256-110-3:c=blue@0.5,drawbox=w=5:h=5:t=1:x=166-2:y=256-16-3:c=cyan@0.5,drawbox=w=5:h=5:t=1:x=202-2:y=256-222-3:c=magenta@0.5,drawbox=w=5:h=5:t=1:x=16-2:y=256-146-3:c=yellow@0.5[e];[d]histogram=mode=color2,transpose=dir=2,drawgrid=w=16:h=16:t=1:c=white@0.2,drawgrid=w=128:h=128:t=1:c=white@0.3,drawgrid=w=4:h=4:t=1:c=white@0.1,drawbox=w=5:h=5:t=1:x=90-2:y=256-240-3:c=red@0.5,drawbox=w=5:h=5:t=1:x=54-2:y=256-34-3:c=green@0.5,drawbox=w=5:h=5:t=1:x=240-2:y=256-110-3:c=blue@0.5,drawbox=w=5:h=5:t=1:x=166-2:y=256-16-3:c=cyan@0.5,drawbox=w=5:h=5:t=1:x=202-2:y=256-222-3:c=magenta@0.5,drawbox=w=5:h=5:t=1:x=16-2:y=256-146-3:c=yellow@0.5[f];[e]pad=iw*2[g];[g][f]overlay=w",

    "values=out=tout",
    "split[a][b];[a]field=top[c];[b]field=bottom[d];[c]values=out=tout[e];[d]values=out=tout[f];[e]pad=0:ih*2[g];[g][f]overlay=0:h",

    "extractplanes=y",
    "extractplanes=u",
    "extractplanes=v",
    "extractplanes=u+v[u][v];[u]pad=iw*2[u2];[u2][v]overlay=w,drawgrid=iw/2:ih:c=blue@0.2:t=1",

    "format=yuvj444p,extractplanes=y,histeq=strength=0.2",
    "format=yuvj444p,extractplanes=u,histeq=strength=0.2",
    "format=yuvj444p,extractplanes=v,histeq=strength=0.2",
};

const bool FiltersListDefault_Separator [FiltersListDefault_Count] =
{
    false,
    false,
    true,

    false,
    true,

    false,
    true,

    false,
    false,
    false,
    true,

    false,
    true,

    false,
    false,
    false,
    true,

    false,
    true,

    false,
    false,
    false,
    true,

    false,
    false,
    false,
};

//***************************************************************************
// Helper
//***************************************************************************

//---------------------------------------------------------------------------
ImageLabel::ImageLabel(FFmpeg_Glue** Picture_, size_t Pos_, QWidget *parent) :
    QWidget(parent),
    Picture(Picture_),
    Pos(Pos_)
{
    Pixmap_MustRedraw=false;
    IsMain=true;
}

//---------------------------------------------------------------------------
void ImageLabel::paintEvent(QPaintEvent *event)
{
    //QWidget::paintEvent(event);

    QPainter painter(this);
    if (!*Picture)
    {
        painter.drawPixmap(0, 0, QPixmap().scaled(event->rect().width(), event->rect().height()));
        return;
    }

    /*
    painter.setRenderHint(QPainter::Antialiasing);

    QSize pixSize = Pixmap.size();
    pixSize.scale(event->rect().size(), Qt::KeepAspectRatio);

    QPixmap scaledPix = Pixmap.scaled(pixSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    */

    QImage* Image;
    switch (Pos)
    {
        case 1 : Image=(*Picture)->Image1; break;
        case 2 : Image=(*Picture)->Image2; break;
        default: return;
    }  
    if (!Image)
    {
        painter.drawPixmap(0, 0, QPixmap().scaled(event->rect().width(), event->rect().height()));
        return;
    }

    QSize Size = event->rect().size();
    if (Pixmap_MustRedraw || Size.width()!=Pixmap.width() || Size.height()!=Pixmap.height())
    {
        if (IsMain && (Size.width()!=Pixmap.width() || Size.height()!=Pixmap.height()))
        {
            (*Picture)->Scale_Change(Size.width(), Size.height());
            switch (Pos)
            {
                case 1 : Image=(*Picture)->Image1; break;
                case 2 : Image=(*Picture)->Image2; break;
                default: return;
            }
        }
        Pixmap.convertFromImage(*Image);
        Pixmap_MustRedraw=false;
    }

    painter.drawPixmap((event->rect().width()-Pixmap.size().width())/2, (event->rect().height()-Pixmap.size().height())/2, Pixmap);
}

//---------------------------------------------------------------------------
void ImageLabel::Remove ()
{
    Pixmap=QPixmap();
    resize(0, 0);
    repaint();
    setVisible(false);
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BigDisplay::BigDisplay(QWidget *parent, FileInformation* FileInformationData_) :
    QDialog(parent),
    FileInfoData(FileInformationData_)
{
    setWindowTitle("QCTools - "+FileInfoData->FileName);
    setWindowFlags(windowFlags()&(0xFFFFFFFF-Qt::WindowContextHelpButtonHint));
    resize(QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5);

    QFont Font=QFont();
    #ifdef _WIN32
        Font.setPointSize(8);
    #else //_WIN32
        Font.setPointSize(8);
    #endif //_WIN32

    Layout=new QGridLayout();
    Layout->setContentsMargins(0, 0, 0, 0);

    //Image1
    Image1=new ImageLabel(&Picture, 1, this);
    Image1->IsMain=true;
    Image1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Layout->addWidget(Image1, 0, 0, 3, 1);
    Layout->setColumnStretch(0, 1);

    //Image2
    Image2=new ImageLabel(&Picture, 2, this);
    Image2->IsMain=false;
    Image2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Layout->addWidget(Image2, 0, 4, 3, 1);
    Layout->setColumnStretch(4, 1);

    // Filters
    FiltersList1=new QToolButton(this);
    FiltersList1->setText("< Filters");
    //FiltersList1->setFont(Font);
    connect(FiltersList1, SIGNAL(pressed()), this, SLOT(on_FiltersList1_click()));
    Layout->addWidget(FiltersList1, 0, 1, 1, 1, Qt::AlignLeft);
    //Layout->addWidget(new QLabel("        "), 0, 2, 1, 1, Qt::AlignHCenter);
    FiltersList2=new QToolButton();
    FiltersList2->setText("Filters >");
    //FiltersList2->setFont(Font);
    connect(FiltersList2, SIGNAL(pressed()), this, SLOT(on_FiltersList2_click()));
    Layout->addWidget(FiltersList2, 0, 3, 1, 1, Qt::AlignRight);

    // Info
    InfoArea=new Info(this, FileInfoData, Info::Style_Columns);
    Layout->addWidget(InfoArea, 1, 1, 1, 3, Qt::AlignHCenter);
    Layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 1, 1, 3, Qt::AlignHCenter);

    // Control
    ControlArea=new Control(this, FileInfoData, Control::Style_Cols, true);
    Layout->addWidget(ControlArea, 3, 0, 1, 5, Qt::AlignBottom);

    // Slider
    Slider=new QSlider(Qt::Horizontal);
    Slider->setMaximum(FileInfoData->Glue->VideoFrameCount);
    connect(Slider, SIGNAL(sliderMoved(int)), this, SLOT(on_Slider_sliderMoved(int)));
    connect(Slider, SIGNAL(actionTriggered(int)), this, SLOT(on_Slider_actionTriggered(int)));
    Layout->addWidget(Slider, 4, 0, 1, 5);

    setLayout(Layout);

    // Picture
    Picture=NULL;
    Picture_Current1=2;
    Picture_Current2=7;

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;
}

//---------------------------------------------------------------------------
BigDisplay::~BigDisplay()
{
    delete Picture;
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::ShowPicture ()
{
    if (!isVisible())
        return;

    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;
    
    // Stats
    if (ControlArea)
        ControlArea->Update();
    if (InfoArea)
        InfoArea->Update();

    // Picture
    if (!Picture)
    {
        string FileName_string=FileInfoData->FileName.toUtf8().data();
        #ifdef _WIN32
            replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
        #endif
        Picture=new FFmpeg_Glue(FileName_string.c_str(), QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5, FFmpeg_Glue::Output_QImage, FiltersListDefault_Value[Picture_Current1], FiltersListDefault_Value[Picture_Current2], Picture_Current1<FiltersListDefault_Count, Picture_Current2<FiltersListDefault_Count);
    }
    Picture->FrameAtPosition(Frames_Pos);
    if (Picture->Image1)
    {
        Image_Width=Picture->Image1->width();
        Image_Height=Picture->Image1->height();
    }

    if (Slider->sliderPosition()!=Frames_Pos)
        Slider->setSliderPosition(Frames_Pos);

    Image1->Pixmap_MustRedraw=true;
    Image1->repaint();
    Image2->Pixmap_MustRedraw=true;
    Image2->repaint();
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::on_Slider_sliderMoved(int value)
{
    if (ControlArea->OM->isEnabled())
        ControlArea->OM->click();
        
    FileInfoData->Frames_Pos_Set(value);
}

//---------------------------------------------------------------------------
void BigDisplay::on_Slider_actionTriggered(int action )
{
    if (action==QAbstractSlider::SliderMove)
        return;

    if (ControlArea->OM->isEnabled())
        ControlArea->OM->click();
        
    FileInfoData->Frames_Pos_Set(Slider->sliderPosition());
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersSource_stateChanged(int state)
{
    ShowPicture ();
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList1_click()
{
    QMenu* Menu=new QMenu(this);
    QActionGroup* ActionGroup=new QActionGroup(this);

    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
    {
        QAction* Action=new QAction(FiltersListDefault_Names[Pos], this);
        Action->setCheckable(true);
        if (Pos==Picture_Current1)
            Action->setChecked(true);
        if (Pos)
            ActionGroup->addAction(Action);
        Menu->addAction(Action);
        if (FiltersListDefault_Separator[Pos])
            Menu->addSeparator();
    }

    connect(Menu, SIGNAL(triggered(QAction*)), this, SLOT(on_FiltersList1_currentIndexChanged(QAction*)));
    Menu->exec(FiltersList1->mapToGlobal(QPoint(-Menu->width(), FiltersList1->height())));
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList2_click()
{
    QMenu* Menu=new QMenu(this);
    QActionGroup* ActionGroup=new QActionGroup(this);

    for (size_t Pos=0; Pos<FiltersListDefault_Count; Pos++)
    {
        QAction* Action=new QAction(FiltersListDefault_Names[Pos], this);
        Action->setCheckable(true);
        if (Pos==Picture_Current2)
            Action->setChecked(true);
        if (Pos)
            ActionGroup->addAction(Action);
        Menu->addAction(Action);
        if (FiltersListDefault_Separator[Pos])
            Menu->addSeparator();
    }

    connect(Menu, SIGNAL(triggered(QAction*)), this, SLOT(on_FiltersList2_currentIndexChanged(QAction*)));
    Menu->exec(FiltersList2->mapToGlobal(QPoint(FiltersList2->width(), FiltersList2->height())));
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList1_currentIndexChanged(QAction * action)
{
    // Help
    if (action->text()=="Help about playback filters")
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (action->text()=="No display")
    {
        Picture->With1_Change(false);
        Image1->Remove();
        Layout->setColumnStretch(0, 0);
        //move(pos().x()+Image_Width, pos().y());
        //adjustSize();
        Picture_Current1=1;
        Image1->IsMain=false;
        Image2->IsMain=true;
        repaint();
        return;
    }

    // Filters
    size_t Pos=0;
    for (; Pos<FiltersListDefault_Count; Pos++)
    {
        if (action->text()==FiltersListDefault_Names[Pos])
        {
            if (Picture_Current1<2)
            {
                Image1->setVisible(true);
                Image1->IsMain=true;
                Image2->IsMain=false;
                Layout->setColumnStretch(0, 1);
               // move(pos().x()-Image_Width, pos().y());
               // resize(width()+Image_Width, height());
            }
            Picture_Current1=Pos;
            Picture->Filter1_Change(FiltersListDefault_Value[Pos]);

            Frames_Pos=(size_t)-1;
            ShowPicture ();
            return;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::on_FiltersList2_currentIndexChanged(QAction * action)
{
    // Help
    if (action->text()=="Help about playback filters")
    {
        Help* Frame=new Help(this);
        Frame->PlaybackFilters();
        return;
    }

    // None
    if (action->text()=="No display")
    {
        Picture->With2_Change(false);
        Image2->Remove();
        Layout->setColumnStretch(4, 0);
        //adjustSize();
        Picture_Current2=1;
        repaint();
        return;
    }

    // Filters
    size_t Pos=0;
    for (; Pos<FiltersListDefault_Count; Pos++)
    {
        if (action->text()==FiltersListDefault_Names[Pos])
        {
            if (Picture_Current2<2)
            {
                Image2->setVisible(true);
                Layout->setColumnStretch(4, 1);
                //resize(width()+Image_Width, height());
            }
            Picture_Current2=Pos;
            Picture->Filter2_Change(FiltersListDefault_Value[Pos]);

            Frames_Pos=(size_t)-1;
            ShowPicture ();
            return;
        }
    }
}

//---------------------------------------------------------------------------
void BigDisplay::resizeEvent(QResizeEvent* Event)
{
    if (Event->oldSize().width()<0 || Event->oldSize().height()<0)
        return;

    /*int DiffX=(Event->size().width()-Event->oldSize().width())/2;
    int DiffY=(Event->size().width()-Event->oldSize().width())/2;
    Picture->Scale_Change(Image_Width+DiffX, Image_Height+DiffY);

    Frames_Pos=(size_t)-1;
    ShowPicture ();*/
    int SizeX=(Event->size().width()-InfoArea->width())/2-25;
    int SizeY=(Event->size().height()-Slider->height())-50;

    /*if (InfoArea->height()+FiltersList1->height()+Slider->height()+ControlArea->height()+50>=Event->size().height())
    {
        InfoArea->hide();
        Layout->removeWidget(InfoArea);
    }
    else
    {
        Layout->addWidget(InfoArea, 0, 1, 1, 3, Qt::AlignLeft);
        InfoArea->show();
    }*/

    //adjust();
    //Picture->Scale_Change(Image1->width(), Image1->height());

    //Frames_Pos=(size_t)-1;
    //ShowPicture ();
    Image1->Pixmap_MustRedraw=true;
    Image2->Pixmap_MustRedraw=true;
}

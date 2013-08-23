#include "mainwindow.h"
#include "ui_mainwindow.h"

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
#include <QPushButton>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_widget.h>
#include <qwt_picker_machine.h>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE
#include "GUI/Help.h"
#include "GUI/PerPicture.h"
#include "Core/Core.h"
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

//***************************************************************************
// Constants
//***************************************************************************


//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::Pictures_Init()
{
    Pictures_Widgets=NULL;
    Control_Widgets=NULL;
    Picture_Main=NULL;
    Picture_Main_X=0;
}

//---------------------------------------------------------------------------
void MainWindow::Pictures_Create()
{
    Pictures_Widgets=new QWidget(this);
    QHBoxLayout* Pictures_Layout=new QHBoxLayout();
    for (size_t Pos=0; Pos<9; Pos++)
    {
        if (Pos==4)
        {
            Labels_Middle = new QToolButton(this);
            connect(Labels_Middle, SIGNAL(clicked(bool)), this, SLOT(on_Labels_Middle_clicked(bool)));
            Labels_Middle->setIconSize(QSize(72, 72));
            Labels_Middle->setMinimumHeight(84);
            Labels_Middle->setMinimumWidth(84);
            Labels_Middle->setIcon(QPixmap(":/icon/logo.jpg").scaled(72, 72));
            Pictures_Layout->addWidget(Labels_Middle);
        }
        else
        {
            Labels[Pos] = new QLabel(this);
            Labels[Pos]->setMinimumHeight(72);
            Labels[Pos]->setText(QString::number(Pos));
            QPalette Palette(Labels[Pos]->palette());
            Palette.setColor(QPalette::Window, Qt::darkGray);
            Labels[Pos]->setAutoFillBackground(true);
            Labels[Pos]->setPalette(Palette);
            Labels[Pos]->setAlignment(Qt::AlignCenter);
            Pictures_Layout->addWidget(Labels[Pos]);
        }
    }
    Pictures_Widgets->setLayout(Pictures_Layout);
    Pictures_Widgets->setVisible(false);

    // Control
    Control_Widgets=new QWidget(this);
    QHBoxLayout* Control_Layout=new QHBoxLayout();

    Control_Minus=new QPushButton(this);
    connect(Control_Minus, SIGNAL(clicked(bool)), this, SLOT(on_Control_Minus1_clicked(bool)));
    Control_Minus->setText("Previous");
    Control_Layout->addWidget(Control_Minus);

    Control_Info=new QLabel(this);
    QPalette Palette(Control_Info->palette());
    Palette.setColor(QPalette::Window, Qt::darkGray);
    Control_Info->setAutoFillBackground(true);
    Control_Info->setPalette(Palette);
    Control_Info->setAlignment(Qt::AlignCenter);
    Control_Layout->addWidget(Control_Info);

    Control_Plus=new QPushButton(this);
    connect(Control_Plus, SIGNAL(clicked(bool)), this, SLOT(on_Control_Plus1_clicked(bool)));
    Control_Plus->setText("Next");
    Control_Layout->addWidget(Control_Plus);

    Control_Widgets->setLayout(Control_Layout);
    ui->verticalLayout->addWidget(Control_Widgets);
}

//---------------------------------------------------------------------------
void MainWindow::Pictures_Update(size_t Picture_Pos)
{
    if (Frames_Total==0)
        return; // Problem    

    Picture_Main_X=Picture_Pos;
        
    double FrameRate=Frames_Total/Files[Files_Pos]->BasicInfo->Duration_Get();
    int Milliseconds=(int)((Picture_Main_X/FrameRate*1000)+0.5); //Rounding
    string Time;
    int H1=Milliseconds/36000000; Milliseconds%=36000000;
    int H2=Milliseconds/ 3600000; Milliseconds%= 3600000;
    int M1=Milliseconds/  600000; Milliseconds%=  600000;
    int M2=Milliseconds/   60000; Milliseconds%=   60000;
    int S1=Milliseconds/   10000; Milliseconds%=   10000;
    int S2=Milliseconds/    1000; Milliseconds%=    1000;
    int m1=Milliseconds/     100; Milliseconds%=     100;
    int m2=Milliseconds/      10; Milliseconds%=      10;
    int m3=Milliseconds         ;
    Time.append(1, '0'+H1);
    Time.append(1, '0'+H2);
    Time.append(1, ':');
    Time.append(1, '0'+M1);
    Time.append(1, '0'+M2);
    Time.append(1, ':');
    Time.append(1, '0'+S1);
    Time.append(1, '0'+S2);
    Time.append(1, '.');
    Time.append(1, '0'+m1);
    Time.append(1, '0'+m2);
    Time.append(1, '0'+m3);
    Control_Info->setText(QString().fromUtf8(Time.c_str())+" (Frame "+QString::number(Picture_Main_X)+")");

    if (Picture_Pos>=Frames_Total)
        Picture_Pos=Frames_Total-1;
        
    for (size_t Pos=0; Pos<9; Pos++)
    {
        if (Pos==4)
            Labels_Middle->setIcon(*Files[Files_Pos]->Thumbnails->Picture_Get(Picture_Pos, Pos));
        else if ((Pos>4 || 4-Pos<=Picture_Pos) && (Pos<4 || Picture_Pos-4+Pos<Frames_Total))
            Labels[Pos]->setPixmap(*Files[Files_Pos]->Thumbnails->Picture_Get(Picture_Pos-4+Pos, Pos));
        else
            Labels[Pos]->setPixmap(QPixmap());
    }

    if (Picture_Main)
        Picture_Main->ShowPicture(Picture_Main_X, Files[Files_Pos]->Stats->y, FileName, Files[0]);

    Control_Minus->setEnabled(Picture_Main_X);
    Control_Plus->setEnabled(Picture_Main_X+1!=Frames_Total);
}

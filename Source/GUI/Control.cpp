/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "GUI/Control.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/TinyDisplay.h"
#include "GUI/Info.h"
#include "GUI/FileInformation.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QToolButton>
#include <QPushButton>
#include <QTimer>
#include <QGridLayout>
#include <QLabel>
#include <QDateTime>
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
Control::Control(QWidget *parent, FileInformation* FileInformationData_, style Style_, bool IsSlave_):
    QWidget(parent),
    FileInfoData(FileInformationData_),
    Style(Style_),
    IsSlave(IsSlave_)
{
    // To update
    TinyDisplayArea=NULL;
    InfoArea=NULL;

    // Positioning info
    Frames_Pos=0;

    QFont Font=QFont();
    #ifdef _WIN32
        Font.setPointSize(8);
    #else //_WIN32
        Font.setPointSize(8);
    #endif //_WIN32

    // Control
    QGridLayout* Layout=new QGridLayout();
    Layout->setSpacing(0);
    Layout->setMargin(0);
    Layout->setContentsMargins(0,0,0,0);

    M2=new QToolButton(this);
    M2->setText("-2x");
    M2->setFont(Font);
    connect(M2, SIGNAL(clicked(bool)), this, SLOT(on_M2_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(M2, 0, 0, 1, 1);
    else
        Layout->addWidget(M2, 0, 0, 1, 1);

    M1=new QToolButton(this);
    M1->setText("-1x");
    M1->setFont(Font);
    connect(M1, SIGNAL(clicked(bool)), this, SLOT(on_M1_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(M1, 0, 1, 1, 1);
    else
        Layout->addWidget(M1, 0, 1, 1, 1);

    M0=new QToolButton(this);
    M0->setText("-0.5x");
    M0->setFont(Font);
    connect(M0, SIGNAL(clicked(bool)), this, SLOT(on_M0_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(M0, 0, 2, 1, 1);
    else
        Layout->addWidget(M0, 0, 2, 1, 1);

    Minus=new QPushButton(this);
    connect(Minus, SIGNAL(clicked(bool)), this, SLOT(on_Minus_clicked(bool)));
    Minus->setText("Previous");
    //Minus->setFont(Font);
    if (Style==Style_Cols)
        Layout->addWidget(Minus, 0, 3, 1, 1);
    else
        Layout->addWidget(Minus, 1, 0, 1, 3);

    OM=new QToolButton(this);
    OM->setText("||");
    OM->setFont(Font);
    connect(OM, SIGNAL(clicked(bool)), this, SLOT(on_OM_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(OM, 0, 4, 1, 1);
    else
        Layout->addWidget(OM, 2, 1, 1, 1);

    Info=new QLabel(this);
    QPalette Palette(Info->palette());
    Palette.setColor(QPalette::Window, Qt::darkGray);
   // Info->setFont(Font);
    Info->setAutoFillBackground(true);
    Info->setPalette(Palette);
    Info->setAlignment(Qt::AlignCenter);
    if (Style==Style_Cols)
        Layout->addWidget(Info, 0, 5, 1, 1);
    else
        Layout->addWidget(Info, 3, 0, 1, 3);

    OP=new QToolButton(this);
    OP->setText("||");
    OP->setFont(Font);
    connect(OP, SIGNAL(clicked(bool)), this, SLOT(on_OP_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(OP, 0, 6, 1, 1);
    else
        Layout->addWidget(OP, 4, 1, 1, 1);

    Plus=new QPushButton(this);
    connect(Plus, SIGNAL(clicked(bool)), this, SLOT(on_Plus_clicked(bool)));
    Plus->setText("    Next    ");
    //Plus->setFont(Font);
    if (Style==Style_Cols)
        Layout->addWidget(Plus, 0, 7, 1, 1);
    else
        Layout->addWidget(Plus, 5, 0, 1, 3);

    P0=new QToolButton(this);
    P0->setText("+0.5x");
    P0->setFont(Font);
    connect(P0, SIGNAL(clicked(bool)), this, SLOT(on_P0_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(P0, 0, 8, 1, 1);
    else
        Layout->addWidget(P0, 6, 0, 1, 1);

    P1=new QToolButton(this);
    P1->setText("+1x");
    P1->setFont(Font);
    connect(P1, SIGNAL(clicked(bool)), this, SLOT(on_P1_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(P1, 0, 9, 1, 1);
    else
        Layout->addWidget(P1, 6, 1, 1, 1);

    P2=new QToolButton(this);
    P2->setText("+2x");
    P2->setFont(Font);
    connect(P2, SIGNAL(clicked(bool)), this, SLOT(on_P2_clicked(bool)));
    if (Style==Style_Cols)
        Layout->addWidget(P2, 0, 10, 1, 1);
    else
        Layout->addWidget(P2, 6, 2, 1, 1);

    setLayout(Layout);

    Timer=NULL;
    SelectedSpeed=Speed_O;

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;
    DateTime=-1;
    Timer_Duration=0;

    Update();
}

//---------------------------------------------------------------------------
Control::~Control()
{
    if (Timer)
        Timer->stop();
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void Control::Update()
{
    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;

    int Milliseconds;
    if (FileInfoData && Frames_Pos)
    {
        double FrameRate=FileInfoData->Glue->VideoFrameCount/FileInfoData->Glue->VideoDuration;
        Milliseconds=(int)((Frames_Pos/FrameRate*1000)+0.5); //Rounding
    }
    else
        Milliseconds=0;
    {
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
    Info->setText(QString().fromUtf8(Time.c_str())+(Style==Style_Cols?" ":"\n")+"(Frame "+QString::number(Frames_Pos)+")");
    }

    // Controls
    if (Frames_Pos==0)
    {
        if (Timer)
            Timer->stop();
        M2->setEnabled(false);
        M1->setEnabled(false);
        M0->setEnabled(false);
        Minus->setEnabled(false);
        OM->setEnabled(false);
        OP->setEnabled(false);
        Plus->setEnabled(true);
        P0->setEnabled(true);
        P1->setEnabled(true);
        P2->setEnabled(true);

        if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
        {
            TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
            TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
        }
    }
    else if (Frames_Pos+1==FileInfoData->Glue->VideoFrameCount)
    {
        if (Timer)
            Timer->stop();
        M2->setEnabled(true);
        M1->setEnabled(true);
        M0->setEnabled(true);
        Minus->setEnabled(true);
        OM->setEnabled(false);
        OP->setEnabled(false);
        Plus->setEnabled(false);
        P0->setEnabled(false);
        P1->setEnabled(false);
        P2->setEnabled(false);

        if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
        {
            TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
            TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
            TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
        }
    }
    else
    {    
        if (SelectedSpeed==Speed_O && Frames_Pos)
        {
            M2->setEnabled(true);
            M1->setEnabled(true);
            M0->setEnabled(true);
            Minus->setEnabled(true);
            OM->setEnabled(false);
            OP->setEnabled(false);

            if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
            {
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
                TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
            }
        }
        if (SelectedSpeed==Speed_O && Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount)
        {
            OM->setEnabled(false);
            OP->setEnabled(false);
            Plus->setEnabled(Plus);
            P0->setEnabled(true);
            P1->setEnabled(true);
            P2->setEnabled(true);

            if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
            {
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
                TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
                TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
            }
        }
    }
}

//---------------------------------------------------------------------------
void Control::on_Minus_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    FileInfoData->Frames_Pos_Minus();
}

//---------------------------------------------------------------------------
void Control::on_Plus_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    FileInfoData->Frames_Pos_Plus();
}

//---------------------------------------------------------------------------
void Control::on_M2_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_M2;
    Timer_Duration=16;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(false);
    M1->setEnabled(true);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_M1_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_M1;
    Timer_Duration=33;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(false);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_M0_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_M0;
    Timer_Duration=66;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_OM_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_O;
    Minus->setEnabled(Frames_Pos);
    Plus->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    M2->setEnabled(Frames_Pos);
    M1->setEnabled(Frames_Pos);
    M0->setEnabled(Frames_Pos);
    OM->setEnabled(false);
    OP->setEnabled(false);
    P0->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    P1->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    P2->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    if (Timer)
        Timer->stop();
}

//---------------------------------------------------------------------------
void Control::on_OP_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_O;
    Minus->setEnabled(Frames_Pos);
    Plus->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    M2->setEnabled(Frames_Pos);
    M1->setEnabled(Frames_Pos);
    M0->setEnabled(Frames_Pos);
    OM->setEnabled(false);
    OP->setEnabled(false);
    P0->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    P1->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);
    P2->setEnabled(Frames_Pos+1!=FileInfoData->Glue->VideoFrameCount);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    if (Timer)
        Timer->stop();
}

//---------------------------------------------------------------------------
void Control::on_P0_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_P0;
    Timer_Duration=66;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(false);
    P1->setEnabled(true);
    P2->setEnabled(true);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_P1_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_P1;
    Timer_Duration=33;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(false);
    P2->setEnabled(true);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_P2_clicked(bool checked)
{
    if (IsSlave)
        return;    
        
    SelectedSpeed=Speed_P2;
    Timer_Duration=16;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    OM->setEnabled(true);
    OP->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(false);

    if (TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
    {
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
        TinyDisplayArea->BigDisplayArea->ControlArea->M2->setEnabled(M2->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M1->setEnabled(M1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->M0->setEnabled(M0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Minus->setEnabled(Minus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OM->setEnabled(OM->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->OP->setEnabled(OP->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->Plus->setEnabled(Plus->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P0->setEnabled(P0->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P1->setEnabled(P1->isEnabled());
        TinyDisplayArea->BigDisplayArea->ControlArea->P2->setEnabled(P2->isEnabled());
    }

    TimeOut_Init();
}

//***************************************************************************
// Time
//***************************************************************************

//---------------------------------------------------------------------------
void Control::TimeOut_Init()
{
    DateTime=QDateTime::currentMSecsSinceEpoch();
    if (!Timer)
    {
        Timer=new QTimer(this);
        connect(Timer, SIGNAL(timeout()), this, SLOT(TimeOut()));
    }
    TimeOut();
}

//---------------------------------------------------------------------------
void Control::TimeOut ()
{
    if (Time_MinusPlus)
    {
        if (Frames_Pos+1==FileInfoData->Glue->VideoFrameCount)
        {
            Timer->stop();
            SelectedSpeed=Speed_O;
            if(TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
            return;
        }
        on_Plus_clicked(true); // Plus->click();
    }
    else
    {
        if (Frames_Pos==0)
        {
            Timer->stop();
            SelectedSpeed=Speed_O;
            if(TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;
            return;
        }
        on_Minus_clicked(true); // Minus->click();
    }

    if (DateTime==-1)
        DateTime=QDateTime::currentMSecsSinceEpoch();
    qint64 DateTime_New=QDateTime::currentMSecsSinceEpoch();
    qint64 Diff=DateTime-DateTime_New;
    DateTime+=Timer_Duration;
    if (Diff<0)
        Diff=0;
    if (Diff<=Timer->interval()-3 || Diff>=Timer->interval()+3 || !Timer->isActive())
    {
        Timer->start(Diff);
    }
}

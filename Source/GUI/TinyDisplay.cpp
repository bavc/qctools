/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "GUI/TinyDisplay.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/FileInformation.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QBoxLayout>
#include <QPushbutton>
#include <QToolbutton>
#include <QLabel>
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
TinyDisplay::TinyDisplay(QWidget *parent, FileInformation* FileInformationData_):
    QWidget(parent),
    // File information
    FileInfoData(FileInformationData_)
{
    // To update
    ControlArea=NULL;

    // Positioning info
    Frames_Pos=0;

    BigDisplayArea=NULL;

    //
    QHBoxLayout* Layout=new QHBoxLayout();
    Layout->setSpacing(1);
    Layout->setMargin(1);
    Layout->setContentsMargins(1,0,1,0);
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
            Layout->addWidget(Labels_Middle);
        }
        else
        {
            Labels[Pos] = new QLabel(this);
            Labels[Pos]->setMinimumHeight(72);
            QPalette Palette(Labels[Pos]->palette());
            Palette.setColor(QPalette::Window, Qt::darkGray);
            Labels[Pos]->setAutoFillBackground(true);
            Labels[Pos]->setPalette(Palette);
            Labels[Pos]->setAlignment(Qt::AlignCenter);
            Layout->addWidget(Labels[Pos]);
        }
    }
    setLayout(Layout);

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;
}

//---------------------------------------------------------------------------
TinyDisplay::~TinyDisplay()
{
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void TinyDisplay::Update()
{
    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;
    
    if (Frames_Pos>=FileInfoData->Glue->VideoFrameCount)
        Frames_Pos=FileInfoData->Glue->VideoFrameCount-1;
        
    for (size_t Pos=0; Pos<9; Pos++)
    {
        if (Pos==4)
            Labels_Middle->setIcon(*FileInfoData->Picture_Get(Frames_Pos));
        else if (Frames_Pos+Pos>=4 && Frames_Pos-4+Pos<FileInfoData->Glue->VideoFramePos)
            Labels[Pos]->setPixmap(*FileInfoData->Picture_Get(Frames_Pos-4+Pos));
        else if (Frames_Pos+Pos>=4 && Frames_Pos-4+Pos<FileInfoData->Glue->VideoFrameCount)
        {
            Labels[Pos]->setPixmap(QPixmap(":/icon/logo.jpg").scaled(72, 72));
            ShouldUpate=true;
        }
        else
            Labels[Pos]->setPixmap(QPixmap());
    }

    // BigDisplayArea
    if (BigDisplayArea)
        BigDisplayArea->ShowPicture();
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void TinyDisplay::on_Labels_Middle_clicked(bool checked)
{
    bool ShouldDisplay=true;
    if (BigDisplayArea)
    {
        ShouldDisplay=!BigDisplayArea->isVisible();
        delete BigDisplayArea; BigDisplayArea=NULL;
    }
    
    if (ShouldDisplay)
    {
        BigDisplayArea=new BigDisplay(this, FileInfoData);
        BigDisplayArea->move(geometry().left()+geometry().width(), geometry().top());
        BigDisplayArea->show();
        BigDisplayArea->ShowPicture();
        if (ControlArea)
        {
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M2   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M2_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M1   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M1_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M0   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M0_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Minus, SIGNAL(clicked(bool)), ControlArea, SLOT(on_Minus_clicked(bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->OM   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_OM_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->OP   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_OP_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Plus , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Plus_clicked (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P0   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P0_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P1   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P1_clicked   (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P2   , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P2_clicked   (bool)));
        }
    }
}

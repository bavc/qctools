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
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
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
        Labels[Pos] = new QToolButton(this);
        connect(Labels[Pos], SIGNAL(clicked(bool)), this, SLOT(on_Labels_Middle_clicked(bool)));
        Labels[Pos]->setIconSize(QSize(72, 72));
        Labels[Pos]->setMinimumHeight(84);
        Labels[Pos]->setMinimumWidth(84);
        Labels[Pos]->setIcon(QPixmap(":/icon/logo.jpg").scaled(72, 72));
        //Labels[Pos]->setStyleSheet("border: 5px;");
        Layout->addWidget(Labels[Pos]);
        if (Pos!=4)
            Labels[Pos]->setStyleSheet("background-color: grey;");
    }
    setLayout(Layout);

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;

    // Disable PlayBackFilters if the source file is not available
    if (!FileInfoData->PlayBackFilters_Available())
    {
        for (size_t Pos=0; Pos<9; Pos++)
            Labels[Pos]->setEnabled(false);
    }
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
        if (Frames_Pos+Pos>=4 && Frames_Pos-4+Pos<FileInfoData->Glue->VideoFramePos)
            Labels[Pos]->setIcon(FileInfoData->Picture_Get(Frames_Pos-4+Pos)->copy(0, 0, 72, 72));
        else if (Frames_Pos+Pos>=4 && Frames_Pos-4+Pos<FileInfoData->Glue->VideoFrameCount)
        {
            Labels[Pos]->setIcon(QPixmap());
            ShouldUpate=true;
        }
        else
            Labels[Pos]->setIcon(QPixmap());
    }

    // BigDisplayArea
    if (BigDisplayArea)
        BigDisplayArea->ShowPicture();
}

//---------------------------------------------------------------------------
void TinyDisplay::Filters_Show()
{
    on_Labels_Middle_clicked(true);
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void TinyDisplay::on_Labels_Middle_clicked(bool checked)
{
    // Positioning the current frame if any button but the center button is clicked
    QObject* Sender=sender();
    if (Sender!=Labels[4])
    {
        size_t Pos=0;
        while (Pos<10)
        {
            if (Sender==Labels[Pos])
                break;
            Pos++;
        }
        if (Pos<10)
            FileInfoData->Frames_Pos_Set(FileInfoData->Frames_Pos_Get()+Pos-4);
    }
    
    if (BigDisplayArea==NULL)
    {
        BigDisplayArea=new BigDisplay(this, FileInfoData);
        BigDisplayArea->resize(QApplication::desktop()->screenGeometry().width()-300, QApplication::desktop()->screenGeometry().height()-300);
        BigDisplayArea->move(150, 150); //BigDisplayArea->move(geometry().left()+geometry().width(), geometry().top());
        if (ControlArea)
        {
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M9         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M9_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M2         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M2_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M1         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M1_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M0         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M0_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Minus      , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Minus_clicked     (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->PlayPause  , SIGNAL(clicked(bool)), ControlArea, SLOT(on_PlayPause_clicked (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Pause      , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Pause_clicked     (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Plus       , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Plus_clicked      (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P0         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P0_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P1         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P1_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P2         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P2_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P9         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P9_clicked        (bool)));
        }
    }
    BigDisplayArea->show();
    BigDisplayArea->ShowPicture();
}

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Core/Core.h"

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
#include <QSizePolicy>
#include <QTimer>

#include <sstream>
//---------------------------------------------------------------------------

//***************************************************************************
// Time
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::TimeOut ()
{
    // Configuring plots
    if (PlotsArea==NULL)
    {
        PlotsArea=new Plots(this, Files[Files_Pos]);
        ui->verticalLayout->addWidget(PlotsArea);

        TinyDisplayArea=new TinyDisplay(this, Files[Files_Pos]);
        ui->verticalLayout->addWidget(TinyDisplayArea);

        ControlArea=new Control(this, Files[Files_Pos], Control::Style_Cols);
        ui->verticalLayout->addWidget(ControlArea);

        InfoArea=new Info(this, Files[Files_Pos], Info::Style_Grid);
        ui->verticalLayout->addWidget(InfoArea);

        PlotsArea->TinyDisplayArea=TinyDisplayArea;
        PlotsArea->ControlArea=ControlArea;
        PlotsArea->InfoArea=InfoArea;
        TinyDisplayArea->ControlArea=ControlArea;
        ControlArea->TinyDisplayArea=TinyDisplayArea;
        ControlArea->InfoArea=InfoArea;

        refreshDisplay();
        PlotsArea->createData_Init();

        configureZoom();
        ui->verticalLayout->removeItem(ui->verticalSpacer);
    }
    refreshDisplay();
    Update();

    // Status
    stringstream Message;
    if (Files[Files_Pos]->Glue->VideoFramePos<Files[Files_Pos]->Glue->VideoFrameCount)
    {
        Message<<"Parsing frame "<<Files[Files_Pos]->Glue->VideoFramePos;
        if (Files[Files_Pos]->Glue->VideoFrameCount)
            Message<<"/"<<Files[Files_Pos]->Glue->VideoFrameCount<<" ("<<(int)((double)Files[Files_Pos]->Glue->VideoFramePos)*100/Files[Files_Pos]->Glue->VideoFrameCount<<"%)";
        statusBar()->showMessage(Message.str().c_str());
        QTimer::singleShot(250, this, SLOT(TimeOut()));
    }
    else
    {
        statusBar()->showMessage("Parsing complete", 10000);
        QTimer::singleShot(0, this, SLOT(TimeOut_Refresh()));
    }
}

//---------------------------------------------------------------------------
void MainWindow::TimeOut_Refresh ()
{
    // Hack for refreshing the plots, else plots are not aligned in some cases (e.g. very small file with sidecar stats file). TODO: find the source of the issue
    refreshDisplay();
}
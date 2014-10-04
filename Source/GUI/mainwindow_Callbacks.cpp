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
    if (ui->actionFilesList->isChecked() && FilesListArea==NULL)
        createFilesList();
    if (ui->actionGraphsLayout->isChecked() && PlotsArea==NULL)
        createGraphsLayout();
    refreshDisplay();
    Update();

    // Simultaneous parsing
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
    {
        if (!Files[Files_Pos]->Glue->IsComplete)
            Files[Files_Pos]->Parse();
    }

    // Status
    stringstream Message_Total;
    int Files_Completed=0;
    if (Files.size()>1)
    {
        Message_Total<<", total ";
        int VideoFrameCount_Total=0;
        int VideoFramePos_Total=0;
        for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
        {
            VideoFramePos_Total+=Files[Files_Pos]->Glue->VideoFramePos_Get();
            VideoFrameCount_Total+=Files[Files_Pos]->Glue->VideoFrameCount_Get();
        }
        if (Files_Completed!=Files.size())
            Message_Total<<(int)((double)VideoFramePos_Total)*100/VideoFrameCount_Total<<"%";
    }
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
        if (Files[Files_Pos]->Glue->IsComplete)
            Files_Completed++;

    if (Files_CurrentPos!=(size_t)-1)
    {
        if (!Files[Files_CurrentPos]->Glue->IsComplete)
        {
            stringstream Message;
            Message<<"Parsing frame "<<Files[Files_CurrentPos]->Glue->VideoFramePos_Get();
            if (Files[Files_CurrentPos]->Glue->VideoFrameCount_Get())
                Message<<"/"<<Files[Files_CurrentPos]->Glue->VideoFrameCount_Get()<<" ("<<(int)((double)Files[Files_CurrentPos]->Glue->VideoFramePos_Get())*100/Files[Files_CurrentPos]->Glue->VideoFrameCount_Get()<<"%)";
            QStatusBar* StatusBar=statusBar();
            if (StatusBar)
                StatusBar->showMessage((Message.str()+Message_Total.str()).c_str());
            QTimer::singleShot(250, this, SLOT(TimeOut()));
        }
        else
        {
            QStatusBar* StatusBar=statusBar();
            if (StatusBar)
                StatusBar->showMessage(("Parsing complete"+Message_Total.str()).c_str(), 10000);
            if (Files_Completed==Files.size())
                QTimer::singleShot(0, this, SLOT(TimeOut_Refresh()));
            else
                QTimer::singleShot(250, this, SLOT(TimeOut()));
        }
        if (FilesListArea)
            FilesListArea->Update();
    }
}

//---------------------------------------------------------------------------
void MainWindow::TimeOut_Refresh ()
{
    // Hack for refreshing the plots, else plots are not aligned in some cases (e.g. very small file with sidecar stats file). TODO: find the source of the issue
    refreshDisplay();
}
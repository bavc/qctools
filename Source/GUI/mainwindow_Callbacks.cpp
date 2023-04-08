/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Core/Core.h"
#include "Core/CommonStats.h"
#include "GUI/Plots.h"

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
    Update();

    // this code seems to launch more parsings tasks
    // 2do: move it to proper place (introduce parsingDone signal, initiate more parsers in slot if required)

    // Simultaneous parsing
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
    {
        CommonStats* Stats=Files[Files_Pos]->ReferenceStat();
        if (Stats && Stats->State_Get()==0)
            Files[Files_Pos]->startParse();
    }

    bool DeckRunning_New=false;

    // Status
    std::stringstream Message_Total;
    int Files_Completed=0;
    if (Files.size()>1)
    {
        Message_Total<<", total ";
        int VideoFrameCount_Total=0;
        int VideoFramePos_Total=0;
        for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
        {
            CommonStats* Stats=Files[Files_Pos]->ReferenceStat();
            if (Stats)
            {
                VideoFramePos_Total+=Stats->x_Current;
                VideoFrameCount_Total+=Stats->x_Current_Max;
            }
        }
        if (Files_Completed!=Files.size())
        {
            if (VideoFrameCount_Total)
                Message_Total<<(int)((double)VideoFramePos_Total)*100/VideoFrameCount_Total<<"%";
            else
                Message_Total<<"100%";
        }
    }
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
    {
        CommonStats* Stats=Files[Files_Pos]->ReferenceStat();
        if (Stats && Stats->State_Get()>=1)
            Files_Completed++;
    }

    if (isFileSelected())
    {
        CommonStats* Stats=Files[getFilesCurrentPos()]->ReferenceStat();
        if (Stats && Stats->State_Get()<1)
        {
            qDebug() << "reading stats.... ";

            std::stringstream Message;
            Message<<"Parsing frame "<<Stats->x_Current;
            if (Stats->x_Current_Max)
                Message<<"/"<<Stats->x_Current_Max<<" ("<<(int)((double)Stats->x_Current)*100/Stats->x_Current_Max<<"%)";
            QStatusBar* StatusBar=statusBar();
            if (StatusBar)
                StatusBar->showMessage((Message.str()+Message_Total.str()).c_str());
            QTimer::singleShot(250, this, SLOT(TimeOut()));
        }
        else if(!Files[getFilesCurrentPos()]->parsed())
        {
            qDebug() << "parsing.... ";
            QStatusBar* StatusBar=statusBar();
            if (StatusBar)
                StatusBar->showMessage("Parsing...");
            QTimer::singleShot(250, this, SLOT(TimeOut()));
        }
        else
        {
            qDebug() << "parsing done.... ";

            QStatusBar* StatusBar=statusBar();
            if (StatusBar)
                StatusBar->showMessage(("Parsing complete"+Message_Total.str()).c_str(), 10000);
            if (Files_Completed != Files.size())
                QTimer::singleShot(250, this, SLOT(TimeOut()));
        }
        if (FilesListArea)
            FilesListArea->Update();
    }

    if (PlotsArea)
        PlotsArea->refresh();
}

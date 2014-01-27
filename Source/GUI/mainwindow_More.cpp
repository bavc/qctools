/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

#include "Core/Core.h"
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************


//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::openFile()
{
    processFile(QFileDialog::getOpenFileName(this, "Open file", "", "Video files (*.avi *.mkv *.mov *.mxf *.mp4);;Statistic files (*.csv);;All (*.*)"));
}

//---------------------------------------------------------------------------
void MainWindow::processFile(const QString &FileName)
{
    if (FileName.isEmpty())
        return;

    // Files (must be deleted first in order to stop ffmpeg processes)
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];    
    Files.clear();
    ui->fileNamesBox->clear();

    // Layout
    QLayout* Layout=layout();
    if (PlotsArea)
    {
        ui->verticalLayout->removeWidget(PlotsArea);
        delete PlotsArea; PlotsArea=NULL;
    }
    if (TinyDisplayArea)
    {
        ui->verticalLayout->removeWidget(TinyDisplayArea);
        delete TinyDisplayArea; TinyDisplayArea=NULL;
    }
    if (ControlArea)
    {
        ui->verticalLayout->removeWidget(ControlArea);
        delete ControlArea; ControlArea=NULL;
    }
    if (InfoArea)
    {
        ui->verticalLayout->removeWidget(InfoArea);
        delete InfoArea; InfoArea=NULL;
    }

    // Status
    statusBar()->showMessage("Scanning "+QFileInfo(FileName).fileName()+"...");

    // Launch analysis
    Files.push_back(new FileInformation(this, FileName));
    Files_Pos=0;
    ui->fileNamesBox->addItem(FileName);

    // Coherency
    if (Files[Files_Pos]->Glue->VideoFrameCount==0)
    {
        statusBar()->showMessage("Unsupported format", 10000);
        return;
    }

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::Update()
{
    if (TinyDisplayArea)
        TinyDisplayArea->Update();
    if (ControlArea)
        ControlArea->Update();
    if (InfoArea)
        InfoArea->Update();
}
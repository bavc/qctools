/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QLabel>
#include <QCheckBox>
#include <QPixmap>
#include <QFont>
#include <QPalette>
#include <QMessageBox>

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
    //processFile(QFileDialog::getOpenFileName(this, "Open file", "", "Video files (*.avi *.mkv *.mov *.mxf *.mp4);;Statistic files (*.csv);;All (*.*)"));

    QStringList List=QFileDialog::getOpenFileNames(this, "Open file", "", "Video files (*.avi *.mkv *.mov *.mxf *.mp4);;Statistic files (*.qctools.xml *.qctools.xml.gz *.xml.gz *.xml);;All (*.*)", 0, QFileDialog::DontUseNativeDialog);
    if (List.empty())
        return;

    clearFiles();
    for (int Pos=0; Pos<List.size(); Pos++)
    {
        addFile(List[Pos]);
    }
}

//---------------------------------------------------------------------------
void MainWindow::closeFile()
{
    if (Files_CurrentPos==(size_t)-1)
        return;
    if (Files.size()==1)
    {
        closeAllFiles();
        return;
    }

    // Launch analysis
    Files.erase(Files.begin()+Files_CurrentPos);
    ui->fileNamesBox->removeItem(Files_CurrentPos);
    if (ui->fileNamesBox->isVisible())
        ui->fileNamesBox->setCurrentIndex(Files_CurrentPos<Files.size()?Files_CurrentPos:Files_CurrentPos-1);
    else
        Files_CurrentPos=Files_CurrentPos<Files.size()?Files_CurrentPos:Files_CurrentPos-1;

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::closeAllFiles()
{
    if (FilesListArea)
        FilesListArea->hide();
    clearGraphsLayout();
    Files.clear();
    ui->fileNamesBox->clear();
    createDragDrop();
    ui->actionFilesList->setChecked(false);
    ui->actionGraphsLayout->setChecked(false);
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
    if (FilesListArea)
    {
        ui->verticalLayout->removeWidget(FilesListArea);
        delete FilesListArea; FilesListArea=NULL;
    }
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
    Files_CurrentPos=0;
    ui->fileNamesBox->addItem(FileName);

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::clearFiles()
{
    // Files (must be deleted first in order to stop ffmpeg processes)
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];
    Files.clear();
    ui->fileNamesBox->clear();

    // Layout
    if (FilesListArea)
    {
        ui->verticalLayout->removeWidget(FilesListArea);
        delete FilesListArea; FilesListArea=NULL;
    }
    clearGraphsLayout();

    Files_CurrentPos=(size_t)-1;
}

//---------------------------------------------------------------------------
void MainWindow::clearDragDrop()
{
    if (DragDrop_Image)
    {
        ui->verticalLayout->removeWidget(DragDrop_Image);
        delete DragDrop_Image; DragDrop_Image=NULL;
    }

    if (DragDrop_Text)
    {
        ui->verticalLayout->removeWidget(DragDrop_Text);
        delete DragDrop_Text; DragDrop_Text=NULL;
    }
}

//---------------------------------------------------------------------------
void MainWindow::createDragDrop()
{
    clearDragDrop();

    QFont Font;
    Font.setPointSize(Font.pointSize()*4);

    DragDrop_Image=new QLabel(this);
    DragDrop_Image->setAlignment(Qt::AlignCenter);
    DragDrop_Image->setPixmap(QPixmap(":/icon/dropfiles.png").scaled(256, 256));
    if (Files_CurrentPos!=(size_t)-1)
        DragDrop_Image->hide();
    ui->verticalLayout->addWidget(DragDrop_Image);

    DragDrop_Text=new QLabel(this);
    DragDrop_Text->setAlignment(Qt::AlignCenter);
    DragDrop_Text->setFont(Font);
    QPalette Palette(DragDrop_Text->palette());
    Palette.setColor(QPalette::WindowText, Qt::darkGray);
    DragDrop_Text->setPalette(Palette);
    DragDrop_Text->setText("Drop video file(s) here");
    if (Files_CurrentPos!=(size_t)-1)
        DragDrop_Text->hide();
    ui->verticalLayout->addWidget(DragDrop_Text);
}

//---------------------------------------------------------------------------
void MainWindow::clearFilesList()
{
    if (FilesListArea)
    {
        ui->verticalLayout->removeWidget(FilesListArea);
        delete FilesListArea; FilesListArea=NULL;
    }
}

//---------------------------------------------------------------------------
void MainWindow::createFilesList()
{
    clearFilesList();

    if (Files_CurrentPos==(size_t)-1)
    {
        createDragDrop();
        return;
    }
    clearDragDrop();

    FilesListArea=new FilesList(this);
    if (!ui->actionFilesList->isChecked())
        FilesListArea->hide();
    ui->verticalLayout->addWidget(FilesListArea);
}

//---------------------------------------------------------------------------
void MainWindow::clearGraphsLayout()
{
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
}

//---------------------------------------------------------------------------
void MainWindow::createGraphsLayout()
{
    clearGraphsLayout();

    if (Files_CurrentPos==(size_t)-1)
    {
        for (size_t Pos=0; Pos<PlotType_Max; Pos++)
            if (CheckBoxes[Pos])
                CheckBoxes[Pos]->hide();
        if (ui->fileNamesBox)
            ui->fileNamesBox->hide();

        createDragDrop();
        return;
    }
    clearDragDrop();

    for (size_t Pos=0; Pos<PlotType_Max; Pos++)
        if (CheckBoxes[Pos])
            CheckBoxes[Pos]->show();
    if (ui->fileNamesBox)
        ui->fileNamesBox->show();

    PlotsArea=new Plots(this, Files[Files_CurrentPos]);
    if (!ui->actionGraphsLayout->isChecked())
        PlotsArea->hide();
    ui->verticalLayout->addWidget(PlotsArea);

    TinyDisplayArea=new TinyDisplay(this, Files[Files_CurrentPos]);
    if (!ui->actionGraphsLayout->isChecked())
        TinyDisplayArea->hide();
    ui->verticalLayout->addWidget(TinyDisplayArea);

    ControlArea=new Control(this, Files[Files_CurrentPos], PlotsArea, Control::Style_Cols);
    if (!ui->actionGraphsLayout->isChecked())
        ControlArea->hide();
    ui->verticalLayout->addWidget(ControlArea);

    //InfoArea=new Info(this, Files[Files_CurrentPos], Info::Style_Grid);
    //ui->verticalLayout->addWidget(InfoArea);

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

//---------------------------------------------------------------------------
void MainWindow::addFile(const QString &FileName)
{
    if (FileName.isEmpty())
        return;

    // Launch analysis
    FileInformation* Temp=new FileInformation(this, FileName);

    Files.push_back(Temp);
    ui->fileNamesBox->addItem(Temp->FileName);

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::selectFile(int NewFilePos)
{
    Files_CurrentPos=NewFilePos;
}

//---------------------------------------------------------------------------
void MainWindow::selectDisplayFile(int NewFilePos)
{
    selectFile(NewFilePos);
    if (!ui->actionGraphsLayout->isChecked())
        ui->actionGraphsLayout->trigger();
}

//---------------------------------------------------------------------------
void MainWindow::selectDisplayFiltersFile(int NewFilePos)
{
    selectDisplayFile(NewFilePos);
    if (TinyDisplayArea)
        TinyDisplayArea->Filters_Show();
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
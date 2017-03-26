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
#include "GUI/Plots.h"
#include "GUI/draggablechildrenbehaviour.h"
#include "GUI/blackmagicdecklink_userinput.h"
#include "GUI/preferences.h"
#include "GUI/BigDisplay.h"

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
    QFileDialog::Option options = QFileDialog::Option(0);

#if defined(Q_OS_WIN) || defined(Q_OS_MACX)
    // for Windows and Mac (to resolve 'gray-out files' issue (https://github.com/bavc/qctools/issues/293) use the Qt builtin dialog which displays files,
    // other platforms should use the native dialog.
    options = QFileDialog::DontUseNativeDialog;
#endif

    QStringList List=QFileDialog::getOpenFileNames(this, "Open file", "", "Video files (*.avi *.mkv *.mov *.mxf *.mp4);;Statistic files (*.qctools.xml *.qctools.xml.gz *.xml.gz *.xml);;All (*.*)", 0, options);
    if (List.empty())
        return;

    clearFiles();
    for (int Pos=0; Pos<List.size(); Pos++)
    {
        addFile(List[Pos]);
    }

    addFile_finish();
}

//---------------------------------------------------------------------------
#ifdef BLACKMAGICDECKLINK_YES
void MainWindow::openCapture()
{
    if (DeckRunning)
    {
        for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
        {
            if (Files[Files_Pos]->blackmagicDeckLink_Glue)
                Files[Files_Pos]->blackmagicDeckLink_Glue->Stop();
        }

        ui->actionBlackmagicDeckLinkCapture->setIcon(QIcon(":/icon/capture_layout.png"));

        DeckRunning=false;
        return;
    }
        
    BlackmagicDeckLink_UserInput* blackmagicDeckLink_UserInput=new BlackmagicDeckLink_UserInput();
    if (!blackmagicDeckLink_UserInput->exec())
        return;
    
    clearFiles();
    addFile(blackmagicDeckLink_UserInput->Card, blackmagicDeckLink_UserInput->Card->Config_In.FrameCount, blackmagicDeckLink_UserInput->Encoding_FileName.toUtf8().data(), blackmagicDeckLink_UserInput->Encoding_Format.toUtf8().data());
    addFile_finish();

    delete blackmagicDeckLink_UserInput;
}
#endif // BLACKMAGICDECKLINK_YES
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
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];
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
    FileInformation* file = new FileInformation(signalServer, FileName, Prefs->ActiveFilters, Prefs->ActiveAllTracks);
    connect(file, SIGNAL(positionChanged()), this, SLOT(Update()));
    file->setIndex(Files.size());

    Files.push_back(file);
    Files_CurrentPos=0;
    ui->fileNamesBox->addItem(FileName);

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::clearFiles()
{
    if (ControlArea)
        ControlArea->stop();

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

    configureZoom();
}

//---------------------------------------------------------------------------
void MainWindow::createGraphsLayout()
{
    clearGraphsLayout();

    if (Files_CurrentPos==(size_t)-1)
    {
        for (size_t type = 0; type < Type_Max; type++)
            for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
                if (CheckBoxes[type][group])
                    CheckBoxes[type][group]->hide();
        if (ui->fileNamesBox)
            ui->fileNamesBox->hide();

        createDragDrop();
        return;
    }
    clearDragDrop();

    for (size_t type = 0; type < Type_Max; type++)
        for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
            if (CheckBoxes[type][group] && Files_CurrentPos<Files.size() && Files[Files_CurrentPos]->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup])
                CheckBoxes[type][group]->show();
            else
                CheckBoxes[type][group]->hide();
    if (ui->fileNamesBox)
        ui->fileNamesBox->show();

    PlotsArea=Files[Files_CurrentPos]->Stats.empty()?NULL:new Plots(this, Files[Files_CurrentPos]);

    auto filtersInfo = Prefs->loadFilterSelectorsOrder();
    changeFilterSelectorsOrder(filtersInfo);
    if (PlotsArea)
    {
        PlotsArea->changeOrder(filtersInfo);
        if (!ui->actionGraphsLayout->isChecked())
            PlotsArea->hide();

        ui->verticalLayout->addWidget(PlotsArea);
    }

    TinyDisplayArea=new TinyDisplay(this, Files[Files_CurrentPos]);
    if (!ui->actionGraphsLayout->isChecked())
        TinyDisplayArea->hide();
    ui->verticalLayout->addWidget(TinyDisplayArea);

    ControlArea=new Control(this, Files[Files_CurrentPos], Control::Style_Cols);
    ControlArea->setPlayAllFrames(ui->actionPlay_All_Frames->isChecked());

    connect( ControlArea, SIGNAL( currentFrameChanged() ), 
        this, SLOT( on_CurrentFrameChanged() ) );

    if (!ui->actionGraphsLayout->isChecked())
        ControlArea->hide();
    ui->verticalLayout->addWidget(ControlArea);

    //InfoArea=new Info(this, Files[Files_CurrentPos], Info::Style_Grid);
    //ui->verticalLayout->addWidget(InfoArea);

    TinyDisplayArea->ControlArea=ControlArea;
    ControlArea->TinyDisplayArea=TinyDisplayArea;
    ControlArea->InfoArea=InfoArea;

    refreshDisplay();

    configureZoom();
}

//---------------------------------------------------------------------------
void MainWindow::addFile(const QString &FileName)
{
    if (FileName.isEmpty())
        return;

    // Launch analysis
    FileInformation* Temp=new FileInformation(signalServer, FileName, Prefs->ActiveFilters, Prefs->ActiveAllTracks);
    connect(Temp, SIGNAL(positionChanged()), this, SLOT(Update()));
    Temp->setIndex(Files.size());

    Files.push_back(Temp);
    ui->fileNamesBox->addItem(Temp->fileName());
}

//---------------------------------------------------------------------------
void MainWindow::addFile(
#ifdef BLACKMAGICDECKLINK_YES
        BlackmagicDeckLink_Glue* BlackmagicDeckLink_Glue,
#endif // BLACKMAGICDECKLINK_YES
        int FrameCount, const string &Encoding_FileName, const string &Encoding_Format)
{
    // Launch analysis
    FileInformation* Temp=new FileInformation(signalServer, QString(), Prefs->ActiveFilters, Prefs->ActiveAllTracks,
#ifdef BLACKMAGICDECKLINK_YES
                                              BlackmagicDeckLink_Glue,
#endif // BLACKMAGICDECKLINK_YES
                                              FrameCount, Encoding_FileName, Encoding_Format);
    connect(Temp, SIGNAL(positionChanged()), this, SLOT(Update()));
    Temp->setIndex(Files.size());

    Files.push_back(Temp);
    ui->fileNamesBox->addItem(Temp->fileName());
}

//---------------------------------------------------------------------------
void MainWindow::addFile_finish()
{
    if (FilesListArea)
    {
        FilesListArea->UpdateAll();
        FilesListArea->show();
    }
    if (Files.size()>1)
    {
        ui->actionFilesList->trigger();
    }
    else
    {
        if(!Files.empty())
            selectFile(0);

        ui->actionGraphsLayout->trigger();
    }

    TimeOut();

    ui->actionUploadToSignalServer->setEnabled(true);
    ui->actionUploadToSignalServerAll->setEnabled(true);
}

//---------------------------------------------------------------------------
void MainWindow::selectFile(int NewFilePos)
{
    Files_CurrentPos=NewFilePos;

    for(int i = 0; i < Files.size(); ++i)
    {
        FileInformation* file = Files[i];

        if(i == Files_CurrentPos) {
            connect(file, SIGNAL(signalServerCheckUploadedStatusChanged()), this, SLOT(updateSignalServerCheckUploadedStatus()));
            connect(file, SIGNAL(signalServerUploadStatusChanged()), this, SLOT(updateSignalServerUploadStatus()));
            connect(file, SIGNAL(signalServerUploadProgressChanged(qint64, qint64)), this, SLOT(updateSignalServerUploadProgress(qint64, qint64)));
        } else {
            disconnect(file, SIGNAL(signalServerCheckUploadedStatusChanged()), this, SLOT(updateSignalServerCheckUploadedStatus()));
            disconnect(file, SIGNAL(signalServerUploadStatusChanged()), this, SLOT(updateSignalServerUploadStatus()));
            disconnect(file, SIGNAL(signalServerUploadProgressChanged(qint64, qint64)), this, SLOT(updateSignalServerUploadProgress(qint64, qint64)));
        }
    }

    if(!Files.empty())
    {
        FileInformation* file = Files[Files_CurrentPos];
        if(file->signalServerUploadStatus() == FileInformation::Idle)
        {
            updateSignalServerCheckUploadedStatus();
        }
        else
        {
            updateSignalServerUploadStatus();
        }
    }
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

SignalServer *MainWindow::getSignalServer()
{
    return signalServer;
}

//---------------------------------------------------------------------------
void MainWindow::Update()
{
	if (TinyDisplayArea)
        TinyDisplayArea->Update(false);

    if(TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
        TinyDisplayArea->BigDisplayArea->ShowPicture();

	if(ControlArea)
		ControlArea->Update();

	if(InfoArea)
		InfoArea->Update();
}

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
#include <QStandardPaths>

#include "Core/Core.h"
#include "Core/FFmpeg_Glue.h"
#include "GUI/player.h"
#include "GUI/Plots.h"
#include "GUI/draggablechildrenbehaviour.h"
#include "GUI/preferences.h"
#include "GUI/barchartprofilesmodel.h"
#include "GUI/playercontrol.h"
#include <float.h>

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

    QStringList List=QFileDialog::getOpenFileNames(this, "Open file", "", "All (*.*);;\
                                                                           Audio files (*.wav);;\
                                                                           Statistic files (*.qctools.xml *.qctools.xml.gz *.xml.gz *.xml);;\
                                                                           Statistic files with thumbnails (*.qctools.mkv);;\
                                                                           Video files (*.avi *.mkv *.mov *.mxf *.mp4 *.ts *.m2ts)", 0, options);
    if (List.empty())
        return;

    for (int Pos=0; Pos<List.size(); Pos++)
    {
        addFile(List[Pos]);
    }

    addFile_finish();
}

//---------------------------------------------------------------------------
bool MainWindow::canCloseFile(size_t index)
{
    if(Files[index]->commentsUpdated())
    {
        auto result = QMessageBox::warning(this, "Comments has been updated!",
                                           "This report contains unsaved comments. If you close before saving, your changes will be lost.",
                                           QMessageBox::Cancel | QMessageBox::Close, QMessageBox::Cancel);
        return result == QMessageBox::Close;
    }

    return true;
}

void MainWindow::closeFile()
{
    if (getFilesCurrentPos()==(size_t)-1)
        return;
    if (Files.size()==1)
    {
        closeAllFiles();
        return;
    }

    if(canCloseFile(getFilesCurrentPos()))
    {
        // Launch analysis
        Files[getFilesCurrentPos()]->deleteLater();
        Files.erase(Files.begin()+getFilesCurrentPos());

        ui->fileNamesBox->removeItem(getFilesCurrentPos());
        if (ui->fileNamesBox->isVisible())
            ui->fileNamesBox->setCurrentIndex(getFilesCurrentPos()<Files.size()?getFilesCurrentPos():getFilesCurrentPos()-1);
        else
            setFilesCurrentPos(getFilesCurrentPos()<Files.size()?getFilesCurrentPos():getFilesCurrentPos()-1);

        TimeOut();
    }
}

//---------------------------------------------------------------------------
void MainWindow::closeAllFiles()
{
    for(size_t Pos = 0; Pos < Files.size(); Pos++)
    {
        if(!canCloseFile(Pos))
            return;
    }

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
    updateExportAllAction();
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
    if (InfoArea)
    {
        ui->verticalLayout->removeWidget(InfoArea);
        delete InfoArea; InfoArea=NULL;
    }

    // Status
    statusBar()->showMessage("Scanning "+QFileInfo(FileName).fileName()+"...");

    // Launch analysis
    FileInformation* file = new FileInformation(signalServer, FileName, Prefs->ActiveFilters, Prefs->ActiveAllTracks, preferences->getActivePanels(), preferences->createQCvaultFileNameString(FileName));
    connect(file, SIGNAL(positionChanged()), this, SLOT(Update()), Qt::DirectConnection); // direct connection is required here to get Update called from separate thread
    file->setIndex(Files.size());
    file->setExportFilters(Prefs->ActiveFilters);

    Files.push_back(file);
    setFilesCurrentPos(0);
    ui->fileNamesBox->addItem(FileName);

    TimeOut();

    updateRecentFiles(FileName);
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

    setFilesCurrentPos((size_t)-1);
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
    if (getFilesCurrentPos()!=(size_t)-1)
        DragDrop_Image->hide();
    ui->verticalLayout->addWidget(DragDrop_Image);

    DragDrop_Text=new QLabel(this);
    DragDrop_Text->setAlignment(Qt::AlignCenter);
    DragDrop_Text->setFont(Font);
    QPalette Palette(DragDrop_Text->palette());
    Palette.setColor(QPalette::WindowText, Qt::darkGray);
    DragDrop_Text->setPalette(Palette);
    DragDrop_Text->setText("Drop video file(s) here");
    if (getFilesCurrentPos()!=(size_t)-1)
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

    if (getFilesCurrentPos()==(size_t)-1)
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

    if (getFilesCurrentPos()==(size_t)-1)
    {
        if (ui->fileNamesBox)
            ui->fileNamesBox->hide();
        if (ui->copyToClipboard_pushButton)
            ui->copyToClipboard_pushButton->hide();
        if (ui->setupFilters_pushButton)
            ui->setupFilters_pushButton->hide();

        createDragDrop();
        return;
    }

    // here we fix geometry of main window while creating plots
    // to ensure it will not expand
    auto maxSize = maximumSize();
    auto minSize = minimumSize();
    auto currentGeometry = geometry();

    setMinimumSize(currentGeometry.size());
    setMaximumSize(currentGeometry.size());

    // .. and schedule returning min/max sizes
    QTimer::singleShot(0, [this, maxSize, minSize]() {
        setMaximumSize(maxSize);
        setMinimumSize(minSize);
    });

    clearDragDrop();

    m_plotsChooser->setFilterCriteria([this](quint64 type, quint64 group) -> bool {
        auto showFilter = true;
        if(type < Type_Max) {
            showFilter = getFilesCurrentPos()<Files.size() && Files[getFilesCurrentPos()]->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup];
        }

        qDebug() << "type: " << type << ", group: " << group << ", showFilter: " << showFilter;
        return showFilter;
    });


    if (ui->fileNamesBox)
        ui->fileNamesBox->show();
    if (ui->copyToClipboard_pushButton)
        ui->copyToClipboard_pushButton->show();
    if (ui->setupFilters_pushButton)
        ui->setupFilters_pushButton->show();

    PlotsArea=Files[getFilesCurrentPos()]->Stats.empty()?NULL:new Plots(this, Files[getFilesCurrentPos()]);

    connect(PlotsArea, &Plots::barchartProfileChanged, this, [&] {
        auto selectedProfileFileName = m_profileSelectorCombobox->itemData(m_profileSelectorCombobox->currentIndex(), BarchartProfilesModel::Data).toString();
        auto isSystem = m_profileSelectorCombobox->itemData(m_profileSelectorCombobox->currentIndex(), BarchartProfilesModel::IsSystem).toBool();

        if(!isSystem) {
            saveBarchartsProfile(selectedProfileFileName);
        }
    });
    connect(Files[getFilesCurrentPos()], &FileInformation::positionChanged, [&]() {
        PlotsArea->setCursorPos(Files[getFilesCurrentPos()]->Frames_Pos_Get());
    });

    applyBarchartsProfile();

    auto filtersInfo = preferences->loadFilterSelectorsOrder();
    m_plotsChooser->changeOrder(filtersInfo);
    if (PlotsArea)
    {
        PlotsArea->changeOrder(filtersInfo);
        if (!ui->actionGraphsLayout->isChecked())
            PlotsArea->hide();

        ui->verticalLayout->addWidget(PlotsArea);
        if(ui->actionGraphsLayout->isChecked()) {
            // we need force show to get all the charts shown (and prevent Qt from doing it at wrong moment)...
            PlotsArea->show();

            QMap<QString, std::tuple<quint64, quint64>> filters;
            m_plotsChooser->getSelectedFilters(&filters);
            // ... and then hide it accordingly to filters
            PlotsArea->updatePlotsVisibility(filters);
        }
    }

    TinyDisplayArea=new TinyDisplay(this, Files[getFilesCurrentPos()]);
    connect(TinyDisplayArea, &TinyDisplay::thumbnailClicked, this, &MainWindow::showPlayer);
    if (!ui->actionGraphsLayout->isChecked())
        TinyDisplayArea->hide();
    ui->verticalLayout->addWidget(TinyDisplayArea);

    configureZoom();

    auto playPauseButton = PlotsArea->playerControl()->playPauseButton();
    playPauseButton->setIcon(QIcon(":/icon/play.png"));

    if(isFileSelected()) {
        auto averageFrameRate = getCurrenFileInformation()->averageFrameRate();
        auto averageFrameDuration = averageFrameRate != 0.0 ? 1000.0 / averageFrameRate : 0.0;

        m_playbackSimulationTimer.setInterval(averageFrameDuration);
    }

    auto exportButton = const_cast<QPushButton*>(PlotsArea->playerControl()->exportButton());
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        auto fileName = QFileDialog::getSaveFileName(this, "Export plots as image", "", "*.png");
        if(!fileName.isEmpty()) {
            this->PlotsArea->playerControl()->setVisible(false);
            auto image = PlotsArea->grab();
            this->PlotsArea->playerControl()->setVisible(true);

            image.save(fileName);
        }

    }, Qt::UniqueConnection);

    static QIcon pauseButton(":/icon/pause.png");
    static QIcon playButton(":/icon/play.png");

    connect(&m_playbackSimulationTimer, &QTimer::timeout, this, [this]() {
        auto fileInfo = getCurrenFileInformation();
        if(!fileInfo->Frames_Pos_Plus()) {
            m_playbackSimulationTimer.stop();
            PlotsArea->playerControl()->playPauseButton()->setIcon(m_playbackSimulationTimer.isActive() ? pauseButton : playButton);
        }
    }, Qt::UniqueConnection);

    connect(PlotsArea->playerControl()->goToEndButton(), &QPushButton::clicked, this, [this]() {
        auto fileInfo = getCurrenFileInformation();
        fileInfo->Frames_Pos_Set(fileInfo->ReferenceStat()->x_Current_Max);
    }, Qt::UniqueConnection);

    connect(PlotsArea->playerControl()->goToStartButton(), &QPushButton::clicked, this, [this]() {
        auto fileInfo = getCurrenFileInformation();
        fileInfo->Frames_Pos_Set(0);
    }, Qt::UniqueConnection);

    connect(PlotsArea->playerControl()->prevFrameButton(), &QPushButton::clicked, this, [this]() {
        auto fileInfo = getCurrenFileInformation();
        fileInfo->Frames_Pos_Minus();
    }, Qt::UniqueConnection);

    connect(PlotsArea->playerControl()->nextFrameButton(), &QPushButton::clicked, this, [this]() {
        auto fileInfo = getCurrenFileInformation();
        fileInfo->Frames_Pos_Plus();
    }, Qt::UniqueConnection);

    connect(m_player->playPauseButton(), &QPushButton::clicked, this, [this]() {
        PlotsArea->playerControl()->playPauseButton()->setIcon(m_player->playPauseButton()->icon());
    }, Qt::UniqueConnection);

    connect(PlotsArea->playerControl()->playPauseButton(), &QPushButton::clicked, this, [this]() {
        if(isFileSelected()) {
            if(!hasMediaFile()) {
                auto fileInfo = getCurrenFileInformation();
                if(!fileInfo->Frames_Pos_AtEnd()) {
                    if(m_playbackSimulationTimer.isActive()) {
                        m_playbackSimulationTimer.stop();
                    } else {
                        m_playbackSimulationTimer.start();
                    }

                    PlotsArea->playerControl()->playPauseButton()->setIcon(m_playbackSimulationTimer.isActive() ? pauseButton : playButton);
                }
            } else {
                m_player->playPauseButton()->animateClick();
            }
        }
    }, Qt::UniqueConnection);

    auto goToTime_lineEdit = PlotsArea->playerControl()->lineEdit();
    connect(PlotsArea->playerControl()->lineEdit(), &QLineEdit::returnPressed, this, [this, goToTime_lineEdit]() {
        if(isFileSelected()) {
            if(!hasMediaFile()) {
                auto timeValue = goToTime_lineEdit->text();
                qint64 ms = Player::timeStringToMs(timeValue);

                auto fileInfo = getCurrenFileInformation();
                if(fileInfo->ReferenceStat()->x_Current_Max >= 1) {
                    auto millisecondsPerFrame = (int)(fileInfo->ReferenceStat()->x[1][1]*1000);

                    auto frameIndex = qreal(ms) / millisecondsPerFrame;
                    fileInfo->Frames_Pos_Set(frameIndex);
                }
            }
        }
    });

}

//---------------------------------------------------------------------------
void MainWindow::addFile(const QString &FileName)
{
    if (FileName.isEmpty())
        return;

    // Launch analysis
    FileInformation* Temp=new FileInformation(signalServer, FileName, Prefs->ActiveFilters, Prefs->ActiveAllTracks, preferences->getActivePanels(), preferences->createQCvaultFileNameString(FileName));
    if (!Temp->isValid() && !Temp->hasStats())
    {
        delete Temp; //Temp=NULL;
        QMessageBox::warning(this, "Can't open file", QString("File %1 is invalid or unsupported").arg(FileName));
        return;
    }

    connect(Temp, SIGNAL(positionChanged()), this, SLOT(Update()), Qt::DirectConnection); // direct connection is required here to get Update called from separate thread
    connect(Temp, SIGNAL(parsingCompleted(bool)), this, SLOT(updateExportAllAction()));

    Temp->setIndex(Files.size());
    Temp->setExportFilters(Prefs->ActiveFilters);

    Files.push_back(Temp);
    ui->fileNamesBox->addItem(Temp->fileName());

    updateRecentFiles(FileName);
}

//---------------------------------------------------------------------------
void MainWindow::addFile_finish()
{
    if (FilesListArea)
    {
        FilesListArea->UpdateAll();
        FilesListArea->show();

        updateExportActions();
        updateExportAllAction();
    }
    if (Files.size()>1)
    {
        ui->actionFilesList->trigger();

        updateExportActions();
        updateExportAllAction();
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
    setFilesCurrentPos(NewFilePos);

    updateExportActions();
    updateExportAllAction();

    for(int i = 0; i < Files.size(); ++i)
    {
        FileInformation* file = Files[i];

        if(i == getFilesCurrentPos()) {
            connect(file, SIGNAL(signalServerCheckUploadedStatusChanged()), this, SLOT(updateSignalServerCheckUploadedStatus()));
            connect(file, SIGNAL(signalServerUploadStatusChanged()), this, SLOT(updateSignalServerUploadStatus()));
            connect(file, SIGNAL(signalServerUploadProgressChanged(qint64, qint64)), this, SLOT(updateSignalServerUploadProgress(qint64, qint64)));
            connect(file, SIGNAL(parsingCompleted(bool)), this, SLOT(updateExportActions()));
        } else {
            disconnect(file, SIGNAL(signalServerCheckUploadedStatusChanged()), this, SLOT(updateSignalServerCheckUploadedStatus()));
            disconnect(file, SIGNAL(signalServerUploadStatusChanged()), this, SLOT(updateSignalServerUploadStatus()));
            disconnect(file, SIGNAL(signalServerUploadProgressChanged(qint64, qint64)), this, SLOT(updateSignalServerUploadProgress(qint64, qint64)));
            disconnect(file, SIGNAL(parsingCompleted(bool)), this, SLOT(updateExportActions()));
        }
    }

    if(!Files.empty() && isFileSelected())
    {
        FileInformation* file = Files[getFilesCurrentPos()];
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

static QTime zeroTime = QTime::fromString("00:00:00");

//---------------------------------------------------------------------------
void MainWindow::Update()
{
	if (TinyDisplayArea)
        TinyDisplayArea->Update(false);

	if(InfoArea)
        InfoArea->Update();

    if(PlotsArea) {
        auto playerControl = const_cast<PlayerControl*> (PlotsArea->playerControl());
        auto fileInfo = getCurrenFileInformation();
        auto duration = getCurrenFileInformation()->Frames_Count_Get();
        auto framesPos = fileInfo->Frames_Pos_Get();

        playerControl->frameLabel()->setText(QString("Frame %1 [%2]").arg(fileInfo->Frames_Pos_Get()).arg(fileInfo->Frame_Type_Get()));

        int Milliseconds=(int)-1;
        if (fileInfo && !fileInfo->Stats.empty()
         && ( framesPos<fileInfo->ReferenceStat()->x_Current
          || (framesPos<fileInfo->ReferenceStat()->x_Current_Max && fileInfo->ReferenceStat()->x[1][framesPos]))) //Also includes when stats are not ready but timestamp is available
            Milliseconds=(int)(fileInfo->ReferenceStat()->x[1][framesPos]*1000);
        else
        {
            double TimeStamp = fileInfo->TimeStampOfCurrentFrame();
            if (TimeStamp!=DBL_MAX)
                Milliseconds=(int)(TimeStamp*1000);
        }

        if (Milliseconds >= 0)
        {
            QTime time = zeroTime;
            time = time.addMSecs(Milliseconds);
            QString timeString = time.toString("hh:mm:ss.zzz");
            playerControl->timeLabel()->setText(timeString);
        }
        else
            playerControl->timeLabel()->setText("");
    }
}

void MainWindow::applyBarchartsProfile()
{
    if(PlotsArea) {
        PlotsArea->loadBarchartsProfile(m_barchartsProfile.object());
    }
}

void MainWindow::loadBarchartsProfile(const QString &profile)
{
    QFile file(profile);
    if(!file.exists(profile)) {
        return;
    }
    if(!file.open(QIODevice::ReadOnly)) {
        return;
    }

    auto bytes = file.readAll();
    if(m_barchartsProfile.isEmpty())
        qDebug() << "m_barchartsProfile is empty";

    m_barchartsProfile = QJsonDocument::fromJson(bytes);
    applyBarchartsProfile();
}

void MainWindow::saveBarchartsProfile(const QString &profileName)
{
    if(PlotsArea) {
        m_barchartsProfile = QJsonDocument(PlotsArea->saveBarchartsProfile());
        auto json = m_barchartsProfile.toJson();
        QFile profile(profileName);
        if(profile.open(QIODevice::WriteOnly))  {
            profile.write(json);
        }
    }
}

void MainWindow::setPlotVisible(quint64 group, quint64 type, bool visible)
{
    if(type < Type_Max) {
        PlotsArea->setPlotVisible(type, group, visible);
    } else if(type == Type_Comments) {
        PlotsArea->setCommentsVisible(visible);
    } else if(type == Type_Panels) {
        for(size_t panelIndex = 0; panelIndex < PlotsArea->panelsCount(); ++panelIndex) {
            auto panel = PlotsArea->panelsView(panelIndex);
            auto panelGroup = panel->panelGroup();
            if(panelGroup == group) {
                panel->setVisible(visible);
                panel->legend()->setVisible(visible);
            }
        }
    }
}

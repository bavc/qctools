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
#include "GUI/preferences.h"
#include "GUI/barchartprofilesmodel.h"
#include "GUI/player.h"

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
    FileInformation* file = new FileInformation(signalServer, FileName, Prefs->ActiveFilters, Prefs->ActiveAllTracks);
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
        for (size_t type = 0; type < Type_Max; type++)
            for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
                if (CheckBoxes[type][group])
                    CheckBoxes[type][group]->hide();

        m_commentsCheckbox->hide();

        if (ui->fileNamesBox)
            ui->fileNamesBox->hide();

        createDragDrop();
        return;
    }
    clearDragDrop();

    for (size_t type = 0; type < Type_Max; type++)
        for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
            if (CheckBoxes[type][group] && getFilesCurrentPos()<Files.size() && Files[getFilesCurrentPos()]->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup])
                CheckBoxes[type][group]->show();
            else
                CheckBoxes[type][group]->hide();
    if (ui->fileNamesBox)
        ui->fileNamesBox->show();

    PlotsArea=Files[getFilesCurrentPos()]->Stats.empty()?NULL:new Plots(this, Files[getFilesCurrentPos()]);
    connect(PlotsArea, &Plots::barchartProfileChanged, this, [&] {
        auto selectedProfileFileName = m_profileSelectorCombobox->itemData(m_profileSelectorCombobox->currentIndex(), BarchartProfilesModel::Data).toString();
        auto isSystem = m_profileSelectorCombobox->itemData(m_profileSelectorCombobox->currentIndex(), BarchartProfilesModel::IsSystem).toBool();

        if(!isSystem) {
            saveBarchartsProfile(selectedProfileFileName);
        }
    });

    applyBarchartsProfile();

    auto filtersInfo = Prefs->loadFilterSelectorsOrder();
    changeFilterSelectorsOrder(filtersInfo);
    if (PlotsArea)
    {
        PlotsArea->changeOrder(filtersInfo);
        if (!ui->actionGraphsLayout->isChecked())
            PlotsArea->hide();

        ui->verticalLayout->addWidget(PlotsArea);
    }

    TinyDisplayArea=new TinyDisplay(this, Files[getFilesCurrentPos()]);
    connect(TinyDisplayArea, &TinyDisplay::thumbnailClicked, this, &MainWindow::showPlayer);
    if (!ui->actionGraphsLayout->isChecked())
        TinyDisplayArea->hide();
    ui->verticalLayout->addWidget(TinyDisplayArea);

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

//---------------------------------------------------------------------------
void MainWindow::Update()
{
	if (TinyDisplayArea)
        TinyDisplayArea->Update(false);

	if(InfoArea)
        InfoArea->Update();
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

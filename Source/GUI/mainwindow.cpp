/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "player.h"
#include "ui_mainwindow.h"
#include "Core/FFmpeg_Glue.h"
#include "GUI/Plots.h"
#include "GUI/preferences.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QSizePolicy>
#include <QScrollArea>
#include <QPrinter>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QInputDialog>
#include <QCheckBox>
#include <QTimer>
#include <QDebug>
#include <QMetaEnum>
#include <QMessageBox>
#include <QJsonDocument>

#include "GUI/draggablechildrenbehaviour.h"
#include "GUI/config.h"

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
QList<std::tuple<int, int>> MainWindow::getFilterSelectorsOrder(int start = 0, int end = -1)
{
    QList<std::tuple<int, int>> filtersInfo;
    if(end == -1)
        end = ui->horizontalLayout->count() - 1;

    for(int i = start; i <= end; ++i)
    {
        auto o = ui->horizontalLayout->itemAt(i)->widget();
        auto group = o->property("group").toInt();
        auto type = o->property("type").toInt();

        filtersInfo.push_back(std::make_tuple(group, type));
    }

    return filtersInfo;
}

QStringList MainWindow::getSelectedFilters() const
{
    QStringList selectedFilters;

    for (size_t type = 0; type < Type_Max; type++)
    {
        for(auto checkbox : CheckBoxes[type])
        {
            if(checkbox->isChecked())
                selectedFilters << checkbox->text();
        }
    }

    if(m_commentsCheckbox->isChecked())
        selectedFilters << m_commentsCheckbox->text();

    return selectedFilters;
}

QAction *MainWindow::uploadAction() const
{
    return ui->actionUploadToSignalServer;
}

QAction *MainWindow::uploadAllAction() const
{
    return ui->actionUploadToSignalServerAll;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    Thumbnails_Modulo(1),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<SharedFile>("SharedFile");

    // FilesList
    FilesListArea=NULL;

    // Plots
    PlotsArea=NULL;

    // Pictures
    TinyDisplayArea=NULL;

    // Control
    ControlArea=NULL;

    // Info
    InfoArea=NULL;

    // Info
    DragDrop_Image=NULL;
    DragDrop_Text=NULL;

    m_player = new Player();

    // UI
    Ui_Init();

    // Files
    setFilesCurrentPos((size_t)-1);

    // Deck
    DeckRunning=false;

    draggableBehaviour = new DraggableChildrenBehaviour(ui->horizontalLayout);
    connect(draggableBehaviour, &DraggableChildrenBehaviour::childPositionChanged, [&](QWidget* child, int oldPos, int newPos) {

        Q_UNUSED(child);

        int start = oldPos;
        int end = newPos;

        if(oldPos > newPos)
        {
            start = newPos;
            end = oldPos;
        }

        QList<std::tuple<int, int>> filtersSelectors = getFilterSelectorsOrder();

        if(PlotsArea)
            PlotsArea->changeOrder(filtersSelectors);
    });
}

//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    Prefs->saveFilterSelectorsOrder(getFilterSelectorsOrder());

    preferences->saveSelectedFilters(getSelectedFilters());
    // Controls
    delete ControlArea;

    // Files (must be deleted first in order to stop ffmpeg processes)
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];

    // Plots
    delete PlotsArea;

    // Pictures
    delete TinyDisplayArea;

    // Info
    delete InfoArea;

    // UI
    delete ui;
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::on_actionQuit_triggered()
{
    for(size_t Pos = 0; Pos < Files.size(); Pos++)
    {
        if(!canCloseFile(Pos))
            return;
    }

    close();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionOpen_triggered()
{
    openFile();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionClose_triggered()
{
    closeFile();
    if (FilesListArea && ui->actionFilesList->isChecked())
        FilesListArea->UpdateAll();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionCloseAll_triggered()
{
    closeAllFiles();
}

//---------------------------------------------------------------------------
void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
    Zoom_Move(value);
}

//---------------------------------------------------------------------------
void MainWindow::on_actionZoomIn_triggered()
{
    Zoom_In();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionZoomOne_triggered()
{
    PlotsArea->zoomXAxis( Plots::ZoomOneToOne );
    configureZoom();

    updateScrollBar();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionZoomOut_triggered()
{
    Zoom_Out();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionGoTo_triggered()
{
    if (!ControlArea && !TinyDisplayArea) //TODO: without TinyDisplayArea
        return;

    if (getFilesCurrentPos()>=Files.size())
        return;

    bool ok;
    int i = QInputDialog::getInt(this, tr("Go to frame at position..."), Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max?("frame position (0-"+QString::number(Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max-1)+"):"):QString("frame position (0-based)"), Files[getFilesCurrentPos()]->Frames_Pos_Get(), 0, Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max-1, 1, &ok);
    if (Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max && i>=Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max)
        i=Files[getFilesCurrentPos()]->ReferenceStat()->x_Current_Max-1;
    if (ok)
    {
        Files[getFilesCurrentPos()]->Frames_Pos_Set(i);
    }
}

//---------------------------------------------------------------------------
void MainWindow::on_actionToolbar_triggered()
{
    ui->toolBar->setVisible(ui->actionToolbar->isChecked());
}

//---------------------------------------------------------------------------
void MainWindow::on_Toolbar_visibilityChanged(bool visible)
{
    ui->actionToolbar->setChecked(visible);
}

//---------------------------------------------------------------------------
void MainWindow::on_actionImport_XmlGz_Prompt_triggered()
{
    QString FileName=QFileDialog::getOpenFileName(this, "Import from .qctools.xml.gz / .qctools.mkv", "",
                                                  "Statistic files (*.qctools.xml *.qctools.xml.gz *.xml.gz *.xml);;\
                                                   Statistic files with thumbnails (*.qctools.mkv)",
                                                   0, QFileDialog::DontUseNativeDialog);
    if (FileName.size()==0)
        return;

    processFile(FileName);
}

//---------------------------------------------------------------------------
void MainWindow::on_actionExport_XmlGz_Prompt_triggered()
{
    if (getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    QString FileName=QFileDialog::getSaveFileName(this, "Export to .qctools.xml.gz", Files[getFilesCurrentPos()]->fileName() + ".qctools.xml.gz", "Statistic files (*.qctools.xml *.qctools.xml.gz *.xml.gz *.xml)", 0, QFileDialog::DontUseNativeDialog);
    if (FileName.size()==0)
        return;

    Files[getFilesCurrentPos()]->Export_XmlGz(FileName, Prefs->ActiveFilters);
    statusBar()->showMessage("Exported to "+FileName);
}

void MainWindow::on_actionExport_Mkv_Prompt_triggered()
{
    if (getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    QString FileName=QFileDialog::getSaveFileName(this, "Export to .qctools.mkv", Files[getFilesCurrentPos()]->fileName() + ".qctools.mkv", "Statistic files (*.qctools.mkv)", 0, QFileDialog::DontUseNativeDialog);
    if (FileName.size()==0)
        return;

    Files[getFilesCurrentPos()]->Export_QCTools_Mkv(FileName, Prefs->ActiveFilters);
    statusBar()->showMessage("Exported to "+FileName);
}

//---------------------------------------------------------------------------
void MainWindow::on_actionExport_XmlGz_Sidecar_triggered()
{
    if (getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    QString FileName=Files[getFilesCurrentPos()]->fileName() + ".qctools.xml.gz";

    Files[getFilesCurrentPos()]->Export_XmlGz(FileName, Prefs->ActiveFilters);
    statusBar()->showMessage("Exported to "+FileName);

}

//---------------------------------------------------------------------------
void MainWindow::on_actionExport_XmlGz_SidecarAll_triggered()
{
    for (size_t Pos=0; Pos<Files.size(); ++Pos)
    {
        QString FileName=Files[Pos]->fileName() + ".qctools.xml.gz";

        Files[Pos]->Export_XmlGz(FileName, Prefs->ActiveFilters);
    }

    statusBar()->showMessage("All files exported to sidecar file");
}

//---------------------------------------------------------------------------
void MainWindow::on_actionExport_XmlGz_Custom_triggered()
{
    if (getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    // TODO
    // Temp
    QString Name=Files[getFilesCurrentPos()]->fileName();
    Name.replace(":", "");
    statusBar()->showMessage("(Not implemeted) Export to ~/.qctools"+Name+".qctools.xml.gz");
}

//---------------------------------------------------------------------------
void MainWindow::on_actionPrint_triggered()
{
    Export_PDF();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionFilesList_triggered()
{
    if (ui->actionGoTo)
        ui->actionGoTo->setVisible(false);
    if (ui->actionExport_XmlGz_Prompt)
        ui->actionExport_XmlGz_Prompt->setVisible(false);
    if (ui->actionExport_XmlGz_Sidecar)
        ui->actionExport_XmlGz_Sidecar->setVisible(false);
    if (ui->actionExport_XmlGz_Custom)
        ui->actionExport_XmlGz_Custom->setVisible(false);
    if (ui->actionPrint)
        ui->actionPrint->setVisible(false);
    if (ui->actionZoomIn)
        ui->actionZoomIn->setVisible(false);
    if (ui->actionZoomOne)
        ui->actionZoomOne->setVisible(false);
    if (ui->actionZoomOut)
        ui->actionZoomOut->setVisible(false);
    if (ui->actionWindowOut)
        ui->actionWindowOut->setVisible(false);
    for (size_t type = 0; type < Type_Max; type++)
        for (size_t group=0; group<CheckBoxes[type].size(); group++)
            CheckBoxes[type][group]->hide();
    m_commentsCheckbox->hide();

    if (ui->fileNamesBox)
        ui->fileNamesBox->hide();
    if (PlotsArea)
        PlotsArea->hide();
    if (TinyDisplayArea)
        TinyDisplayArea->hide();
    if (ControlArea)
        ControlArea->hide();
    if (FilesListArea && !Files.empty())
        FilesListArea->show();

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionGraphsLayout_triggered()
{
    if (ui->actionGoTo)
        ui->actionGoTo->setVisible(true);
    if (ui->actionExport_XmlGz_Prompt)
        ui->actionExport_XmlGz_Prompt->setVisible(true);
    if (ui->actionExport_XmlGz_Sidecar)
        ui->actionExport_XmlGz_Sidecar->setVisible(true);
    //if (ui->actionExport_XmlGz_Custom) // Not implemented action
    //    ui->actionExport_XmlGz_Custom->setVisible(true);
    if (ui->actionExport_XmlGz_Prompt)
        ui->actionExport_XmlGz_Prompt->setVisible(true);
    //if (ui->actionPrint)
    //    ui->actionPrint->setVisible(true);
    if (ui->actionZoomIn)
        ui->actionZoomIn->setVisible(true);
    if (ui->actionZoomOne)
        ui->actionZoomOne->setVisible(true);
    if (ui->actionZoomOut)
        ui->actionZoomOut->setVisible(true);
    if (ui->actionWindowOut)
        ui->actionWindowOut->setVisible(false);
    for (size_t type = 0; type < Type_Max; type++)
        for (size_t group=0; group<CheckBoxes[type].size(); group++)
            if (CheckBoxes[type][group] && getFilesCurrentPos()<Files.size() && Files[getFilesCurrentPos()]->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup])
                CheckBoxes[type][group]->show();

    if(getFilesCurrentPos()<Files.size())
        m_commentsCheckbox->show();

    if (ui->fileNamesBox)
        ui->fileNamesBox->show();
    if (PlotsArea)
        PlotsArea->show();
    if (TinyDisplayArea)
        TinyDisplayArea->show();
    if (ControlArea)
        ControlArea->show();
    if (FilesListArea)
        FilesListArea->hide();

    if (ui->fileNamesBox)
        ui->fileNamesBox->setCurrentIndex(getFilesCurrentPos());

    TimeOut();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    Options_Preferences();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionFiltersLayout_triggered()
{
    showPlayer();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionGettingStarted_triggered()
{
    Help_GettingStarted();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionHowToUseThisTool_triggered()
{
    Help_HowToUse();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionFilterDescriptions_triggered()
{
    Help_FilterDescriptions();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionPlaybackFilters_triggered()
{
    Help_PlaybackFilters();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionDataFormat_triggered()
{
    Help_DataFormat();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionAbout_triggered()
{
    Help_About();
}

//---------------------------------------------------------------------------
void MainWindow::on_fileNamesBox_currentIndexChanged(int index)
{
    setFilesCurrentPos(index);
    if (!ui->actionGraphsLayout->isChecked())
        return;
    createGraphsLayout();
    refreshDisplay();
    Update();
    QTimer::singleShot(0, this, SLOT(TimeOut_Refresh()));
}

//---------------------------------------------------------------------------
void MainWindow::on_check_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_M1_triggered()
{
    if (ControlArea)
        ControlArea->on_M1_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_Minus_triggered()
{
    if (ControlArea)
        ControlArea->on_Minus_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_PlayPause_triggered()
{
    if (ControlArea)
        ControlArea->on_PlayPause_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_Pause_triggered()
{
    if (ControlArea)
        ControlArea->on_PlayPause_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_Plus_triggered()
{
    if (ControlArea)
        ControlArea->on_Plus_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_P1_triggered()
{
    if (ControlArea)
        ControlArea->on_P1_clicked(true);
}

//---------------------------------------------------------------------------
void MainWindow::on_Full_triggered()
{
  if (isFullScreen())
     setWindowState(Qt::WindowActive);
  else
     setWindowState(Qt::WindowFullScreen);
}

//---------------------------------------------------------------------------
void MainWindow::on_CurrentFrameChanged()
{
    PlotsArea->onCurrentFrameChanged();
    updateScrollBar( true );
}

//---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

//---------------------------------------------------------------------------
void MainWindow::dropEvent(QDropEvent *Event)
{
    const QMimeData* Data=Event->mimeData ();
    if (Event->mimeData()->hasUrls())
    {
        //foreach (QUrl url, Event->mimeData()->urls())
        //clearFiles();
        QList<QUrl> urls=Event->mimeData()->urls();
        for (int Pos=0; Pos<urls.size(); Pos++)
        {
            addFile(urls[Pos].toLocalFile());
        }
    }

    clearDragDrop();
    addFile_finish();
}

void MainWindow::on_actionUploadToSignalServer_triggered()
{
    if(!Files.empty() && isFileSelected())
    {
        FileInformation* file = Files[getFilesCurrentPos()];
        if(file->signalServerUploadStatus() == FileInformation::Uploading)
        {
            file->cancelUpload();
        }
        else
        {
            QString statsFileName = file->fileName() + ".qctools.xml.gz";
            QFileInfo info(statsFileName);
            if(info.exists(statsFileName))
            {
                file->upload(info);
            }
            else
            {
                QMessageBox::warning(this, "Nothing to upload", QString("File %1 not found").arg(info.fileName()));
            }
        }
    }
}

void MainWindow::on_actionUploadToSignalServerAll_triggered()
{
    bool canCancel = false;
    Q_FOREACH(FileInformation* file, Files) {
        if(file->signalServerCheckUploadedStatus() == FileInformation::Uploading)
            canCancel = true;
    }

    Q_FOREACH(FileInformation* file, Files) {

        if(canCancel)
        {
            file->cancelUpload();
        } else
        {
            QString statsFileName = file->fileName() + ".qctools.xml.gz";
            QFileInfo info(statsFileName);
            if(info.exists(statsFileName))
            {
                file->upload(statsFileName);
            }
            else
            {
                QMessageBox::warning(this, "Nothing to upload", QString("File %1 not found").arg(info.fileName()));
            }
        }
    }
}

void MainWindow::onSignalServerConnectionChanged(SignalServerConnectionChecker::State state)
{
    qDebug() << "signalserver connection: " << state;
    updateConnectionIndicator();
}

void MainWindow::updateConnectionIndicator()
{
    if(connectionChecker->state() == SignalServerConnectionChecker::NotChecked)
    {
        connectionIndicator->setToolTip("signalserver status: not checked");
        connectionIndicator->setStyleSheet("background-color: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.621827, fy:0.359, stop:0 rgba(255, 255, 255, 255), stop:0.901015 rgba(155, 155, 155, 255), stop:1 rgba(255, 255, 255, 0));");
    } else if(connectionChecker->state() == SignalServerConnectionChecker::Online)
    {
        connectionIndicator->setToolTip("signalserver status: online");
        connectionIndicator->setStyleSheet("background-color: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.621827, fy:0.359, stop:0 rgba(0, 255, 0, 255), stop:0.901015 rgba(0, 155, 0, 255), stop:1 rgba(255, 255, 255, 0));");
    } else if(connectionChecker->state() == SignalServerConnectionChecker::Error || connectionChecker->state() == SignalServerConnectionChecker::Timeout)
    {
        connectionIndicator->setToolTip("signalserver status: error");
        connectionIndicator->setStyleSheet("background-color: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.621827, fy:0.359, stop:0 rgba(255, 0, 0, 255), stop:0.901015 rgba(155, 0, 0, 255), stop:1 rgba(255, 255, 255, 0));");
    }
}

void MainWindow::updateSignalServerSettings()
{
    if(Prefs->isSignalServerEnabled())
    {
        QString urlString = Prefs->signalServerUrlString();
        if(!urlString.startsWith("http", Qt::CaseInsensitive))
            urlString.prepend("http://");

        QUrl url(urlString);

        connectionChecker->start(url, Prefs->signalServerLogin(), Prefs->signalServerPassword());

        signalServer->setUrl(url);
        signalServer->setLogin(Prefs->signalServerLogin());
        signalServer->setPassword(Prefs->signalServerPassword());
        signalServer->setAutoUpload(Prefs->isSignalServerAutoUploadEnabled());

    } else {
        connectionChecker->stop();

        signalServer->setUrl(QUrl());
        signalServer->setLogin(QString());
        signalServer->setPassword(QString());
        signalServer->setAutoUpload(false);
    }

    uploadAction()->setVisible(Prefs->isSignalServerEnabled());
    uploadAllAction()->setVisible(Prefs->isSignalServerEnabled());
    ui->actionSignalServer_status->setVisible(Prefs->isSignalServerEnabled());
}

template <typename T> QString convertEnumToQString(const char* typeName, int value)
{
    const QMetaObject &mo = T::staticMetaObject;
    int index = mo.indexOfEnumerator(typeName);
    QMetaEnum metaEnum = mo.enumerator(index);
    return metaEnum.valueToKey(value);
}

QPixmap signalServerCheckUploadedStatusToPixmap(FileInformation::SignalServerCheckUploadedStatus signalServerCheckUploadedStatus)
{
    switch(signalServerCheckUploadedStatus)
    {
    case FileInformation::NotChecked:
        return QPixmap();
    case FileInformation::Checking:
        return QPixmap();
    case FileInformation::Uploaded:
        return QPixmap(":/icon/signalserver_success.png");
    case FileInformation::NotUploaded:
        return QPixmap(":/icon/signalserver_not_uploaded.png");
    case FileInformation::CheckError:
        return QPixmap(":/icon/signalserver_error.png");
    default:
        return QPixmap();
    }
}

void MainWindow::updateSignalServerCheckUploadedStatus()
{
    const QMetaObject &mo = FileInformation::staticMetaObject;
    int index = mo.indexOfEnumerator("SignalServerCheckUploadedStatus");
    QMetaEnum metaEnum = mo.enumerator(index);

    FileInformation* file = Files[getFilesCurrentPos()];
    FileInformation::SignalServerCheckUploadedStatus checkUploadedStatus = file->signalServerCheckUploadedStatus();
    QString key = convertEnumToQString<FileInformation>("SignalServerCheckUploadedStatus", checkUploadedStatus);
    ui->actionSignalServer_status->setText(QString("Signalserver: %1").arg(key));
    ui->actionSignalServer_status->setIcon(signalServerCheckUploadedStatusToPixmap(checkUploadedStatus));

    ui->actionSignalServer_status->setToolTip("");

    if(checkUploadedStatus == FileInformation::CheckError)
        ui->actionSignalServer_status->setToolTip(file->signalServerCheckUploadedStatusErrorString());
}

QPixmap signalServerUploadStatusToPixmap(FileInformation::SignalServerUploadStatus status)
{
    switch(status)
    {
    case FileInformation::Idle:
        return QPixmap(":/icon/signalserver_upload.png");
    case FileInformation::Uploading:
        return QPixmap(":/icon/signalserver_uploading.png");
    case FileInformation::Done:
        return QPixmap(":/icon/signalserver_success.png");
    case FileInformation::UploadError:
        return QPixmap(":/icon/signalserver_error.png");
    default:
        return QPixmap();
    }
}

void MainWindow::updateSignalServerUploadStatus()
{
    const QMetaObject &mo = FileInformation::staticMetaObject;
    int index = mo.indexOfEnumerator("SignalServerUploadStatus");
    QMetaEnum metaEnum = mo.enumerator(index);

    FileInformation* file = Files[getFilesCurrentPos()];
    FileInformation::SignalServerUploadStatus uploadStatus = file->signalServerUploadStatus();

    ui->actionSignalServer_status->setToolTip("");
    ui->actionSignalServer_status->setIcon(signalServerUploadStatusToPixmap(uploadStatus));

    if(uploadStatus == FileInformation::Idle || uploadStatus == FileInformation::Done || uploadStatus == FileInformation::UploadError)
    {
        if(uploadStatus == FileInformation::Done)
        {
            ui->actionSignalServer_status->setText("Uploaded");
        } else if(uploadStatus == FileInformation::UploadError)
        {
            ui->actionSignalServer_status->setText("Upload Error");
            ui->actionSignalServer_status->setToolTip(file->signalServerUploadStatusErrorString());
        }

        ui->actionUploadToSignalServer->setToolTip("Upload current stats to signalserver");
        ui->actionUploadToSignalServer->setText("Upload to Signalserver");
        ui->actionUploadToSignalServerAll->setText("Upload to Signalserver (All files)");

        ui->actionUploadToSignalServer->setIcon(signalServerUploadStatusToPixmap(FileInformation::Idle));
        ui->actionUploadToSignalServerAll->setIcon(signalServerUploadStatusToPixmap(FileInformation::Idle));
    }
    else if(uploadStatus == FileInformation::Uploading)
    {
        ui->actionSignalServer_status->setText("Uploading");
        ui->actionUploadToSignalServer->setToolTip("Cancel Upload to Signalserver");
        ui->actionUploadToSignalServer->setText("Cancel Upload to Signalserver");
        ui->actionUploadToSignalServerAll->setText("Cancel Upload to Signalserver (All files)");

        ui->actionUploadToSignalServer->setIcon(signalServerUploadStatusToPixmap(FileInformation::Uploading));
        ui->actionUploadToSignalServerAll->setIcon(signalServerUploadStatusToPixmap(FileInformation::Uploading));
    }
}

void MainWindow::updateSignalServerUploadProgress(qint64 value, qint64 total)
{
    ui->actionSignalServer_status->setText(QString("Uploading: %1 / %2").arg(value).arg(total));
}

void MainWindow::updateExportActions()
{
    if (getFilesCurrentPos() != (size_t)-1)
    {
        auto file = Files[getFilesCurrentPos()];
        bool parsed = file->parsed();
        bool hasStats = file->hasStats();
        bool exportEnabled = hasStats || parsed;

        ui->actionExport_Mkv_Prompt->setEnabled(exportEnabled);
        ui->actionExport_XmlGz_Custom->setEnabled(exportEnabled);
        ui->actionExport_XmlGz_Prompt->setEnabled(exportEnabled);
        ui->actionExport_XmlGz_Sidecar->setEnabled(exportEnabled);
    }
}

void MainWindow::updateExportAllAction()
{
    bool allParsedOrHaveStats = true;
    for(auto i = 0; i < Files.size(); ++i) {
        bool parsedOrHasStats = Files[i]->parsed() || Files[i]->hasStats();
        if(!parsedOrHasStats) {
            allParsedOrHaveStats = parsedOrHasStats;
            break;
        }
    }

    ui->actionExport_XmlGz_SidecarAll->setEnabled(allParsedOrHaveStats);
}

void MainWindow::showPlayer()
{
    auto selectedFile = Files[getFilesCurrentPos()];
    if(selectedFile != m_player->file()) {
        m_player->hide();
    }
    m_player->setFile(selectedFile);
    m_player->show();
}

void MainWindow::on_actionNavigateNextComment_triggered()
{
    if (getFilesCurrentPos()>=Files.size())
        return;

    auto framesCount = Files[getFilesCurrentPos()]->Glue->VideoFrameCount_Get();
    auto currentPos = Files[getFilesCurrentPos()]->Frames_Pos_Get();
    while(++currentPos < framesCount)
    {
        if(Files[getFilesCurrentPos()]->ReferenceStat()->comments[currentPos])
        {
            Files[getFilesCurrentPos()]->Frames_Pos_Set(currentPos);
            PlotsArea->onCurrentFrameChanged();
            break;
        }
    }
}

void MainWindow::on_actionNavigatePreviousComment_triggered()
{
    if (getFilesCurrentPos()>=Files.size())
        return;

    auto currentPos = Files[getFilesCurrentPos()]->Frames_Pos_Get();
    while(--currentPos >= 0)
    {
        if(Files[getFilesCurrentPos()]->ReferenceStat()->comments[currentPos])
        {
            Files[getFilesCurrentPos()]->Frames_Pos_Set(currentPos);
            PlotsArea->onCurrentFrameChanged();
            break;
        }
    }

}

void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
    {
        bool fileAlreadyOpened = false;
        for(auto i = 0; i < ui->fileNamesBox->count(); ++i)
        {
            if(ui->fileNamesBox->itemText(i) == action->text())
            {
                fileAlreadyOpened = true;
                break;
            }
        }

        if(!fileAlreadyOpened)
        {
            if(!QFile::exists(action->text()))
            {
                QMessageBox::warning(this, "Can't open file", QString("File %1 doesn't exist").arg(action->text()));
            } else {
                addFile(action->text());
                addFile_finish();
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for(size_t Pos = 0; Pos < Files.size(); Pos++)
    {
        if(!canCloseFile(Pos))
        {
            event->ignore();
            return;
        }
    }

    m_player->hide();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    if(Config::instance().getDebug())
    {
        static QLabel* label = new QLabel();
        ui->statusBar->insertPermanentWidget(0, label);

        label->setText(QString("MainWindow width = %1, height = %2").arg(width()).arg(height()));
    }
}

size_t MainWindow::getFilesCurrentPos() const
{
    return files_CurrentPos;
}

void MainWindow::setFilesCurrentPos(const size_t &value)
{
    bool fileWasSelected = isFileSelected();

    if(files_CurrentPos != value) {
        files_CurrentPos = value;

        if(fileWasSelected != isFileSelected())
            Q_EMIT(fileSelected(isFileSelected()));
    }

    ui->actionReveal_file_location->setEnabled(isFileSelected());
    ui->actionFiltersLayout->setEnabled(isFileSelected());
}

bool MainWindow::isFileSelected() const
{
    return files_CurrentPos != (size_t)-1;
}

void MainWindow::on_actionClear_Recent_History_triggered()
{
    for(auto action : recentFilesActions)
    {
        ui->menuFile->removeAction(action);
    }

    recentFilesActions.clear();
    preferences->setRecentFiles(QStringList());

    ui->actionClear_Recent_History->setEnabled(false);
}

void MainWindow::on_actionReveal_file_location_triggered()
{
    if(getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    QFileInfo fileInfo(Files[getFilesCurrentPos()]->fileName());
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteDir().path()));
}

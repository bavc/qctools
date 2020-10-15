/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "barchartprofilesmodel.h"
#include "mainwindow.h"
#include "managebarchartconditions.h"
#include "ui_mainwindow.h"

#include "GUI/preferences.h"
#include "GUI/Help.h"
#include "GUI/Plots.h"
#include "GUI/Comments.h"
#include "GUI/playercontrol.h"
#include "GUI/draggablechildrenbehaviour.h"
#include "Core/Core.h"
#include "Core/VideoCore.h"

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
#include <QShortcut>
#include <QToolButton>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QActionGroup>
#include <QPushButton>
#include <QSet>
#include <QJsonDocument>
#include <QStandardItemModel>
#include <QMessageBox>

#include <qwt_plot_renderer.h>
#include <QDebug>

#include "Core/SignalServer.h"
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************

const int MaxRecentFiles = 20;

//***************************************************************************
//
//***************************************************************************

void MainWindow::Ui_Init()
{
    ui->setupUi(this);

    // Shortcuts
    QShortcut *shortcutEqual = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal), this);
    QObject::connect(shortcutEqual, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));

    connect(ui->actionPlay_Pause, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->playPauseButton()->animateClick();
    }, Qt::UniqueConnection);

    auto* playAction = new QAction(this);
    playAction->setShortcuts({ QKeySequence(Qt::Key_Space), QKeySequence("K") });
    connect(playAction, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->playPauseButton()->animateClick();
    }, Qt::UniqueConnection);
    addAction(playAction);

    connect(ui->actionNext, &QAction::triggered, [this]() {
        this->PlotsArea->playerControl()->nextFrameButton()->animateClick();
    });

    auto* nextAction = new QAction(this);
    nextAction->setShortcuts({ QKeySequence(Qt::Key_Right) });
    connect(nextAction, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->nextFrameButton()->animateClick();
    }, Qt::UniqueConnection);
    addAction(nextAction);

    connect(ui->actionPrev, &QAction::triggered, [this]() {
        this->PlotsArea->playerControl()->prevFrameButton()->animateClick();
    });

    auto* prevAction = new QAction(this);
    prevAction->setShortcuts({ QKeySequence(Qt::Key_Left) });
    connect(prevAction, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->prevFrameButton()->animateClick();
    }, Qt::UniqueConnection);
    addAction(prevAction);

    connect(ui->actionGo_to_start, &QAction::triggered, [this]() {
        this->PlotsArea->playerControl()->goToStartButton()->animateClick();
    });

    auto* gotostartAction = new QAction(this);
    gotostartAction->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Left), QKeySequence(Qt::Key_Slash) });
    connect(gotostartAction, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->goToStartButton()->animateClick();
    }, Qt::UniqueConnection);
    addAction(gotostartAction);

    connect(ui->actionGo_to_end, &QAction::triggered, [this]() {
        this->PlotsArea->playerControl()->goToEndButton()->animateClick();
    });

    auto* gotoendAction = new QAction(this);
    gotoendAction->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Right), QKeySequence(Qt::Key_BracketRight) });
    connect(gotoendAction, &QAction::triggered, this, [this]() {
        this->PlotsArea->playerControl()->goToEndButton()->animateClick();
    }, Qt::UniqueConnection);
    addAction(gotoendAction);

    // Drag n drop
    setAcceptDrops(true);

    // Icons
    ui->actionOpen->setIcon(QIcon(":/icon/document-open.png"));
    ui->actionExport_XmlGz_Prompt->setIcon(QIcon(":/icon/export_xml.png"));
    ui->actionPrint->setIcon(QIcon(":/icon/document-print.png"));
    ui->actionZoomIn->setIcon(QIcon(":/icon/zoom-in.png"));
    ui->actionZoomOne->setIcon(QIcon(":/icon/zoom-one.png"));
    ui->actionZoomOut->setIcon(QIcon(":/icon/zoom-out.png"));
    ui->actionFilesList->setIcon(QIcon(":/icon/multifile_layout.png"));
    ui->actionGraphsLayout->setIcon(QIcon(":/icon/graph_layout.png"));
    ui->actionFiltersLayout->setIcon(QIcon(":/icon/filters_layout.png"));
    ui->actionGettingStarted->setIcon(QIcon(":/icon/help.png"));
    ui->actionWindowOut->setIcon(QIcon(":/icon/window-out.png"));
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->insertWidget(ui->actionFilesList, spacer);

    QLabel* boleanChartProfile = new QLabel("Select barchart profile: ");
    ui->toolBar->insertWidget(ui->actionFilesList, boleanChartProfile);

    m_profileSelectorCombobox = new QComboBox;
    connect(this, &MainWindow::fileSelected, m_profileSelectorCombobox, &QComboBox::setEnabled);
    connect(this, &MainWindow::fileSelected, this, [this](bool selected) {
        ui->actionGo_to_end->setEnabled(selected);
        ui->actionGo_to_start->setEnabled(selected);
        ui->actionNext->setEnabled(selected);
        ui->actionPrev->setEnabled(selected);
        ui->actionPlay_Pause->setEnabled(selected);

        ui->actionGrab_frame->setEnabled(selected);
        ui->actionGrab_plots_image->setEnabled(selected);

        ui->actionShow_hide_debug_panel->setEnabled(selected);
        ui->actionShow_hide_filters_panel->setEnabled(selected);

        ui->actionNavigateNextComment->setEnabled(selected);
        ui->actionNavigatePreviousComment->setEnabled(selected);
    });

    connect(this, &MainWindow::filePositionChanged, [this](size_t filePosition) {
        if (isFileSelected(filePosition)) {
            this->setWindowTitle(QString("QCTools - %1").arg(Files[filePosition]->fileName()));
        } else {
            this->setWindowTitle(QString("QCTools"));
        }
    });

    auto profilesModel = new BarchartProfilesModel(m_profileSelectorCombobox, QCoreApplication::applicationDirPath());

    m_profileSelectorCombobox->setModel(profilesModel);

    ui->toolBar->insertWidget(ui->actionFilesList, m_profileSelectorCombobox);

    QObject::connect(m_profileSelectorCombobox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), [this](int index) {
        auto value = m_profileSelectorCombobox->itemData(index).toString();
        loadBarchartsProfile(value);
    });

    if(m_profileSelectorCombobox->count() != 0) {
        loadBarchartsProfile(m_profileSelectorCombobox->itemData(m_profileSelectorCombobox->currentIndex()).toString());
    }

    QToolButton* manageBarchartProfiles = new QToolButton;
    manageBarchartProfiles->setIcon(QIcon(":/icon/settings.png"));

    connect(this, &MainWindow::fileSelected, manageBarchartProfiles, &QToolButton::setEnabled);
    connect(manageBarchartProfiles, &QToolButton::clicked, [this, profilesModel] {

        auto selectionIndex = m_profileSelectorCombobox->model()->index(m_profileSelectorCombobox->currentIndex(), 0);
        ManageBarchartConditions manageDialog(profilesModel, selectionIndex);
        connect(&manageDialog, &ManageBarchartConditions::newProfile, this, [&](const QString& profileFilePath) {
            auto currentProfileFilePath = m_profileSelectorCombobox->currentData(BarchartProfilesModel::Data).toString();

            Plots fakePlots(0, Files[getFilesCurrentPos()]);
            QJsonDocument profilesJson = QJsonDocument(fakePlots.saveBarchartsProfile());

            QFile file(profileFilePath);
            if(file.open(QFile::WriteOnly)) {
                qDebug() << "profile created: " << profileFilePath;
                file.write(profilesJson.toJson());
            } else {
                QMessageBox::warning(this, "Warning", QString("Failed to create profile %1").arg(profileFilePath));
            }

            if(currentProfileFilePath == profileFilePath) {
                m_barchartsProfile = profilesJson;
                applyBarchartsProfile();
            }
        });

        connect(&manageDialog, &ManageBarchartConditions::profileUpdated, this, [&](const QString& profileFilePath) {
            auto currentProfile = m_profileSelectorCombobox->currentData(BarchartProfilesModel::Data).toString();

            if(currentProfile == profileFilePath) {
                loadBarchartsProfile(profileFilePath);
            }
        });

        manageDialog.exec();
    });

    ui->toolBar->insertWidget(ui->actionFilesList, manageBarchartProfiles);

    // Config
    ui->verticalLayout->setSpacing(0);
    ui->verticalLayout->setMargin(0);
    ui->verticalLayout->setStretch(0, 1);
    ui->verticalLayout->setContentsMargins(0,0,0,0);

    // Window
    setWindowTitle("QCTools");
    setWindowIcon(QIcon(":/icon/logo.png"));

    //setUnifiedTitleAndToolBarOnMac(true); //Disabled because the toolbar dos not permit to move the window as expected by Mac users

    //ToolBar
    QObject::connect(ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(on_Toolbar_visibilityChanged(bool)));

    ui->horizontalLayout->setSpacing(0);

    //ToolTip
    if (ui->fileNamesBox)
        ui->fileNamesBox->hide();
    if (ui->copyToClipboard_pushButton)
        ui->copyToClipboard_pushButton->hide();
    if (ui->setupFilters_pushButton)
        ui->setupFilters_pushButton->hide();

    preferences = new Preferences(this);

    for (quint64 type = 0; type < Type_Max; type++)
    {
        for ( quint64 group = 0; group < PerStreamType[type].CountOfGroups; group++ ) // Group_Axis
        {
            auto name = PerStreamType[type].PerGroup[group].Name;
            auto description = PerStreamType[type].PerGroup[group].Description;
            auto selected = PerStreamType[type].PerGroup[group].CheckedByDefault;

            m_plotsChooser->add(name, type, group, description, selected);
        }
    }

    m_plotsChooser->add("Comments", Type_Comments, 0, "comments", true);

    for(auto panelInfo : preferences->availablePanels())
    {
        if(preferences->activePanels().contains(panelInfo.name))
        {
            m_plotsChooser->add(panelInfo.name, Type_Panels, qHash(panelInfo.name), panelInfo.name);
        }
    }

    auto selectedFilters = preferences->loadSelectedFilters();
    if(!selectedFilters.empty())
        m_plotsChooser->selectFilters(selectedFilters);

    configureZoom();

    //Groups
    QActionGroup* alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction(ui->actionFilesList);
    alignmentGroup->addAction(ui->actionGraphsLayout);
    alignmentGroup->addAction(ui->actionFiltersLayout);

    createDragDrop();
    ui->actionFilesList->setChecked(false);
    ui->actionGraphsLayout->setChecked(false);

    connectionIndicator = new QWidget;
    connectionIndicator->setToolTip("signalserver status: not checked");
    connectionIndicator->setMinimumSize(24, 24);
    ui->statusBar->addPermanentWidget(connectionIndicator);

    signalServer = new SignalServer(this);
    connectionChecker = new SignalServerConnectionChecker(this);
	connect(connectionChecker, SIGNAL(connectionStateChanged(SignalServerConnectionChecker::State)),
		this, SLOT(onSignalServerConnectionChanged(SignalServerConnectionChecker::State)));

    //Preferences
    Prefs=new PreferencesDialog(preferences, connectionChecker, this);
    connect(Prefs, SIGNAL(saved()), this, SLOT(updateSignalServerSettings()));
    connect(Prefs, &PreferencesDialog::saved, [&]() {

        if(!PlotsArea)
            return;

        auto existingPanelsSet = QSet<QString>();
        auto existingPanelsList = m_plotsChooser->getAvailableFilters([](quint64 type) -> bool {
            return type == Type_Panels;
        });

        for(auto & panelName : existingPanelsList)
        {
            existingPanelsSet.insert(panelName);
        }

        auto panelsToAdd= preferences->activePanels().subtract(existingPanelsSet);
        auto panelsToRemove = existingPanelsSet.subtract(preferences->activePanels());

        for(size_t panelIndex = 0; !panelsToRemove.empty() && panelIndex < PlotsArea->panelsCount(); ++panelIndex) {
            auto panel = PlotsArea->panelsView(panelIndex);
            if(panelsToRemove.contains(panel->panelTitle())) {
                panel->setVisible(false);
                panel->legend()->setVisible(false);

                m_plotsChooser->remove(panel->panelTitle());
            }
        }

        for(auto newPanel : panelsToAdd)
        {
            m_plotsChooser->add(newPanel, Type_Panels, qHash(newPanel), newPanel, true);
        }
    });

    updateSignalServerSettings();
    updateConnectionIndicator();

    //Temp
    ui->actionWindowOut->setVisible(false);
    ui->actionPrint->setVisible(false);

    // Not implemented action
    if (ui->actionExport_XmlGz_Custom)
        ui->actionExport_XmlGz_Custom->setVisible(false);

    QStringList recentFiles = preferences->recentFiles();

    int recentFilesIndex = recentFiles.length();
    while(recentFilesIndex-- > 0)
    {
        auto action = createOpenRecentAction(recentFiles[recentFilesIndex]);

        if(recentFilesActions.empty())
        {
            ui->menuFile->insertAction(ui->menuFile->insertSeparator(ui->actionClear_Recent_History), action);
        }
        else
        {
            ui->menuFile->insertAction(recentFilesActions.first(), action);
        }

        recentFilesActions.prepend(action);
    }

    if(recentFiles.length())
    {
        ui->actionClear_Recent_History->setEnabled(true);
    }
}

//---------------------------------------------------------------------------
bool MainWindow::isPlotZoomable() const
{
    return PlotsArea && PlotsArea->visibleFrames().count() > 4;
}

//---------------------------------------------------------------------------
void MainWindow::configureZoom()
{
    updateScrollBar();

    if (Files.empty() || PlotsArea==NULL || !PlotsArea->isZoomed() )
    {
        ui->actionZoomOut->setEnabled(false);
        ui->actionZoomOne->setEnabled(false);
        if (getFilesCurrentPos()<Files.size() && isPlotZoomable())
        {
            ui->actionZoomIn->setEnabled(true);
            ui->actionZoomOne->setEnabled(true);
        }
        else
        {
            ui->actionZoomIn->setEnabled(false);
        }

        ui->actionGoTo->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Prompt->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Sidecar->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Custom->setEnabled(!Files.empty());
        //ui->actionPrint->setEnabled(!Files.empty());
        return;
    }

    ui->actionZoomOne->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled( isPlotZoomable() );
    ui->actionGoTo->setEnabled(true);
    ui->actionExport_XmlGz_Prompt->setEnabled(true);
    ui->actionExport_XmlGz_Sidecar->setEnabled(true);
    ui->actionExport_XmlGz_Custom->setEnabled(true);
    //ui->actionPrint->setEnabled(true);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Move(size_t Begin)
{
    PlotsArea->Zoom_Move(Begin);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_In()
{
    Zoom( true );
    updateScrollBar();
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Out()
{
    Zoom( false );
    updateScrollBar();
}

void MainWindow::Zoom( bool on )
{
    PlotsArea->zoomXAxis( on ? Plots::ZoomIn : Plots::ZoomOut );
    configureZoom();
}

QAction *MainWindow::createOpenRecentAction(const QString &fileName)
{
    auto action = new QAction(fileName, this);
    connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));

    return action;
}

void MainWindow::updateRecentFiles(const QString &fileName)
{
    ui->actionClear_Recent_History->setEnabled(true);

    QStringList recentFiles = preferences->recentFiles();
    auto index = recentFiles.indexOf(fileName);

    if(index != 0)
    {
        if(index == -1)
        {
            auto action = createOpenRecentAction(fileName);

            if(recentFilesActions.empty())
            {
                ui->menuFile->insertAction(ui->menuFile->insertSeparator(ui->actionClear_Recent_History), action);
            }
            else
            {
                ui->menuFile->insertAction(recentFilesActions.first(), action);
            }

            recentFilesActions.prepend(action);
            recentFiles.prepend(fileName);

            if(recentFiles.size() > MaxRecentFiles)
            {
                recentFiles.takeLast();
                ui->menuFile->removeAction(recentFilesActions.takeLast());
            }
        }
        else
        {
            ui->menuFile->removeAction(recentFilesActions[index]);
            ui->menuFile->insertAction(recentFilesActions.first(), recentFilesActions[index]);

            recentFilesActions.move(index, 0);
            recentFiles.move(index, 0);
        }

        preferences->setRecentFiles(recentFiles);
    }
}

void MainWindow::updateScrollBar( bool blockSignals )
{
    QScrollBar* sb = ui->horizontalScrollBar;

    if ( PlotsArea==NULL || !PlotsArea->isZoomed() )
    {
        sb->hide();
    }
    else
    {
        const FrameInterval intv = PlotsArea->visibleFrames();

        sb->blockSignals( blockSignals );

        sb->setRange( 0, PlotsArea->numFrames() - intv.count() + 1 );
        sb->setValue( intv.from );
        sb->setPageStep( intv.count() );
        sb->setSingleStep( intv.count() );

        sb->blockSignals( false );

        sb->show();
    }
}

//---------------------------------------------------------------------------
void MainWindow::Export_PDF()
{
    if (getFilesCurrentPos()>=Files.size() || !Files[getFilesCurrentPos()])
        return;

    QString SaveFileName=QFileDialog::getSaveFileName(this, "Acrobat Reader file (PDF)", Files[getFilesCurrentPos()]->fileName() + ".qctools.pdf", "PDF (*.pdf)", 0, QFileDialog::DontUseNativeDialog);

    if (SaveFileName.isEmpty())
        return;

    /*QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(FileName);
    printer.setPageMargins(8,3,3,5,QPrinter::Millimeter);
    QPainter painter(&printer);  */

    /*
    QwtPlotRenderer PlotRenderer;
    PlotRenderer.renderDocument(const_cast<QwtPlot*>( PlotsArea->plot(TempType, Group_Y) ),
        SaveFileName, "PDF", QSizeF(210, 297), 150);
    QDesktopServices::openUrl(QUrl("file:///"+SaveFileName, QUrl::TolerantMode));
    */
}

//---------------------------------------------------------------------------
void MainWindow::Options_Preferences()
{
    Prefs->show();
}

//---------------------------------------------------------------------------
void MainWindow::Help_GettingStarted()
{
    Help* Frame=new Help(this);
    Frame->GettingStarted();
}

//---------------------------------------------------------------------------
void MainWindow::Help_HowToUse()
{
    Help* Frame=new Help(this);
    Frame->HowToUseThisTool();
}

//---------------------------------------------------------------------------
void MainWindow::Help_FilterDescriptions()
{
    Help* Frame=new Help(this);
    Frame->FilterDescriptions();
}

//---------------------------------------------------------------------------
void MainWindow::Help_PlaybackFilters()
{
    Help* Frame=new Help(this);
    Frame->PlaybackFilters();
}

//---------------------------------------------------------------------------
void MainWindow::Help_DataFormat()
{
    Help* Frame=new Help(this);
    Frame->DataFormat();
}

//---------------------------------------------------------------------------
void MainWindow::Help_About()
{
    Help* Frame=new Help(this);
    Frame->About();
}

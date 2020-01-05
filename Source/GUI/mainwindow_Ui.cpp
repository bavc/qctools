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

//---------------------------------------------------------------------------
void MainWindow::Ui_Init()
{
    ui->setupUi(this);

    // Shortcuts
    QShortcut *shortcutEqual = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal), this);
    QObject::connect(shortcutEqual, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));

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
    move(75, 75);
    resize(QApplication::desktop()->screenGeometry().width()-150, QApplication::desktop()->screenGeometry().height()-150);
    //setUnifiedTitleAndToolBarOnMac(true); //Disabled because the toolbar dos not permit to move the window as expected by Mac users

    //ToolBar
    QObject::connect(ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(on_Toolbar_visibilityChanged(bool)));

    ui->horizontalLayout->setSpacing(0);

    //ToolTip
    if (ui->fileNamesBox)
        ui->fileNamesBox->hide();

    preferences = new Preferences(this);

    QSet<QString> selectedFilters = QSet<QString>::fromList(preferences->loadSelectedFilters());

    for (size_t type = 0; type < Type_Max; type++)
        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ ) // Group_Axis
        {
            QPushButton* CheckBox=new QPushButton(PerStreamType[type].PerGroup[group].Name);

            QFontMetrics metrics(CheckBox->font());
            CheckBox->setMinimumWidth(metrics.width(CheckBox->text()));
            CheckBox->setCheckable(true);
            CheckBox->setFlat(true);
            CheckBox->setProperty("type", (quint64) type); // unfortunately QVariant doesn't support size_t
            CheckBox->setProperty("group", (quint64) group);
            CheckBox->setStyleSheet("\
                QPushButton {\
                    color: black;\
                    padding-top: 8px;\
                    padding-bottom: 8px;\
                    border: solid;\
                    border-color: lightgrey;\
                    border-width: 0 0 0 1px;\
                }\
                QPushButton:checked{\
                    background-color: grey;\
                }\
                QPushButton:hover{\
                    background-color: lightgrey;\
                }  \
                ");

            CheckBox->setToolTip(PerStreamType[type].PerGroup[group].Description);

            if(!selectedFilters.empty())
            {
                auto checkboxText = CheckBox->text();
                auto filterSelected = selectedFilters.contains(checkboxText);
                CheckBox->setChecked(filterSelected);
            } else
            {
                CheckBox->setChecked(PerStreamType[type].PerGroup[group].CheckedByDefault);
            }

            CheckBox->setVisible(false);
            QObject::connect(CheckBox, SIGNAL(toggled(bool)), this, SLOT(on_check_toggled(bool)));
            ui->horizontalLayout->addWidget(CheckBox);

            CheckBoxes[type].push_back(CheckBox);
        }

    m_commentsCheckbox=new QPushButton("Comments");
    QFontMetrics metrics(m_commentsCheckbox->font());
    m_commentsCheckbox->setMinimumWidth(metrics.width(m_commentsCheckbox->text()));

    m_commentsCheckbox->setProperty("type", (quint64) Type_Max);
    m_commentsCheckbox->setProperty("group", (quint64) 0);
    m_commentsCheckbox->setToolTip("comments");
    m_commentsCheckbox->setFlat(true);
    m_commentsCheckbox->setStyleSheet("\
        QPushButton {\
            color: black;\
            padding-top: 8px;\
            padding-bottom: 8px;\
            border: solid;\
            border-color: lightgrey;\
            border-width: 0 1px 0 0;\
        }\
        QPushButton:checked{\
            background-color: grey;\
        }\
        QPushButton:hover{\
            background-color: lightgrey;\
        }  \
        ");
    m_commentsCheckbox->setCheckable(true);
    m_commentsCheckbox->setChecked(true);
    m_commentsCheckbox->setVisible(false);
    QObject::connect(m_commentsCheckbox, SIGNAL(toggled(bool)), this, SLOT(on_check_toggled(bool)));
    ui->horizontalLayout->addWidget(m_commentsCheckbox);

    qDebug() << "checkboxes in layout: " << ui->horizontalLayout->count();

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

void MainWindow::changeFilterSelectorsOrder(QList<std::tuple<int, int> > filtersInfo)
{
    QSignalBlocker blocker(draggableBehaviour);

    auto boxlayout = static_cast<QHBoxLayout*> (ui->horizontalLayout);

    QList<QLayoutItem*> items;

    for(std::tuple<int, int> groupAndType : filtersInfo)
    {
        int group = std::get<0>(groupAndType);
        int type = std::get<1>(groupAndType);

        auto rowsCount = boxlayout->count();
        for(auto row = 0; row < rowsCount; ++row)
        {
            auto checkboxItem = boxlayout->itemAt(row);
            if(checkboxItem->widget()->property("group") == group && checkboxItem->widget()->property("type") == type)
            {
                items.append(boxlayout->takeAt(row));
                break;
            }
        }
    };

    for(auto item : items)
    {
        boxlayout->addItem(item);
    }
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
void MainWindow::refreshDisplay()
{
    if (PlotsArea)
    {
        PlotsArea->commentsPlot()->setVisible(m_commentsCheckbox->isChecked());
        PlotsArea->commentsPlot()->legend()->setVisible(m_commentsCheckbox->isChecked());

        for (size_t type = 0; type<Type_Max; type++)
            for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
                PlotsArea->setPlotVisible( type, group, CheckBoxes[type][group]->isChecked() );

        PlotsArea->alignYAxes();
    }
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

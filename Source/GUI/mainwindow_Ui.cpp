/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "GUI/preferences.h"
#include "GUI/Help.h"
#include "GUI/Plots.h"
#include "GUI/draggablechildrenbehaviour.h"
#include "Core/Core.h"
#include "Core/VideoCore.h"
#include "Core/BlackmagicDeckLink_Glue.h"

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

#include <qwt_plot_renderer.h>
#include <QDebug>
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************


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
    QShortcut *shortcutJ = new QShortcut(QKeySequence(Qt::Key_J), this);
    QObject::connect(shortcutJ, SIGNAL(activated()), this, SLOT(on_M1_triggered()));
    QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QObject::connect(shortcutLeft, SIGNAL(activated()), this, SLOT(on_Minus_triggered()));
    QShortcut *shortcutK = new QShortcut(QKeySequence(Qt::Key_K), this);
    QObject::connect(shortcutK, SIGNAL(activated()), this, SLOT(on_Pause_triggered()));
    QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QObject::connect(shortcutRight, SIGNAL(activated()), this, SLOT(on_Plus_triggered()));
    QShortcut *shortcutL = new QShortcut(QKeySequence(Qt::Key_L), this);
    QObject::connect(shortcutL, SIGNAL(activated()), this, SLOT(on_P1_triggered()));
    QShortcut *shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(shortcutSpace, SIGNAL(activated()), this, SLOT(on_PlayPause_triggered()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));

    // Drag n drop
    setAcceptDrops(true);

    // Icons
    ui->actionOpen->setIcon(QIcon(":/icon/document-open.png"));
    ui->actionBlackmagicDeckLinkCapture->setIcon(QIcon(":/icon/capture_layout.png"));
    ui->actionCSV->setIcon(QIcon(":/icon/export_csv.png"));
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

    //ToolTip
    if (ui->fileNamesBox)
        ui->fileNamesBox->hide();
    for (size_t type = 0; type < Type_Max; type++)
        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ ) // Group_Axis
        {
            QCheckBox* CheckBox=new QCheckBox(PerStreamType[type].PerGroup[group].Name);

            CheckBox->setProperty("type", (quint64) type); // unfortunately QVariant doesn't support size_t
            CheckBox->setProperty("group", (quint64) group);

            CheckBox->setToolTip(PerStreamType[type].PerGroup[group].Description);
            CheckBox->setCheckable(true);
            CheckBox->setChecked(PerStreamType[type].PerGroup[group].CheckedByDefault);
            CheckBox->setVisible(false);
            QObject::connect(CheckBox, SIGNAL(toggled(bool)), this, SLOT(on_check_toggled(bool)));
            ui->horizontalLayout->addWidget(CheckBox);

            CheckBoxes[type].push_back(CheckBox);
        }

    qDebug() << "checkboxes in layout: " << ui->horizontalLayout->count();

    configureZoom();

    //Groups
    QActionGroup* alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction(ui->actionFilesList);
    alignmentGroup->addAction(ui->actionGraphsLayout);
    alignmentGroup->addAction(ui->actionFiltersLayout);

    QActionGroup* playModesGroup = new QActionGroup(this);
    playModesGroup->addAction(ui->actionPlay_All_Frames);
    playModesGroup->addAction(ui->actionPlay_at_Frame_Rate);

    createDragDrop();
    ui->actionFilesList->setChecked(false);
    ui->actionGraphsLayout->setChecked(false);

    //Preferences
    Prefs=new Preferences(this);

    //Temp
    ui->actionWindowOut->setVisible(false);
    ui->actionPrint->setVisible(false);

    // Not implemented action
    if (ui->actionExport_XmlGz_Custom)
        ui->actionExport_XmlGz_Custom->setVisible(false);

    #if defined(BLACKMAGICDECKLINK_YES)
        // Deck menu
        if (BlackmagicDeckLink_Glue::CardsList().empty())
        {
            ui->actionBlackmagicDeckLinkCapture->setVisible(false);
            ui->menuBlackmagicDeckLink->menuAction()->setVisible(false);
        }
    #endif
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
        if (Files_CurrentPos<Files.size() && isPlotZoomable())
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
        ui->actionCSV->setEnabled(!Files.empty());
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
    ui->actionCSV->setEnabled(true);
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

void MainWindow::changeFilterSelectorsOrder(QList<QPair<int, int> > filtersInfo)
{
    QSignalBlocker blocker(draggableBehaviour);

    QHBoxLayout* boxlayout = static_cast<QHBoxLayout*> (ui->horizontalLayout);

    QList<QLayoutItem*> items;

    QList<QPair<int, int> >::iterator groupAndType;
    for(groupAndType = filtersInfo.begin(); groupAndType != filtersInfo.end(); ++groupAndType)
    {
        int group = groupAndType->first;
        int type = groupAndType->second;

        int rowsCount = boxlayout->count();
        for(int row = 0; row < rowsCount; ++row)
        {
            QLayoutItem* checkboxItem = boxlayout->itemAt(row);
            if(checkboxItem->widget()->property("group") == group && checkboxItem->widget()->property("type") == type)
            {
                items.append(boxlayout->takeAt(row));
                break;
            }
        }
    }

    QList<QLayoutItem*>::iterator item;
    for(item = items.begin(); item != items.end(); ++item)
    {
        boxlayout->addItem(*item);
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
void MainWindow::Export_CSV()
{
    if (Files_CurrentPos>=Files.size() || !Files[Files_CurrentPos])
        return;

    Files[Files_CurrentPos]->Export_CSV(QFileDialog::getSaveFileName(this, "Export to CSV", Files[Files_CurrentPos]->FileName+".qctools.csv", "Statistic files (*.csv)", 0, QFileDialog::DontUseNativeDialog));
}

//---------------------------------------------------------------------------
void MainWindow::Export_PDF()
{
    if (Files_CurrentPos>=Files.size() || !Files[Files_CurrentPos])
        return;

    QString SaveFileName=QFileDialog::getSaveFileName(this, "Acrobat Reader file (PDF)", Files[Files_CurrentPos]->FileName+".qctools.pdf", "PDF (*.pdf)", 0, QFileDialog::DontUseNativeDialog);

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
        for (size_t type = 0; type<Type_Max; type++)
            for (size_t group=0; group<PerStreamType[type].CountOfGroups; group++)
                PlotsArea->setPlotVisible( type, group, CheckBoxes[type][group]->checkState()==Qt::Checked );
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

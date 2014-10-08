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
#include "Core/Core.h"

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
    QShortcut *shortcutPlus = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Plus), this);
    QObject::connect(shortcutPlus, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutEqual = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal), this);
    QObject::connect(shortcutEqual, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutZoomOut = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus), this);
    QObject::connect(shortcutZoomOut, SIGNAL(activated()), this, SLOT(on_actionZoomOut_triggered()));
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
    QShortcut *shortcutG = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_G), this);
    QObject::connect(shortcutG, SIGNAL(activated()), this, SLOT(on_actionGoTo_triggered()));

    // Drag n drop
    setAcceptDrops(true);

    // Icons
    ui->actionOpen->setIcon(QIcon(":/icon/document-open.png"));
    ui->actionCSV->setIcon(QIcon(":/icon/export.png"));
    ui->actionPrint->setIcon(QIcon(":/icon/document-print.png"));
    ui->actionZoomIn->setIcon(QIcon(":/icon/zoom-in.png"));
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

    QLabel* DragDrop=new QLabel();
    DragDrop->setAlignment(Qt::AlignCenter);
    DragDrop->setPixmap(QPixmap(":/icon/window-out.png"));

    // Window
    setWindowTitle("QCTools");
    setWindowIcon(QIcon(":/icon/logo.png"));
    move(75, 75);
    resize(QApplication::desktop()->screenGeometry().width()-150, QApplication::desktop()->screenGeometry().height()-150);
    setUnifiedTitleAndToolBarOnMac(true);

    //ToolBar
    QObject::connect(ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(on_Toolbar_visibilityChanged(bool)));

    //ToolTip
    if (ui->fileNamesBox)
        ui->fileNamesBox->hide();
    for (size_t j=0; j<PlotType_Axis; j++)
    {
        CheckBoxes[j]=new QCheckBox(PerPlotGroup[j].Name);
        CheckBoxes[j]->setToolTip(PerPlotGroup[j].Description);
        CheckBoxes[j]->setCheckable(true);
        CheckBoxes[j]->setChecked(PerPlotGroup[j].CheckedByDefault);
        CheckBoxes[j]->setVisible(false);
        QObject::connect(CheckBoxes[j], SIGNAL(toggled(bool)), this, SLOT(on_check_toggled(bool)));
        ui->horizontalLayout->addWidget(CheckBoxes[j]);
    }

    configureZoom();

    //Groups
    QActionGroup* alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction(ui->actionFilesList);
    alignmentGroup->addAction(ui->actionGraphsLayout);
    alignmentGroup->addAction(ui->actionFiltersLayout);

    createDragDrop();
    ui->actionFilesList->setChecked(false);
    ui->actionGraphsLayout->setChecked(false);

    //Temp
    ui->actionFiltersLayout->setVisible(false);
    ui->actionWindowOut->setVisible(false);
    ui->actionPrint->setVisible(false);
    ui->actionPreferences->setVisible(false);
    ui->menuOptions->setVisible(false);
    ui->menuOptions->setTitle(QString());
    ui->menuOptions->setEnabled(false);

    // Not implemented action
    if (ui->actionExport_XmlGz_Custom)
        ui->actionExport_XmlGz_Custom->setVisible(false);
}

//---------------------------------------------------------------------------
void MainWindow::configureZoom()
{
    if (Files.empty() || PlotsArea==NULL || PlotsArea->ZoomScale==1)
    {
        ui->horizontalScrollBar->setRange(0, 0);
        ui->horizontalScrollBar->setMaximum(0);
        ui->horizontalScrollBar->setPageStep(1);
        ui->horizontalScrollBar->setSingleStep(1);
        ui->horizontalScrollBar->setEnabled(false);
        ui->actionZoomOut->setEnabled(false);
        ui->actionZoomIn->setEnabled(!Files.empty());
        ui->actionGoTo->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Prompt->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Sidecar->setEnabled(!Files.empty());
        ui->actionExport_XmlGz_Custom->setEnabled(!Files.empty());
        ui->actionCSV->setEnabled(!Files.empty());
        //ui->actionPrint->setEnabled(!Files.empty());
        return;
    }

    size_t Increment=Files[Files_CurrentPos]->Videos[0]->x_Current_Max/PlotsArea->ZoomScale;
    ui->horizontalScrollBar->setMaximum(Files[Files_CurrentPos]->Videos[0]->x_Current_Max-Increment);
    ui->horizontalScrollBar->setPageStep(Increment);
    ui->horizontalScrollBar->setSingleStep(Increment);
    ui->horizontalScrollBar->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
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

    ui->horizontalScrollBar->setValue(Begin);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_In()
{
    PlotsArea->ZoomScale*=2;
    configureZoom();
    size_t Position=Files[Files_CurrentPos]->Frames_Pos_Get();
    size_t Increment=Files[Files_CurrentPos]->Videos[0]->x_Current_Max/PlotsArea->ZoomScale;
    if (Position+Increment/2>Files[Files_CurrentPos]->Videos[0]->x_Current_Max)
        Position=Files[Files_CurrentPos]->Videos[0]->x_Current_Max-Increment/2;
    if (Position>Increment/2)
        Position-=Increment/2;
    else
        Position=0;
    Zoom_Move(Position);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Out()
{
    if (PlotsArea->ZoomScale>1)
        PlotsArea->ZoomScale/=2;
    configureZoom();
    size_t Position=Files[Files_CurrentPos]->Frames_Pos_Get();
    size_t Increment=Files[Files_CurrentPos]->Videos[0]->x_Current_Max/PlotsArea->ZoomScale;
    if (Position+Increment/2>Files[Files_CurrentPos]->Videos[0]->x_Current_Max)
        Position=Files[Files_CurrentPos]->Videos[0]->x_Current_Max-Increment/2;
    if (Position>Increment/2)
        Position-=Increment/2;
    else
        Position=0;
    Zoom_Move(Position);
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

    QwtPlotRenderer PlotRenderer;
    PlotRenderer.renderDocument(PlotsArea->plots[0], SaveFileName, "PDF", QSizeF(210, 297), 150);
    QDesktopServices::openUrl(QUrl("file:///"+SaveFileName, QUrl::TolerantMode));
}

//---------------------------------------------------------------------------
void MainWindow::refreshDisplay()
{
    if (PlotsArea)
    {
        for (size_t j=0; j<PlotType_Axis; j++)
            PlotsArea->Status[j]=CheckBoxes[j]->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_Axis]=true;
        PlotsArea->refreshDisplay();
    }


    refreshDisplay_Axis();
}

//---------------------------------------------------------------------------
void MainWindow::refreshDisplay_Axis()
{
    if (PlotsArea)
        PlotsArea->refreshDisplay_Axis();
}

//---------------------------------------------------------------------------
void MainWindow::Options_Preferences()
{
    Preferences* Frame=new Preferences(this);
    Frame->show();
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

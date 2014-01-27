/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    QShortcut *shortcutMinus = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus), this);
    QObject::connect(shortcutMinus, SIGNAL(activated()), this, SLOT(on_actionZoomOut_triggered()));

    // Drag n drop
    setAcceptDrops(true);

    // Icons
    ui->actionOpen->setIcon(QIcon(":/icon/document-open.png"));
    ui->actionZoomIn->setIcon(QIcon(":/icon/zoom-in.png"));
    ui->actionZoomOut->setIcon(QIcon(":/icon/zoom-out.png"));
    ui->actionPrint->setIcon(QIcon(":/icon/document-print.png"));
    ui->actionGettingStarted->setIcon(QIcon(":/icon/help.png"));

    // Config
    ui->verticalLayout->setSpacing(0);
    ui->verticalLayout->setMargin(0);
    ui->verticalLayout->setStretch(0, 1);
    ui->verticalLayout->setContentsMargins(0,0,0,0);

    // Window
    setWindowTitle("QCTools");
    setWindowIcon(QIcon(":/icon/logo.jpg"));
    move(75, 75);
    resize(QApplication::desktop()->screenGeometry().width()-150, QApplication::desktop()->screenGeometry().height()-150);

    //ToolBar
    QObject::connect(ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(on_Toolbar_visibilityChanged(bool)));

    //ToolTip
    ui->check_Y->setToolTip(StatsFile_Description[PlotType_Y]);
    ui->check_U->setToolTip(StatsFile_Description[PlotType_U]);
    ui->check_V->setToolTip(StatsFile_Description[PlotType_V]);
    ui->check_YDiff->setToolTip(StatsFile_Description[PlotType_YDiff]);
    ui->check_YDiffX->setToolTip(StatsFile_Description[PlotType_YDiffX]);
    ui->check_UDiff->setToolTip(StatsFile_Description[PlotType_UDiff]);
    ui->check_VDiff->setToolTip(StatsFile_Description[PlotType_VDiff]);
    ui->check_Diffs->setToolTip(StatsFile_Description[PlotType_Diffs]);
    ui->check_TOUT->setToolTip(StatsFile_Description[PlotType_TOUT]);
    ui->check_VREP->setToolTip(StatsFile_Description[PlotType_VREP]);
    ui->check_HEAD->setToolTip(StatsFile_Description[PlotType_HEAD]);
    ui->check_BRNG->setToolTip(StatsFile_Description[PlotType_BRNG]);

    configureZoom();
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
        ui->actionCSV->setEnabled(!Files.empty());
        ui->actionPrint->setEnabled(!Files.empty());
        return;
    }

    size_t Increment=Files[Files_Pos]->Glue->VideoFrameCount/PlotsArea->ZoomScale;
    ui->horizontalScrollBar->setMaximum(Files[Files_Pos]->Glue->VideoFrameCount-Increment);
    ui->horizontalScrollBar->setPageStep(Increment);
    ui->horizontalScrollBar->setSingleStep(Increment);
    ui->horizontalScrollBar->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
    ui->actionGoTo->setEnabled(true);
    ui->actionCSV->setEnabled(true);
    ui->actionPrint->setEnabled(true);
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
    size_t Position=Files[Files_Pos]->Glue->VideoFramePos;
    size_t Increment=Files[Files_Pos]->Glue->VideoFrameCount/PlotsArea->ZoomScale;
    if (Position+Increment/2>Files[Files_Pos]->Glue->VideoFrameCount)
        Position=Files[Files_Pos]->Glue->VideoFrameCount-Increment/2;
    if (Position>Increment/2)
        Position-=Increment/2;
    Zoom_Move(Position);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Out()
{
    if (PlotsArea->ZoomScale>1)
        PlotsArea->ZoomScale/=2;
    configureZoom();
    size_t Position=Files[Files_Pos]->Glue->VideoFramePos;
    size_t Increment=Files[Files_Pos]->Glue->VideoFrameCount/PlotsArea->ZoomScale;
    if (Position+Increment/2>Files[Files_Pos]->Glue->VideoFrameCount)
        Position=Files[Files_Pos]->Glue->VideoFrameCount-Increment/2;
    if (Position>Increment/2)
        Position-=Increment/2;
    Zoom_Move(Position);
}

//---------------------------------------------------------------------------
void MainWindow::Export_CSV()
{
    if (Files_Pos>=Files.size() || !Files[Files_Pos])
        return;
    
    Files[Files_Pos]->Export_CSV(QFileDialog::getSaveFileName(this, "Export to CSV", Files[Files_Pos]->FileName+".qctools.csv", "Statistic files (*.csv)", 0, QFileDialog::DontUseNativeDialog));
}

//---------------------------------------------------------------------------
void MainWindow::Export_PDF()
{
    if (Files_Pos>=Files.size() || !Files[Files_Pos])
        return;
    
    QString SaveFileName=QFileDialog::getSaveFileName(this, "Acrobat Reader file (PDF)", Files[Files_Pos]->FileName+".qctools.pdf", "PDF (*.pdf)", 0, QFileDialog::DontUseNativeDialog);

    if (SaveFileName.isEmpty())
        return;

    /*QPrinter printer(QPrinter::HighResolution);  
    printer.setOutputFormat(QPrinter::PdfFormat);  
    printer.setOutputFileName(FileName);  
    printer.setPageMargins(8,3,3,5,QPrinter::Millimeter);  
    QPainter painter(&printer);  */

    QwtPlotRenderer PlotRenderer;
    PlotRenderer.renderDocument(PlotsArea->plots[PlotType_Y], SaveFileName, "PDF", QSizeF(210, 297), 150);
    QDesktopServices::openUrl(QUrl("file:///"+SaveFileName, QUrl::TolerantMode));
}

//---------------------------------------------------------------------------
void MainWindow::refreshDisplay()
{
    if (PlotsArea)
    {
        PlotsArea->Status[PlotType_Y]=ui->check_Y->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_U]=ui->check_U->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_V]=ui->check_V->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_YDiff]=ui->check_YDiff->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_YDiffX]=ui->check_YDiffX->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_UDiff]=ui->check_UDiff->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_VDiff]=ui->check_VDiff->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_Diffs]=ui->check_Diffs->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_TOUT]=ui->check_TOUT->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_VREP]=ui->check_VREP->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_HEAD]=ui->check_HEAD->checkState()==Qt::Checked;
        PlotsArea->Status[PlotType_BRNG]=ui->check_BRNG->checkState()==Qt::Checked;
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
void MainWindow::Help_About()
{
    Help* Frame=new Help(this);
    Frame->About();
}

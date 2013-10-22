#include "mainwindow.h"
#include "ui_mainwindow.h"

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

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_widget.h>
#include <qwt_picker_machine.h>

#include "PerPicture.h"

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE
#include <GUI/Help.h>
#include <Core/Core.h>
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

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
    ui->actionHowToUse->setIcon(QIcon(":/icon/help.png"));

    // Window
    setWindowTitle("QC Tools");
    setWindowIcon(QIcon(":/icon/logo.jpg"));

    // First display
    FirstDisplay=new QLabel(this);
    FirstDisplay->setPixmap(QPixmap(":/icon/logo.jpg").scaled(geometry().width(), geometry().height()));
    ui->verticalLayout->addWidget(FirstDisplay);
}

//---------------------------------------------------------------------------
void MainWindow::configureZoom()
{
    if (Files.empty() || ZoomScale==1)
    {
        ui->horizontalScrollBar->setRange(0, 0);
        ui->horizontalScrollBar->setMaximum(0);
        ui->horizontalScrollBar->setPageStep(1);
        ui->horizontalScrollBar->setSingleStep(1);
        ui->horizontalScrollBar->setEnabled(false);
        ui->actionZoomOut->setEnabled(false);
        ui->actionZoomIn->setEnabled(!Files.empty());
        ui->actionCSV->setEnabled(!Files.empty());
        ui->actionPrint->setEnabled(!Files.empty());
        return;
    }

    size_t Increment=Frames_Total/ZoomScale;
    ui->horizontalScrollBar->setMaximum(Frames_Total-Increment);
    ui->horizontalScrollBar->setPageStep(Increment);
    ui->horizontalScrollBar->setSingleStep(Increment);
    ui->horizontalScrollBar->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
    ui->actionCSV->setEnabled(true);
    ui->actionPrint->setEnabled(true);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Move(size_t Begin)
{
    size_t Increment=Frames_Total/ZoomScale;
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            QwtPlotZoomer* zoomer = new QwtPlotZoomer(plots[Type]->canvas());
            QRectF Rect=zoomer->zoomBase();
            Rect.setLeft(Files[Files_Pos]->BasicInfo->Duration_Get()*Begin/Frames_Total);
            Rect.setWidth(Files[Files_Pos]->BasicInfo->Duration_Get()*Increment/Frames_Total);
            zoomer->zoom(Rect);
            if (Type<=PlotType_V)
                plots[Type]->setAxisScale(QwtPlot::yLeft, 0, 255, 85);
            plots[Type]->replot();
            delete zoomer;
        }
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_In()
{
    ZoomScale*=2;
    configureZoom();
    Zoom_Move(ui->horizontalScrollBar->value());
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Out()
{
    if (ZoomScale>1)
        ZoomScale/=2;
    configureZoom();
    Zoom_Move(ui->horizontalScrollBar->value());
}

//---------------------------------------------------------------------------
void MainWindow::Export_CSV()
{
    QString SaveFileName=QFileDialog::getSaveFileName(this, "Export to CSV", FileName+".qctools_development.csv", "Statistic files (*.csv)");

    Files[Files_Pos]->Export_CSV(SaveFileName);
}

//---------------------------------------------------------------------------
void MainWindow::Export_PDF()
{
   QString SaveFileName=QFileDialog::getSaveFileName(this, "Acrobat Reader file (PDF)", FileName+".qctools_development.pdf", "PDF (*.pdf)");

    if (SaveFileName.isEmpty())
        return;

    /*QPrinter printer(QPrinter::HighResolution);  
    printer.setOutputFormat(QPrinter::PdfFormat);  
    printer.setOutputFileName(FileName);  
    printer.setPageMargins(8,3,3,5,QPrinter::Millimeter);  
    QPainter painter(&printer);  */

    QwtPlotRenderer PlotRenderer;
    PlotRenderer.renderDocument(plots[PlotType_Y], SaveFileName, "PDF", QSizeF(210, 297), 150);
    QDesktopServices::openUrl(QUrl("file:///"+SaveFileName, QUrl::TolerantMode));
}

//---------------------------------------------------------------------------
void MainWindow::refreshDisplay()
{
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            plots[Type]->setVisible(false);
            ui->verticalLayout->removeWidget(plots[Type]);
        }
    if (Pictures_Widgets)
    {
        Pictures_Widgets->setVisible(false);
        ui->verticalLayout->removeWidget(Pictures_Widgets);
    }
    if (Control_Widgets)
    {
        Control_Widgets->setVisible(false);
        ui->verticalLayout->removeWidget(Control_Widgets);
    }

    //Per checkbox
    bool Status[PlotType_Max];
    Status[PlotType_Y]=ui->check_Y->checkState()==Qt::Checked;
    Status[PlotType_U]=ui->check_U->checkState()==Qt::Checked;
    Status[PlotType_V]=ui->check_V->checkState()==Qt::Checked;
    Status[PlotType_YDiff]=ui->check_YDiff->checkState()==Qt::Checked;
    Status[PlotType_YDiffX]=ui->check_YDiffX->checkState()==Qt::Checked;
    Status[PlotType_UDiff]=ui->check_UDiff->checkState()==Qt::Checked;
    Status[PlotType_VDiff]=ui->check_VDiff->checkState()==Qt::Checked;
    Status[PlotType_Diffs]=ui->check_Diffs->checkState()==Qt::Checked;
    Status[PlotType_TOUT]=ui->check_TOUT->checkState()==Qt::Checked;
    Status[PlotType_VREP]=ui->check_VREP->checkState()==Qt::Checked;
    Status[PlotType_HEAD]=ui->check_HEAD->checkState()==Qt::Checked;
    Status[PlotType_RANG]=ui->check_RANG->checkState()==Qt::Checked;
    PlotType Latest=PlotType_Max;
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (Status[Type])
        {
            if (plots[Type])
            {
                createData_Update((PlotType)Type);
                plots[Type]->setVisible(true);
                ui->verticalLayout->addWidget(plots[Type]);
            }
            Latest=(PlotType)Type;
        }
    if (Pictures_Widgets)
    {
        Pictures_Widgets->setVisible(true);
        ui->verticalLayout->addWidget(Pictures_Widgets);
    }
    if (Control_Widgets)
    {
        Control_Widgets->setVisible(true);
        ui->verticalLayout->addWidget(Control_Widgets);
    }

    //RePlot
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
            plots[Type]->enableAxis(QwtPlot::xBottom, Latest==Type);
    
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
            plots[Type]->replot();
}

//---------------------------------------------------------------------------
void MainWindow::Help_HowToUse()
{
    Help* Frame=new Help(this);
    Frame->show_HowToUse();
}

//---------------------------------------------------------------------------
void MainWindow::Help_FilterDescriptions()
{
    Help* Frame=new Help(this);
    Frame->show_FilterDescriptions();
}

//---------------------------------------------------------------------------
void MainWindow::Help_License()
{
    Help* Frame=new Help(this);
    Frame->show_License();
}

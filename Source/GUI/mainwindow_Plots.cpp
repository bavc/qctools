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
void MainWindow::Plots_Init()
{
    memset(plots, 0, sizeof(QwtPlot*)*PlotType_Max);
    memset(curves, 0, sizeof(QwtPlotCurve*)*5*PlotType_Max);
    memset(plotsZoomers, 0, sizeof(QwtPlotZoomer*)*PlotType_Max);
    ZoomScale=1;
    Frames_Total=0;
    Frames_Pos=0;
    memset(plots_YMax, 0, sizeof(double)*PlotType_Max);
}

//---------------------------------------------------------------------------
void MainWindow::Plots_Create()
{
    //Creating data
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type]==NULL)
            Plots_Create((PlotType)Type);
}

//---------------------------------------------------------------------------
void MainWindow::Plots_Create(PlotType Type)
{
    QwtPlot* plot = new QwtPlot;

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin( true );
    grid->enableYMin( true );
    grid->setMajorPen( Qt::darkGray, 0, Qt::DotLine );
    grid->setMinorPen( Qt::gray, 0 , Qt::DotLine );
    grid->attach( plot );

    plot->setAxisScale(QwtPlot::xBottom, 0, Files[Files_Pos]->BasicInfo->Duration_Get());
    plot->setAxisMaxMajor(QwtPlot::yLeft, 2);
    plot->setAxisMaxMinor(QwtPlot::yLeft, 1);

    for(unsigned j=0; j<StatsFile_Counts[Type]; ++j)
    {
        curves[Type][j] = new QwtPlotCurve(Names[StatsFile_Positions[Type]+j]);
        QColor c;
        
        switch (StatsFile_Counts[Type])
        {
             case 1 :
                        c=Qt::black;
                        break;
             case 3 : 
                        switch (j)
                        {
                            case 0: c=Qt::black; break;
                            case 1: c=Qt::darkGray; break;
                            case 2: c=Qt::darkGray; break;
                            default: c=Qt::black;
                        }
                        break;
             case 5 : 
                        switch (j)
                        {
                            case 0: c=Qt::red; break;
                            case 1: c=QColor::fromRgb(0x00, 0x66, 0x00); break; //Qt::green
                            case 2: c=Qt::black; break;
                            case 3: c=Qt::green; break;
                            case 4: c=Qt::red; break;
                            default: c=Qt::black;
                        }
                        break;
            default:    c=Qt::black;
        }

        curves[Type][j]->setPen(c);
        /*
        if (Count==5 && (j==2 || j==3))
            curve->setBrush(Qt::lightGray);
        if (Count==5 && (j==1 || j==4))
            curve->setBrush(Qt::darkGray);
        if (Count==5 && (j==0))
            curve->setBrush(Qt::white);
        */

        curves[Type][j]->setRenderHint(QwtPlotItem::RenderAntialiased);
        curves[Type][j]->setZ(curves[Type][j]->z()-j);
        curves[Type][j]->attach(plot);
     }

    //adding the legend
    QwtLegend *legend = new QwtLegend;
    //legend->setDefaultItemMode(QwtLegendData::Checkable);
    plot->insertLegend(legend, QwtPlot::RightLegend);

    //same axis
    QwtScaleWidget *scaleWidget = plot->axisWidget(QwtPlot::yLeft);
    scaleWidget->scaleDraw()->setMinimumExtent(40);

    //same legend
    QwtPlotLayout *plotLayout=plot->plotLayout();
    QRectF canvasRect=plotLayout->canvasRect();
    QRectF legendRect=plotLayout->legendRect();
    //plotLayout->setLegendPosition(legendRect);
    QwtLegend* A=(QwtLegend*)plot->legend();
    QWidget* W=new QWidget();
    W->setMinimumWidth(100);
    A->contentsWidget()->layout()->addWidget(W);
    
    // Show the plots

    plots[Type]=plot;

    if (StatsFile_Counts[Type]>3)
        plots[Type]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //if (Type==0)
    {
        plotsPicker[Type] = new QwtPlotPicker( QwtPlot::xBottom, QwtPlot::yLeft, QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, plots[Type]->canvas() );
        plotsPicker[Type]->setStateMachine( new QwtPickerDragPointMachine () );
        plotsPicker[Type]->setRubberBandPen( QColor( Qt::green ) );
        plotsPicker[Type]->setTrackerPen( QColor( Qt::white ) );
        connect(plotsPicker[Type], SIGNAL(moved(const QPoint&)), SLOT(plot_moved(const QPoint&)));
        connect(plotsPicker[Type], SIGNAL(mousePressEvent(QMouseEvent*)), SLOT(plot_mousePressEvent(QMouseEvent*)));
    }

}

//---------------------------------------------------------------------------
void MainWindow::createData_Init()
{
    statusBar()->showMessage("Initializing...");

    if (FirstDisplay)
    {
        ui->verticalLayout->removeWidget(FirstDisplay);
        delete FirstDisplay; FirstDisplay=NULL;
    }

    // Create
    Plots_Create();
    Pictures_Create();
    createData_Update();

   //UI
    ui->fileNamesBox->clear();
    ui->fileNamesBox->addItem(FileName);
    configureZoom();
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::createData_Update()
{    
    //Creating data
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type] && plots[Type]->isVisible())
            createData_Update((PlotType)Type);
}

//---------------------------------------------------------------------------
void MainWindow::createData_Update(PlotType Type)
{
    double y_Max_ForThisPlot=Files[Files_Pos]->Stats->y_Max[Type];
    
    //plot->setMinimumHeight(0);
    //plot->enableAxis(QwtPlot::yLeft, false);

    if (y_Max_ForThisPlot)
    {
        double StepCount=3;
        if (Type==PlotType_Diffs)
            StepCount=2;

        if (y_Max_ForThisPlot>plots_YMax[Type])
        {
            double Step=floor(y_Max_ForThisPlot/StepCount);
            if (Step==0)
            {
                Step=floor(y_Max_ForThisPlot/StepCount*10)/10;
                if (Step==0)
                {
                    Step=floor(y_Max_ForThisPlot/StepCount*100)/100;
                    if (Step==0)
                    {
                        Step=floor(y_Max_ForThisPlot/StepCount*1000)/1000;
                        if (Step==0)
                        {
                            Step=floor(y_Max_ForThisPlot/StepCount*10000)/10000;
                            if (Step==0)
                            {
                                Step=floor(y_Max_ForThisPlot/StepCount*100000)/100000;
                                if (Step==0)
                                {
                                    Step=floor(y_Max_ForThisPlot/StepCount*1000000)/1000000;
                                }
                            }
                        }
                    }
                }
            }
            if (Step)
            {
                plots_YMax[Type]=Files[Files_Pos]->Stats->y_Max[Type];
                plots[Type]->setAxisScale(QwtPlot::yLeft, 0, plots_YMax[Type], Step);
            }
        }
    }

    for(unsigned j=0; j<StatsFile_Counts[Type]; ++j)
    {
        curves[Type][j]->setRawSamples(Files[Files_Pos]->Stats->x, Files[Files_Pos]->Stats->y[StatsFile_Positions[Type]+j], Files[Files_Pos]->Stats->Frames_Current);
        plots[Type]->replot();
    }
}



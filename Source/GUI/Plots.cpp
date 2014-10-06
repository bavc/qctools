/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "GUI/Plots.h"
#include "Core/Core.h"

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
#include <qwt_plot_marker.h>
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************


//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
Plots::Plots(QWidget *parent, FileInformation* FileInformationData_) :
    QWidget(parent),
    FileInfoData(FileInformationData_)
{
    // To update
    TinyDisplayArea=NULL;
    ControlArea=NULL;
    InfoArea=NULL;

    // Positioning info
    ZoomScale=1;

    // Status
    memset(Status, true, sizeof(bool)*PlotType_Max);

    // Layouts and Widgets
    Layout=NULL;
    memset(Layouts      , 0, sizeof(QHBoxLayout*  )*PlotType_Max);
    memset(paddings     , 0, sizeof(QWidget*      )*PlotType_Max);
    memset(plots        , 0, sizeof(QwtPlot*      )*PlotType_Max);
    memset(legends      , 0, sizeof(QwtLegend*    )*PlotType_Max);

    // Widgets addons
    memset(plotsCurves  , 0, sizeof(QwtPlotCurve* )*PlotType_Max*5);
    memset(plotsZoomers , 0, sizeof(QwtPlotZoomer*)*PlotType_Max);
    memset(plotsPickers , 0, sizeof(QwtPlotPicker*)*PlotType_Max);
    memset(plotsMarkers , 0, sizeof(QwtPlotMarker*)*PlotType_Max);

    // X axis info
    XAxis_Kind=NULL;
    XAxis_Kind_index=1;
    Zoom_Left=0;
    Zoom_Width=0;
    Marker_FramePos=(size_t)-1;
    Data_FramePos_Max=0;
    Data_FramePos_Current=0;

    // Y axis info
    memset(plots_YMax, 0, sizeof(double)*PlotType_Max);

    // Plots
    Plots_Create();
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
    for (size_t Type=0; Type<PlotType_Max; Type++)
    {
        // Widgets addons
        for(unsigned j=0; j<5; ++j)
            delete plotsCurves[Type][j];
        delete plotsZoomers [Type];
        delete plotsPickers [Type];
        delete plotsMarkers [Type];

        // Layouts and Widgets
        delete Layouts      [Type];
        delete paddings     [Type];
        delete plots        [Type];
        delete legends      [Type];
    }
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void Plots::Plots_Create()
{
    //Creating data
    for (size_t Type=0; Type<PlotType_Max; Type++)
        Plots_Create((PlotType)Type);

    // XAxis_Kind
    XAxis_Kind=new QComboBox(this);
    XAxis_Kind->setVisible(false);
    XAxis_Kind->addItem("Frames");
    XAxis_Kind->addItem("Seconds");
    XAxis_Kind->addItem("Minutes");
    XAxis_Kind->addItem("Hours");
    XAxis_Kind->setCurrentIndex(XAxis_Kind_index);
    //XAxis_Kind->setEnabled(false);
    connect(XAxis_Kind, SIGNAL(currentIndexChanged(int)), this, SLOT(on_XAxis_Kind_currentIndexChanged(int)));
}

//---------------------------------------------------------------------------
void Plots::Plots_Create(PlotType Type)
{
    // Paddings
    if (paddings[Type]==NULL)
    {
        paddings[Type]=new QWidget(this);
        paddings[Type]->setVisible(false);
    }

    // General design of plot
    QwtPlot* plot = new QwtPlot(this);
    plot->setVisible(false);
    plot->setMinimumHeight(1);
    plot->enableAxis(QwtPlot::xBottom, Type==PlotType_Axis);
    plot->setAxisMaxMajor(QwtPlot::yLeft, 0);
    plot->setAxisMaxMinor(QwtPlot::yLeft, 0);
    plot->setAxisScale(QwtPlot::xBottom, 0, FileInfoData->Videos[0]->x_Max[XAxis_Kind_index]);
    if (PerPlotGroup[Type].Count>3)
        plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Plot grid
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(Qt::darkGray, 0, Qt::DotLine );
    grid->setMinorPen(Qt::gray, 0 , Qt::DotLine );
    grid->attach(plot);

    // Plot curves
    for(unsigned j=0; j<PerPlotGroup[Type].Count; ++j)
    {
        plotsCurves[Type][j] = new QwtPlotCurve(PerPlotName[PerPlotGroup[Type].Start+j].Name);
        QColor c;

        switch (PerPlotGroup[Type].Count)
        {
             case 1 :
                        switch (Type)
                        {
                            case PlotType_YDiff: c=Qt::darkGreen; break;
                            case PlotType_UDiff: c=Qt::darkBlue; break;
                            case PlotType_VDiff: c=Qt::darkRed; break;
                            default: c=Qt::black;
                        }
                        break;
             case 2 :
                        switch (j)
                        {
                            case 0: c=Qt::darkGreen; break;
                            case 1: c=Qt::darkRed; break;
                            default: c=Qt::black;
                        }
                        break;
             case 3 :
                        switch (j)
                        {
                            case 0: c=Qt::darkGreen; break;
                            case 1: c=Qt::darkBlue; break;
                            case 2: c=Qt::darkRed; break;
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

        plotsCurves[Type][j]->setPen(c);
        plotsCurves[Type][j]->setRenderHint(QwtPlotItem::RenderAntialiased);
        switch (Type)
        {
            case PlotType_Diffs   :
            case PlotType_MSE     :
            case PlotType_PSNR    :
                                    break;  //Invert data order (except for Diffs, MSE, PSNR...)
            default:
                                    plotsCurves[Type][j]->setZ(plotsCurves[Type][j]->z()-j);
        }
        plotsCurves[Type][j]->attach(plot);
     }

    // Legends
    QwtLegend *legend = new QwtLegend(this);
    legend->setVisible(false);
    legend->setMinimumHeight(1);
    legend->setMaxColumns(1);
    QFont Font=legend->font();
    #ifdef _WIN32
        Font.setPointSize(6);
    #else // _WIN32
        Font.setPointSize(8);
    #endif //_WIN32
    legend->setFont(Font);
    connect(plot, SIGNAL(legendDataChanged(const QVariant &, const QList<QwtLegendData> &)), legend, SLOT(updateLegend(const QVariant &, const QList<QwtLegendData> &)));
    plot->updateLegend();

    // Assignment
    plots[Type]=plot;
    legends[Type]=legend;

    // Pickers
    plotsPickers[Type] = new QwtPlotPicker( QwtPlot::xBottom, QwtPlot::yLeft, QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, plots[Type]->canvas() );
    plotsPickers[Type]->setStateMachine( new QwtPickerDragPointMachine () );
    plotsPickers[Type]->setRubberBandPen( QColor( Qt::green ) );
    plotsPickers[Type]->setTrackerPen( QColor( Qt::white ) );
    connect(plotsPickers[Type], SIGNAL(moved(const QPointF&)), SLOT(plot_moved(const QPointF&)));
    connect(plotsPickers[Type], SIGNAL(selected(const QPointF&)), SLOT(plot_moved(const QPointF&)));

    // Marker
    QwtPlotMarker* plotMarker=new QwtPlotMarker;
    plotMarker->setLineStyle(QwtPlotMarker::VLine);
    plotMarker->setLinePen(QPen(Qt::magenta, 1));
    plotMarker->setXValue(0);
    plotMarker->attach(plot);
    plotsMarkers[Type]=plotMarker;
}

//---------------------------------------------------------------------------
void Plots::Plots_Update()
{
    size_t FramePos=FileInfoData->Frames_Pos_Get();
    double X=FileInfoData->Videos[0]->x[XAxis_Kind_index][FramePos];

    // Put the current frame in center
    if (ZoomScale!=1)
    {
        size_t Increment=Data_FramePos_Max/ZoomScale;
        size_t NewBegin=0;
        if (FramePos>Increment/2)
        {
            NewBegin=FramePos-Increment/2;
            if (NewBegin+Increment>Data_FramePos_Max)
                NewBegin=Data_FramePos_Max-Increment;
        }
        Zoom_Move(NewBegin);
    }

    Marker_Update(X);
}

//---------------------------------------------------------------------------
void Plots::Marker_Update()
{
    if (Marker_FramePos==FileInfoData->Frames_Pos_Get())
        return;
        
    Marker_FramePos=FileInfoData->Frames_Pos_Get();
    Marker_Update(FileInfoData->Videos[0]->x[XAxis_Kind_index][Marker_FramePos]);
}

//---------------------------------------------------------------------------
void Plots::Marker_Update(double X)
{
    for (size_t Type=0; Type<PlotType_Max; ++Type)
    {
        plotsMarkers[Type]->setXValue(X);
        plots[Type]->replot();
    }
}

//---------------------------------------------------------------------------
void Plots::createData_Init()
{
    // Create
    createData_Update();
    refreshDisplay();
}

//---------------------------------------------------------------------------
void Plots::createData_Update()
{
    //Creating data
    for (size_t Type=0; Type<PlotType_Max; Type++)
        if (plots[Type] && plots[Type]->isVisible())
            createData_Update((PlotType)Type);
    
    //Update of zoom in case of total duration change
    if (Data_FramePos_Max+1!=FileInfoData->Videos[0]->x_Current_Max)
    {
        Data_FramePos_Max=FileInfoData->Videos[0]->x_Current_Max-1;
        Zoom_Update();
    }

    //Update of zoom in case of total duration change
    //if (Data_FramePos_Current!=FileInfoData->Videos[0]->x_Current)
    //{
    //    Data_FramePos_Current=FileInfoData->Videos[0]->x_Current;
    //    plot_moved(Marker_RealPoint);
    //}
}

//---------------------------------------------------------------------------
void Plots::createData_Update(PlotType Type)
{
    if (PerPlotGroup[Type].Min!=PerPlotGroup[Type].Max && FileInfoData->Videos[0]->y_Max[Type]>=PerPlotGroup[Type].Max/2)
        FileInfoData->Videos[0]->y_Max[Type]=PerPlotGroup[Type].Max;
    double y_Max_ForThisPlot=FileInfoData->Videos[0]->y_Max[Type];

    //plot->setMinimumHeight(0);

    if (y_Max_ForThisPlot)
    {
        double StepCount=PerPlotGroup[Type].StepsCount;

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

            if (FileInfoData->Videos[0]->y_Max[Type]==0)
            {
                FileInfoData->Videos[0]->y_Max[Type]=1; //Special case, in order to force a scale instead of -1 to 1
                Step=1;
            };

            if (Step)
            {
                plots_YMax[Type]=FileInfoData->Videos[0]->y_Max[Type];
                plots[Type]->setAxisScale(QwtPlot::yLeft, 0, plots_YMax[Type], Step);
            }
        }
    }
    else
    {
        plots[Type]->setAxisScale(QwtPlot::yLeft, 0, 1, 1); //Special case, in order to force a scale instead of -1 to 1
    }

    for(unsigned j=0; j<PerPlotGroup[Type].Count; ++j)
        plotsCurves[Type][j]->setRawSamples(FileInfoData->Videos[0]->x[XAxis_Kind_index], FileInfoData->Videos[0]->y[PerPlotGroup[Type].Start+j], FileInfoData->Videos[0]->x_Current);
    plots[Type]->replot();
}

//---------------------------------------------------------------------------
void Plots::Zoom_Update()
{
    size_t Increment=Data_FramePos_Max/ZoomScale;
    int Pos=FileInfoData->Frames_Pos_Get();
    if (Pos==(int)-1)
        return;    
    if (Pos>Increment/2)
        Pos-=Increment/2;
    else
        Pos=0;
    Zoom_Move(Pos);
}

//---------------------------------------------------------------------------
void Plots::Zoom_Move(size_t Begin)
{
    size_t Increment=Data_FramePos_Max/ZoomScale;
    if (Begin+Increment>Data_FramePos_Max)
        Begin=Data_FramePos_Max-Increment;

    Zoom_Left=FileInfoData->Videos[0]->x[XAxis_Kind_index][Begin];
    Zoom_Width=FileInfoData->Videos[0]->x_Max[XAxis_Kind_index]/ZoomScale;

    for (size_t Type=0; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            QwtPlotZoomer* zoomer = new QwtPlotZoomer(plots[Type]->canvas());
            QRectF Rect=zoomer->zoomBase();
            Rect.setLeft(Zoom_Left);
            Rect.setWidth(Zoom_Width);
            zoomer->zoom(Rect);
            delete zoomer;
        }
}

//---------------------------------------------------------------------------
void Plots::refreshDisplay()
{
    if (Layout==NULL)
    {
        Layout=new QVBoxLayout(this);
        Layout->setSpacing(0);
        Layout->setMargin(0);
        Layout->setContentsMargins(0,0,0,0);
    }

    int Pos=0;
    for (size_t Type=0; Type<PlotType_Max; Type++)
        if (Status[Type])
        {
            if (Layouts[Type]==NULL)
            {
                if (Layouts[Type]==NULL)
                    Layouts[Type]=new QHBoxLayout();
                Layouts[Type]->setSpacing(0);
                Layouts[Type]->setMargin(0);
                Layouts[Type]->setContentsMargins(0,0,0,0);

                Layouts[Type]->addWidget(paddings[Type]);
                //paddings[Type]->setStyleSheet("background-color:blue;"); // For GUI debug

                Layouts[Type]->addWidget(plots[Type]);
                Layouts[Type]->setStretchFactor(plots[Type], 1);
                //plots[Type]->setStyleSheet("background-color:green;"); // For GUI debug

                createData_Update((PlotType)Type);
                if (Type!=PlotType_Axis)
                {
                    legends[Type]->setContentsMargins(0, 0, 0, 0);
                    Layouts[Type]->addWidget(legends[Type]);
                    //legends[Type]->setStyleSheet("background-color:blue;"); // For GUI debug
                }
                else
                {
                    plots[PlotType_Axis]->setMaximumHeight(plots[PlotType_Axis]->axisWidget(QwtPlot::xBottom)->height());

                    XAxis_Kind->setContentsMargins(0, 0, 0, 0);
                    Layouts[Type]->addWidget(XAxis_Kind);
                    //XAxis_Kind->setStyleSheet("background-color:blue;"); // For GUI debug
                }

                Layout->insertLayout(Pos, Layouts[Type]);
                if (Type!=PlotType_Axis)
                    Layout->setStretchFactor(Layouts[Type], 1);

                paddings[Type]->setVisible(true);
                plots[Type]->setVisible(true);
                if (Type!=PlotType_Axis)
                     legends[Type]->setVisible(true);
                else
                     XAxis_Kind->setVisible(true);
            }
            Pos++;
        }
        else
        {
            if (paddings[Type])
                paddings[Type]->setVisible(false);
            if (plots[Type])
                plots[Type]->setVisible(false);
            if (legends[Type])
                legends[Type]->setVisible(false);
            Layout->removeItem(Layouts[Type]);
            delete Layouts[Type]; Layouts[Type]=NULL;
        }

    setLayout(Layout);
    createData_Update();
}

//---------------------------------------------------------------------------
void Plots::refreshDisplay_Axis()
{
    // Paddings
    int Width_Max=0;
    for (size_t Type=0; Type<PlotType_Max; Type++)
    {
        int Width_Temp=plots[Type]->axisWidget(QwtPlot::yLeft)->width();
        if (Width_Temp>Width_Max)
            Width_Max=Width_Temp;
    }
    for (size_t Type=0; Type<PlotType_Max; Type++)
    {
        int temp_Width=Width_Max-plots[Type]->axisWidget(QwtPlot::yLeft)->width();
        paddings[Type]->setMinimumWidth(temp_Width);
        paddings[Type]->setMaximumWidth(temp_Width);
    }

    // Legends
    Width_Max=0;
    for (size_t Type=0; Type<PlotType_Max; Type++)
    {
        int Width_Temp;
        if (Type==PlotType_Axis)
        {
            #ifdef _WIN32
                Width_Temp=XAxis_Kind->width();
            #else // _WIN32
                Width_Temp=XAxis_Kind->width()-3; // FIXME: For a reason I ignore, plots and XAxis_Kind overlap on Mac, this is an hack to show them correctly
            #endif //_WIN32
        }
        else
            Width_Temp=legends[Type]->width();
        if (Width_Temp>Width_Max)
            Width_Max=Width_Temp;
    }
    for (size_t Type=0; Type<PlotType_Max; Type++)
    {
        if (Type==PlotType_Axis)
        {
            #ifdef _WIN32
                XAxis_Kind->setMinimumWidth(Width_Max);
                XAxis_Kind->setMaximumWidth(Width_Max);
            #else // _WIN32
                XAxis_Kind->setMinimumWidth(Width_Max+3); // FIXME: For a reason I ignore, plots and XAxis_Kind overlap on Mac, this is an hack to show them correctly
                XAxis_Kind->setMaximumWidth(Width_Max+3);
            #endif //_WIN32
        }
        else
        {
            legends[Type]->setMinimumWidth(Width_Max);
            legends[Type]->setMaximumWidth(Width_Max);
        }
    }

    //RePlot
    for (size_t Type=0; Type<PlotType_Max; Type++)
        if (plots[Type])
            plots[Type]->replot();
}

void Plots::plot_moved( const QPointF &pos )
{
    Marker_RealPoint=pos;

    double X=pos.x();
    if (X<0)
        X=0;

    double* x=FileInfoData->Videos[0]->x[XAxis_Kind_index];
    size_t Pos=0;  
    while (Pos<FileInfoData->Videos[0]->x_Current_Max && X>=x[Pos])
        Pos++;
    if (Pos)
    {
        if (Pos>=FileInfoData->Videos[0]->x_Current)
            Pos=FileInfoData->Videos[0]->x_Current-1;

        double Distance1=X-x[Pos-1];
        double Distance2=x[Pos]-X;
        if (Distance1<Distance2)
            Pos--;
    }
    X=x[Pos];

    FileInfoData->Frames_Pos_Set(Pos);
    Marker_Update(X);
}

void Plots::on_XAxis_Kind_currentIndexChanged(int index)
{
    XAxis_Kind_index=index;

    createData_Update();
    Zoom_Update();
}

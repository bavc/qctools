/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/Plots.h"
#include "GUI/Plot.h"
#include "Core/Core.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qwt_legend.h>
#include <qwt_scale_widget.h>
#include <cmath>
//---------------------------------------------------------------------------

class KindComboBox: public QComboBox
{
public:
    KindComboBox( QWidget* parent ):
        QComboBox( parent )
    {
        setContentsMargins( 0, 0, 0, 0 );

        addItem( "Frames" );
        addItem( "Seconds" );
        addItem( "Minutes" );
        addItem( "Hours" );
    }
};

class DummyAxisPlot: public QwtPlot
{
public:
    DummyAxisPlot( QWidget* parent ):
        QwtPlot( parent )
    {
        setMaximumHeight( axisWidget( QwtPlot::xBottom )->height() );
        enableAxis( QwtPlot::xBottom, true );
    }
};

class PlotLegend: public QwtLegend
{
public:
    PlotLegend( QWidget *parent ):
        QwtLegend( parent )
    {
        setMinimumHeight( 1 );
        setMaxColumns( 1 );
        setContentsMargins( 0, 0, 0, 0 );
        contentsWidget()->layout()->setAlignment( Qt::AlignLeft | Qt::AlignTop );

        QFont fnt = font();
        fnt.setPointSize( 8 );

        setFont( fnt );
    }
};

//---------------------------------------------------------------------------
Plots::Plots( QWidget *parent, FileInformation* FileInformationData_ ) :
    QWidget( parent ),
    m_fileInfoData( FileInformationData_ ),
    m_zoomLevel( 1 ),
    m_dataTypeIndex( 1 ),
    m_Data_FramePos_Max( 0 )
{

    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 0 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    for ( int row = 0; row < PlotType_Max; row++ )
    {
        if ( row != PlotType_Axis )
        {
            Plot* plot = new Plot( ( PlotType )row, this );
			plot->setMinimumHeight( 1 );

            QwtLegend *legend = new PlotLegend( this );
            connect( plot, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
                     legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

            connect( plot, SIGNAL( cursorMoved( double ) ), SLOT( onCursorMoved( double ) ) );
            plot->updateLegend();

            if ( PerPlotGroup[row].Count > 3 )
                plot->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

            layout->addWidget( legend, row, 1 );
            m_plots[row] = plot;
        }
        else
        {
            KindComboBox* comboBox = new KindComboBox( this );
            comboBox->setCurrentIndex( m_dataTypeIndex );
            connect( comboBox, SIGNAL( currentIndexChanged( int ) ),
                     this, SLOT( onDataTypeChanged( int ) ) );

            layout->addWidget( comboBox, row, 1 );
            m_plots[row] = new DummyAxisPlot( this );;
        }

        m_plots[row]->setAxisScale( QwtPlot::xBottom, 0, videoStats()->x_Max[m_dataTypeIndex] );
        layout->addWidget( m_plots[row], row, 0 );

        setPlotVisible( ( PlotType )row, false );
    }

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
}

//---------------------------------------------------------------------------
const QwtPlot* Plots::plot( PlotType Type ) const
{
    return m_plots[Type];
}

//---------------------------------------------------------------------------
Plot* Plots::plotAt( int row )
{
    return dynamic_cast<Plot*>( m_plots[row] );
}

//---------------------------------------------------------------------------
const Plot* Plots::plotAt( int row ) const
{
    return dynamic_cast<Plot*>( m_plots[row] );
}

//---------------------------------------------------------------------------
void Plots::updateAll()
{
    syncPlots();
    alignYAxes();
    replotAll();
}

//---------------------------------------------------------------------------
void Plots::Plots_Update()
{
    const size_t pos = framePos();

    // Put the current frame in center
    if ( m_zoomLevel != 1 )
    {
        const size_t increment = m_Data_FramePos_Max / m_zoomLevel;

        size_t NewBegin = 0;
        if ( pos > increment / 2 )
        {
            NewBegin = pos - increment / 2;
            if ( NewBegin + increment > m_Data_FramePos_Max )
                NewBegin = m_Data_FramePos_Max - increment;
        }
        shiftXAxes( NewBegin );
    	replotAll();
    }

    setCursorPos( videoStats()->x[m_dataTypeIndex][pos] );
}

//---------------------------------------------------------------------------
void Plots::Marker_Update()
{
	setCursorPos( videoStats()->x[m_dataTypeIndex][framePos()] );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( double x )
{
    for ( int row = 0; row < PlotType_Axis; ++row )
        plotAt( row )->setCursorPos( x );
}

//---------------------------------------------------------------------------
void Plots::syncPlots()
{
    for ( int i = 0; i < PlotType_Axis; i++ )
    {
        if ( m_plots[i]->isVisibleTo( this ) )
            syncPlot( ( PlotType )i );
    }

    if ( m_Data_FramePos_Max + 1 != videoStats()->x_Current_Max )
    {
        //Update of zoom in case of total duration change
        m_Data_FramePos_Max = videoStats()->x_Current_Max - 1;
        shiftXAxes();
    }

    replotAll();
}

//---------------------------------------------------------------------------
double Plots::axisStepSize( double s ) const
{
	for ( int d = 1; d <= 1000000; d *= 10 )
	{
		const double step = floor( s * d ) / d;
		if ( step > 0.0 )
			return step;
	}

	return 0.0;
}

//---------------------------------------------------------------------------
void Plots::syncPlot( PlotType Type )
{
    Plot* plot = plotAt( Type );
    if ( plot == NULL )
        return;

    VideoStats* video = videoStats();

    if ( PerPlotGroup[Type].Min != PerPlotGroup[Type].Max &&
            video->y_Max[Type] >= PerPlotGroup[Type].Max / 2 )
    {
        video->y_Max[Type] = PerPlotGroup[Type].Max;
    }

    const double yMax = video->y_Max[Type];
    if ( yMax )
    {
        if ( yMax > plot->axisInterval( QwtPlot::yLeft ).maxValue() )
        {
			const double stepCount = PerPlotGroup[Type].StepsCount;
			const double stepSize = axisStepSize( yMax / stepCount );

            if ( stepSize )
                plot->setAxisScale( QwtPlot::yLeft, 0, yMax, stepSize );
        }
    }
    else
    {
        //Special case, in order to force a scale of 0 to 1
        plot->setAxisScale( QwtPlot::yLeft, 0, 1, 1 );
    }

    for( unsigned j = 0; j < PerPlotGroup[Type].Count; ++j )
    {
        plot->setCurveSamples( j, video->x[m_dataTypeIndex],
            video->y[PerPlotGroup[Type].Start + j], video->x_Current );
    }
}

//---------------------------------------------------------------------------
void Plots::shiftXAxes()
{
    const size_t increment = m_Data_FramePos_Max / m_zoomLevel;

    int pos = framePos();
    if ( pos == -1 )
        return;

    if ( pos > increment / 2 )
        pos -= increment / 2;
    else
        pos = 0;

    shiftXAxes( pos );
    replotAll();
}

//---------------------------------------------------------------------------
void Plots::Zoom_Move( size_t Begin )
{
    shiftXAxes( Begin );
    replotAll();
}

//---------------------------------------------------------------------------
void Plots::shiftXAxes( size_t Begin )
{
    size_t increment = m_Data_FramePos_Max / m_zoomLevel;
    if ( Begin + increment > m_Data_FramePos_Max )
        Begin = m_Data_FramePos_Max - increment;

    const double x = videoStats()->x[m_dataTypeIndex][Begin];
    const double width = videoStats()->x_Max[m_dataTypeIndex] / m_zoomLevel;

    for ( int row = 0; row < PlotType_Max; row++ )
        m_plots[row]->setAxisScale( QwtPlot::xBottom, x, x + width );
}


//---------------------------------------------------------------------------
void Plots::alignYAxes()
{
    double maxExtent = 0;

    for ( int i = 0; i < PlotType_Max; i++ )
    {
        QwtScaleWidget *scaleWidget = m_plots[i]->axisWidget( QwtPlot::yLeft );

        QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
        scaleDraw->setMinimumExtent( 0.0 );

        if ( m_plots[i]->isVisibleTo( this ) )
        {
            const double extent = scaleDraw->extent( scaleWidget->font() );
            maxExtent = qMax( extent, maxExtent );
        }
    }
    maxExtent += 3; // margin

    for ( int i = 0; i < PlotType_Max; i++ )
    {
        QwtScaleWidget *scaleWidget = m_plots[i]->axisWidget( QwtPlot::yLeft );
        scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
    }
}

//---------------------------------------------------------------------------
void Plots::onCursorMoved( double cursorX )
{
    const double* xData = videoStats()->x[m_dataTypeIndex];

    size_t pos = 0;
    while ( pos < videoStats()->x_Current_Max && cursorX >= xData[pos] )
        pos++;

    if ( pos )
    {
        if ( pos >= videoStats()->x_Current )
            pos = videoStats()->x_Current - 1;

        double Distance1 = cursorX - xData[pos - 1];
        double Distance2 = xData[pos] - cursorX;
        if ( Distance1 < Distance2 )
            pos--;
    }

    m_fileInfoData->Frames_Pos_Set( pos );
    setCursorPos( xData[pos] );
}

//---------------------------------------------------------------------------
void Plots::onDataTypeChanged( int index )
{
    m_dataTypeIndex = index;

    syncPlots();
    Marker_Update();
    shiftXAxes();
}

//---------------------------------------------------------------------------
void Plots::setPlotVisible( PlotType Type, bool on )
{
    const int row = Type;

    QGridLayout* l = dynamic_cast<QGridLayout*>( layout() );

    if ( row < l->rowCount() )
    {
        for ( int col = 0; col < l->columnCount(); col++ )
        {
            QLayoutItem *item = l->itemAtPosition( row, col );
            if ( item && item->widget() )
                item->widget()->setVisible( on );
        }
    }
}

//---------------------------------------------------------------------------
void Plots::zoom( bool up )
{
    if ( up )
    {
        m_zoomLevel *= 2;
    }
    else
    {
        if ( m_zoomLevel > 1 )
            m_zoomLevel /= 2;
    }
}

//---------------------------------------------------------------------------
void Plots::replotAll()
{
    for ( int i = 0; i < PlotType_Max; i++ )
    {
        if ( m_plots[i]->isVisibleTo( this ) )
            m_plots[i]->replot();
    }
}

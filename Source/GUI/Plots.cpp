/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/Plots.h"
#include "GUI/Plot.h"
#include "GUI/PlotLegend.h"
#include "GUI/PlotScaleWidget.h"
#include "Core/Core.h"
#include <QComboBox>
#include <QGridLayout>
#include <cmath>

//---------------------------------------------------------------------------

class XAxisFormatBox: public QComboBox
{
public:
    XAxisFormatBox( QWidget* parent = NULL ):
        QComboBox( parent )
    {
        setContentsMargins( 0, 0, 0, 0 );

        addItem( "Frames" );
        addItem( "Seconds" );
        addItem( "Minutes" );
        addItem( "Hours" );
        addItem( "Time" );
    }
};

//---------------------------------------------------------------------------
Plots::Plots( QWidget *parent, FileInformation* FileInformationData_ ) :
    QWidget( parent ),
    m_fileInfoData( FileInformationData_ ),
    m_zoomLevel( 1 ),
    m_dataTypeIndex( Plots::AxisSeconds )
{
    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 1 );
    layout->setContentsMargins( 0, 0, 0, 0 );

	// bottom scale
	m_scaleWidget = new PlotScaleWidget();
    m_scaleWidget->setFormat( Plots::AxisTime );
	m_scaleWidget->setScale( 0, videoStats()->x_Max[m_dataTypeIndex] );

	// plots and legends

    for ( int row = 0; row < PlotType_Max; row++ )
    {
		Plot* plot = new Plot( ( PlotType )row, this );

		// we allow to shrink the plot below height of the size hint
		plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
        plot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
		initYAxis( plot );

		QwtLegend *legend = new PlotLegend( this );
		connect( plot, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
				 legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

		connect( plot, SIGNAL( cursorMoved( double ) ), SLOT( onCursorMoved( double ) ) );
		plot->updateLegend();

        layout->addWidget( plot, row, 0 );
		layout->addWidget( legend, row, 1 );

		updateSamples( plot );
		m_plots[row] = plot;
    }

	layout->addWidget( m_scaleWidget, PlotType_Max, 0, 1, 2 );

    // combo box for the axis format
    XAxisFormatBox* xAxisBox = new XAxisFormatBox();
    xAxisBox->setCurrentIndex( Plots::AxisTime );
    connect( xAxisBox, SIGNAL( currentIndexChanged( int ) ),
        this, SLOT( onXAxisFormatChanged( int ) ) );

	int axisBoxRow = layout->rowCount() - 1;
#if 1
    // one row below to have space enough for bottom scale tick labels
    layout->addWidget( xAxisBox, PlotType_Max + 1, 1 );
#else
    layout->addWidget( xAxisBox, PlotType_Max, 1 );
#endif

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );

    setCursorPos( videoStats()->x[m_dataTypeIndex][framePos()] );
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


void Plots::scrollXAxis()
{
    // position of the current frame has changed 

    if ( isZoomed() )
    {
    	// Put the current frame in center
    	const size_t pos = framePos();
        const size_t numFrames = visibleFramesCount();

        size_t Begin = 0;
        if ( pos > numFrames / 2 )
        {
            Begin = pos - numFrames / 2;
            if ( Begin + numFrames > ( videoStats()->x_Current_Max - 1 ) )
                Begin = ( videoStats()->x_Current_Max - 1 ) - numFrames;
        }

    	if ( Begin + numFrames > ( videoStats()->x_Current_Max - 1 ) )
        	Begin = ( videoStats()->x_Current_Max - 1 ) - numFrames;

    	const double x = videoStats()->x[m_dataTypeIndex][Begin];
		const double w = m_scaleWidget->interval().width();

    	m_scaleWidget->setScale( x, x + w );

        replotAll();
    }

    setCursorPos( videoStats()->x[m_dataTypeIndex][framePos()] );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( double x )
{
    for ( int i = 0; i < PlotType_Max; ++i )
        m_plots[i]->setCursorPos( x );
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
void Plots::initYAxis( Plot* plot )
{
	const PlotType plotType = plot->type();

    VideoStats* video = videoStats();

    if ( PerPlotGroup[plotType].Min != PerPlotGroup[plotType].Max &&
            video->y_Max[plotType] >= PerPlotGroup[plotType].Max / 2 )
    {
        video->y_Max[plotType] = PerPlotGroup[plotType].Max;
    }

    const double yMax = video->y_Max[plotType];
    if ( yMax )
    {
        if ( yMax > plot->axisInterval( QwtPlot::yLeft ).maxValue() )
        {
            const double stepCount = PerPlotGroup[plotType].StepsCount;
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
}

void Plots::updateSamples( Plot* plot )
{
    const PlotType plotType = plot->type();
	const VideoStats* video = videoStats();

    for( unsigned j = 0; j < PerPlotGroup[plotType].Count; ++j )
    {
        plot->setCurveSamples( j, video->x[m_dataTypeIndex],
            video->y[PerPlotGroup[plotType].Start + j], video->x_Current );
    }
}

//---------------------------------------------------------------------------
void Plots::Zoom_Move( size_t Begin )
{
    const double x0 = videoStats()->x[m_dataTypeIndex][Begin];
    const double width = m_scaleWidget->interval().width();

	double x = x0 + width; 
	if ( x > videoStats()->x_Current_Max )
		x = videoStats()->x_Current_Max;

	m_scaleWidget->setScale( x - width, x );

    replotAll();
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
void Plots::onXAxisFormatChanged( int format )
{
	if ( format == m_scaleWidget->format() )
		return;

	int dataTypeIndex = format;
    if ( format == AxisTime )
        dataTypeIndex = AxisSeconds;

	if ( m_dataTypeIndex != dataTypeIndex )
	{
		const int frame0 = visibleFramesBegin();
		const int numFrames = visibleFramesCount();

		m_dataTypeIndex = dataTypeIndex;

    	for ( int i = 0; i < PlotType_Max; i++ )
           	updateSamples( m_plots[i] );

		const double* x = videoStats()->x[m_dataTypeIndex];
    	m_scaleWidget->setScale( x[frame0], x[frame0 + numFrames - 1] );

		setCursorPos( x[framePos()] );
	}

    m_scaleWidget->setFormat( format );
	m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::setPlotVisible( PlotType Type, bool on )
{
	if ( on == m_plots[Type]->isVisibleTo( this ) )
		return;

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

	alignYAxes();
}

//---------------------------------------------------------------------------
void Plots::zoomXAxis( bool up )
{
    if ( up )
    {
        m_zoomLevel *= 2;
    }
    else
    {
        if ( m_zoomLevel <= 1 )
			return;

        m_zoomLevel /= 2;
    }

	const double xMax = videoStats()->x_Current_Max;

    size_t pos = framePos();
    size_t Increment = xMax / m_zoomLevel;

    if ( pos + Increment / 2 > xMax )
        pos = xMax - Increment / 2;

    if ( pos > Increment / 2 )
        pos -= Increment / 2;
    else
        pos=0;

    if ( pos + Increment > ( videoStats()->x_Current_Max - 1 ) )
        pos = ( videoStats()->x_Current_Max - 1 ) - Increment;

    const double x = videoStats()->x[m_dataTypeIndex][pos];
    const double width = videoStats()->x_Max[m_dataTypeIndex] / m_zoomLevel;

    m_scaleWidget->setScale( x, x + width );

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::replotAll()
{
    for ( int i = 0; i < PlotType_Max; i++ )
    {
		m_plots[i]->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
        if ( m_plots[i]->isVisibleTo( this ) )
            m_plots[i]->replot();
    }
}

bool Plots::isZoomed() const
{
	return m_zoomLevel > 1;
}

bool Plots::isZoomable() const
{
	return visibleFramesCount() > 4;
}

size_t Plots::visibleFramesCount() const
{
	// current scale width translated into frames
	const double w = m_scaleWidget->interval().width();
	return qRound( w * videoStats()->x_Current_Max / videoStats()->x_Max[m_dataTypeIndex] );
}

int Plots::visibleFramesBegin() const
{
	const double* x = videoStats()->x[m_dataTypeIndex];
	const double value = m_scaleWidget->interval().minValue();
#if 1
	// TODO
	int i;
	for ( i = 0; i < videoStats()->x_Current; i++ )
	{
		if ( x[i] > value )
			break;
	}
	return i - 1;
#endif
}

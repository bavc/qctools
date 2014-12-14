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
    m_dataTypeIndex( Plots::AxisSeconds )
{
    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 1 );
    layout->setContentsMargins( 0, 0, 0, 0 );

	// bottom scale
	m_scaleWidget = new PlotScaleWidget();
    m_scaleWidget->setFormat( Plots::AxisTime );
	setFrameRange( 0, numFrames() - 1 );

	// plots and legends

    for ( int row = 0; row < PlotType_Max; row++ )
    {
		Plot* plot = new Plot( ( PlotType )row, this );

		// we allow to shrink the plot below height of the size hint
		plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
        plot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
		initYAxis( plot );
		updateSamples( plot );

		connect( plot, SIGNAL( cursorMoved( double ) ), SLOT( onCursorMoved( double ) ) );

        layout->addWidget( plot, row, 0 );
		layout->addWidget( plot->legend(), row, 1 );

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

void Plots::setFrameRange( int from, int to )
{
	m_visibleFrame[0] = from;
	m_visibleFrame[1] = to;

	const double* x = videoStats()->x[m_dataTypeIndex];
	m_scaleWidget->setScale( x[from], x[to] );
}

void Plots::scrollXAxis()
{
    // position of the current frame has changed 

    if ( m_visibleFrame[1] < numFrames() - 1 )
    {
    	const int pos = framePos();
        const int numVisibleFrames = visibleFramesCount();

        if ( pos > m_visibleFrame[0] + numVisibleFrames / 2 )
		{
    		// Put the current frame in center
            const int from = pos - numVisibleFrames / 2;
			const int to = qMin( from + numVisibleFrames, numFrames() ) - 1;

			setFrameRange( to - numVisibleFrames, to );

        	replotAll();
		}
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
void Plots::Zoom_Move( int Begin )
{
	const int numVisibleFrames = visibleFramesCount();
	const int to = qMin( Begin + numVisibleFrames, numFrames() - 1 );

	setFrameRange( to - numVisibleFrames + 1, to );

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
    while ( pos < numFrames() && cursorX >= xData[pos] )
        pos++;

    if ( pos )
    {
#if 0
        // numFrames() ???
        if ( pos >= videoStats()->x_Current )
            pos = videoStats()->x_Current - 1;
#else
        if ( pos >= numFrames() )
            pos = numFrames() - 1;
#endif

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
		m_dataTypeIndex = dataTypeIndex;

    	for ( int i = 0; i < PlotType_Max; i++ )
           	updateSamples( m_plots[i] );

    	setFrameRange( m_visibleFrame[0], m_visibleFrame[1] );

		const double* x = videoStats()->x[m_dataTypeIndex];
		setCursorPos( x[framePos()] );
	}

    m_scaleWidget->setFormat( format );
	m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::setPlotVisible( PlotType Type, bool on )
{
	if ( on != m_plots[Type]->isVisibleTo( this ) )
	{
		m_plots[Type]->setVisible( on );
		m_plots[Type]->legend()->setVisible( on );

		alignYAxes();
    }
}

//---------------------------------------------------------------------------
void Plots::zoomXAxis( bool up )
{
	int numVisibleFrames = visibleFramesCount();
    if ( up )
		numVisibleFrames /= 2;
    else
		numVisibleFrames *= 2;

	const int to = qMin( m_visibleFrame[0] + numVisibleFrames, numFrames() ) - 1;
	const int from = qMax( 0, to - numVisibleFrames );

    setFrameRange( from, to );

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
	return visibleFramesCount() < numFrames();
}

bool Plots::isZoomable() const
{
	return visibleFramesCount() > 4;
}

size_t Plots::visibleFramesCount() const
{
	return m_visibleFrame[1] - m_visibleFrame[0] + 1;
}

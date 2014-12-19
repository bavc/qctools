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
#include <QEvent>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
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
Plots::Plots( QWidget *parent, FileInformation* fileInformation ) :
    QWidget( parent ),
    m_fileInfoData( fileInformation ),
    m_dataTypeIndex( Plots::AxisSeconds )
{
    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 1 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    // bottom scale
    m_scaleWidget = new PlotScaleWidget();
    m_scaleWidget->setFormat( Plots::AxisTime );
    setVisibleFrames( 0, numFrames() - 1 );

    // plots and legends

    for ( int row = 0; row < PlotType_Max; row++ )
    {
        Plot* plot = new Plot( ( PlotType )row, this );

        // we allow to shrink the plot below height of the size hint
        plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
        plot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
        initYAxis( plot );
        updateSamples( plot );

        connect( plot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );

        plot->canvas()->installEventFilter( this );

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

    setCursorPos( framePos() );
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
void Plots::setVisibleFrames( int from, int to )
{
	if ( from != m_frameInterval.from || to != m_frameInterval.to )
	{
    	m_frameInterval.from = from;
    	m_frameInterval.to = to;

    	const double* x = videoStats()->x[m_dataTypeIndex];
    	m_scaleWidget->setScale( x[from], x[to] );
	}
}

//---------------------------------------------------------------------------
FrameInterval Plots::visibleFrames() const
{
    return m_frameInterval;
}

//---------------------------------------------------------------------------
void Plots::onCurrentFrameChanged()
{
    // position of the current frame has changed 

    if ( isZoomed() )
    {
        const int n = m_frameInterval.count();

        const int from = qBound( 0, framePos() - n / 2, numFrames() - n );
        const int to = from + n - 1;

        if ( from != m_frameInterval.from )
        {
            setVisibleFrames( from, to );
            replotAll();
        }
    }

    setCursorPos( framePos() );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( int framePos )
{
    const double x = videoStats()->x[m_dataTypeIndex][framePos];
    for ( int i = 0; i < PlotType_Max; ++i )
        m_plots[i]->setCursorPos( x );
}

//---------------------------------------------------------------------------
void Plots::initYAxis( Plot* plot )
{
    const PlotType plotType = plot->type();

    VideoStats* video = videoStats();
    const struct per_plot_group& group = PerPlotGroup[plotType];

    double yMax = video->y_Max[plotType];
    if ( ( group.Min != group.Max ) && ( yMax >= group.Max / 2 ) )
        yMax = group.Max;

    if ( yMax )
    {
        plot->setYAxis( yMax, group.StepsCount );
    }
    else
    {
        //Special case, in order to force a scale of 0 to 1
        plot->setYAxis( 1.0, 1 );
    }

}

//---------------------------------------------------------------------------
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
	const int n = m_frameInterval.count();

	const int from = qMax( Begin, 0 );
	const int to = qMin( numFrames(), from + n ) - 1;

    setVisibleFrames( from, to );

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

bool Plots::eventFilter( QObject *object, QEvent *event )
{
    if ( event->type() == QEvent::Move || event->type() == QEvent::Resize )
    {
        for ( int i = 0; i < PlotType_Max; i++ )
        {
            if ( m_plots[i]->isVisibleTo( this ) )
            {
                if ( object == m_plots[i]->canvas() )
                    alignXAxis( m_plots[i] );

                break;
            }
        }
    }

    return QWidget::eventFilter( object, event );
}

void Plots::alignXAxis( const Plot* plot )
{
    const QWidget* canvas = plot->canvas();

    QRect r = canvas->geometry(); 
    r.moveTopLeft( mapFromGlobal( plot->mapToGlobal( r.topLeft() ) ) );

    int left = r.left();
    left += plot->plotLayout()->canvasMargin( QwtPlot::yLeft );

    int right = width() - ( r.right() - 1 );
    right += plot->plotLayout()->canvasMargin( QwtPlot::yRight );

    m_scaleWidget->setBorderDist( left, right );
}

//---------------------------------------------------------------------------
void Plots::onCursorMoved( int framePos )
{
    m_fileInfoData->Frames_Pos_Set( framePos );
    setCursorPos( framePos );
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

        setVisibleFrames( m_frameInterval.from, m_frameInterval.to );
        setCursorPos( framePos() );
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
    int numVisibleFrames = m_frameInterval.count();
    if ( up )
        numVisibleFrames /= 2;
    else
        numVisibleFrames *= 2;

    const int to = qMin( m_frameInterval.from + numVisibleFrames, numFrames() ) - 1;
    const int from = qMax( 0, to - numVisibleFrames );

    setVisibleFrames( from, to );

    replotAll();
}

bool Plots::isZoomed() const
{
    return m_frameInterval.count() < numFrames();
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

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
    m_zoomFactor ( 1 ),
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
    m_plots = new Plot**[m_fileInfoData->Stats.size()];
    size_t layout_y=0;
    
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        size_t countOfGroups = PerStreamType[type].CountOfGroups;
        
        m_plots[streamPos] = new Plot*[countOfGroups + 1]; //+1 for axix
    
        for ( size_t group = 0; group < countOfGroups; group++ )
        {
            Plot* plot = new Plot( streamPos, type, group, this );

            // we allow to shrink the plot below height of the size hint
            plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
            plot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
            initYAxis( plot );
            updateSamples( plot );

            connect( plot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );

            plot->canvas()->installEventFilter( this );

            layout->addWidget( plot, layout_y, 0 );
            layout->addWidget( plot->legend(), layout_y, 1 );

            m_plots[streamPos][group] = plot;

            layout_y++;
        }
    }

    layout->addWidget( m_scaleWidget, layout_y, 0, 1, 2 );

    // combo box for the axis format
    XAxisFormatBox* xAxisBox = new XAxisFormatBox();
    xAxisBox->setCurrentIndex( Plots::AxisTime );
    connect( xAxisBox, SIGNAL( currentIndexChanged( int ) ),
        this, SLOT( onXAxisFormatChanged( int ) ) );

    int axisBoxRow = layout->rowCount() - 1;
#if 1
    // one row below to have space enough for bottom scale tick labels
    layout->addWidget( xAxisBox, layout_y + 1, 1 );
#else
    layout->addWidget( xAxisBox, layout_y, 1 );
#endif

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    setCursorPos( framePos() );
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
}

//---------------------------------------------------------------------------
const QwtPlot* Plots::plot( size_t streamPos, size_t Group ) const
{
    return m_plots[streamPos][Group];
}

//---------------------------------------------------------------------------
void Plots::refresh()
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        for ( int i = 0; i < PerStreamType[type].CountOfGroups; i++ )
        {
            initYAxis( m_plots[streamPos][i] );
            updateSamples( m_plots[streamPos][i] );
        }
    }

    setCursorPos( framePos() );
    replotAll();
}

//---------------------------------------------------------------------------
void Plots::setVisibleFrames( int from, int to , bool force)
{
    if ( from != m_frameInterval.from || to != m_frameInterval.to || force)
    {
        m_frameInterval.from = from;
        m_frameInterval.to = to;

        const double* x = stats(0)->x[m_dataTypeIndex];
        m_timeInterval.from = x[from];
        m_timeInterval.to = x[to];

        // Handling unfinished parsing with estimated x
        if ( m_timeInterval.from == 0  && from)
            m_timeInterval.from = stats(0)->x_Max[m_dataTypeIndex] / stats(0)->x_Max[0] * from;
        if ( m_timeInterval.to == 0 )
            m_timeInterval.to = stats(0)->x_Max[m_dataTypeIndex] / stats(0)->x_Max[0] * to;
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
void Plots::setCursorPos( int newFramePos )
{
    setFramePos( newFramePos );

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        const double x = m_fileInfoData->Stats[streamPos]->x[m_dataTypeIndex][framePos(streamPos)];

        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

        for ( int i = 0; i < PerStreamType[type].CountOfGroups; ++i )
            m_plots[streamPos][i]->setCursorPos( x );
    }

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);
    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::initYAxis( Plot* plot )
{
    const size_t plotType = plot->type();
    const size_t plotGroup = plot->group();

    CommonStats* stat = stats( plot->streamPos() );
    const struct per_group& group = PerStreamType[plotType].PerGroup[plotGroup];

    double yMin = stat->y_Min[plotGroup];
    double yMax = stat->y_Max[plotGroup];
    if ( ( group.Min != group.Max ) && ( yMax - yMin >= ( group.Max - group.Min) / 2 ) )
        yMax = group.Max;

    if ( yMin != yMax )
    {
        plot->setYAxis( yMin, yMax, group.StepsCount );
    }
    else
    {
        //Special case, in order to force a scale of 0 to 1
        plot->setYAxis( 0.0, 1.0, 1 );
    }

}

//---------------------------------------------------------------------------
void Plots::updateSamples( Plot* plot )
{
    const size_t plotType = plot->type();
    const size_t plotGroup = plot->group();
    const CommonStats* stat = stats( plot->streamPos() );

    for (size_t type = 0; type < CountOfStreamTypes; type++)
        for( unsigned j = 0; j < PerStreamType[type].CountOfGroups; ++j )
        {
            plot->setCurveSamples( j, stat->x[m_dataTypeIndex],
                stat->y[PerStreamType[plotType].PerGroup[plotGroup].Start + j], stat->x_Current );
        }
}

//---------------------------------------------------------------------------
void Plots::Zoom_Move( int Begin )
{
    const int n = m_frameInterval.count();

    const int from = qMax( Begin, 0 );
    const int to = qMin( numFrames(), from + n ) - 1;

    setVisibleFrames( to - n + 1, to );

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
		size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
            m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
    }

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    refresh();

    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::alignYAxes()
{
    double maxExtent = 0;

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
        {
            QwtScaleWidget *scaleWidget = m_plots[streamPos][group]->axisWidget( QwtPlot::yLeft );

            QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
            scaleDraw->setMinimumExtent( 0.0 );

            if ( m_plots[streamPos][group]->isVisibleTo( this ) )
            {
                const double extent = scaleDraw->extent( scaleWidget->font() );
                maxExtent = qMax( extent, maxExtent );
            }
        }
        maxExtent += 3; // margin
    }

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
        {
            QwtScaleWidget *scaleWidget = m_plots[streamPos][group]->axisWidget( QwtPlot::yLeft );
            scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
        }
    }
}

bool Plots::eventFilter( QObject *object, QEvent *event )
{
    if ( event->type() == QEvent::Move || event->type() == QEvent::Resize )
    {
        for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        {
            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
            {
                if ( m_plots[streamPos][group]->isVisibleTo( this ) )
                {
                    if ( object == m_plots[streamPos][group]->canvas() )
                        alignXAxis( m_plots[streamPos][group] );

                    break;
                }
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

        setVisibleFrames( m_frameInterval.from, m_frameInterval.to, true );

        for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        {
		    size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
        }

        m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

        refresh();
    }

    m_scaleWidget->setFormat( format );
    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::setPlotVisible( size_t type, size_t group, bool on )
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        if ( type == m_fileInfoData->Stats[streamPos]->Type_Get() && on != m_plots[streamPos][group]->isVisibleTo( this ))
        {
            m_plots[streamPos][group]->setVisible( on );
            m_plots[streamPos][group]->legend()->setVisible( on );
        }
    }

    alignYAxes();
}

//---------------------------------------------------------------------------
void Plots::zoomXAxis( bool up )
{
    if ( up )
        m_zoomFactor++;
    else if ( m_zoomFactor > 1)
        m_zoomFactor--;
        
    int numVisibleFrames = m_fileInfoData->Frames_Count_Get() / (1 << m_zoomFactor);

    int to = qMin( framePos() + numVisibleFrames / 2, numFrames() );
    int from = qMax( 0, to - numVisibleFrames );

    setVisibleFrames( from, to );

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
		size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
            m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
    }

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    refresh();

    m_scaleWidget->update();

    replotAll();
}

bool Plots::isZoomed() const
{
    return m_frameInterval.count() < numFrames();
}

//---------------------------------------------------------------------------
void Plots::replotAll()
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
		size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

        for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
        {
            m_plots[streamPos][group]->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
            if ( m_plots[streamPos][group]->isVisibleTo( this ) )
                m_plots[streamPos][group]->replot();
        }
    }
}

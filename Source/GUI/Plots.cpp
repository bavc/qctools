/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/Plots.h"
#include "GUI/Plot.h"
#include "Core/CommonStats.h"
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
Plots::Plots( QWidget *parent, const struct stream_info* streamInfo, FileInformation* FileInformationData_, size_t StatsPos  ) :
    QWidget( parent ),
    m_fileInfoData( FileInformationData_ ),
    m_zoomLevel( 1 ),
    m_dataTypeIndex( 1 ),
    m_Data_FramePos_Max( 0 ),
    m_statsPos( StatsPos ),
    m_streamInfo( streamInfo )
{

    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 0 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_plots=new QwtPlot*[m_streamInfo->CountOfGroups];
    for ( size_t row = 0; row < m_streamInfo->CountOfGroups; row++ )
    {
        if ( row != m_streamInfo->CountOfGroups-1 ) // Group_Axis
        {
            Plot* plot = new Plot( m_streamInfo, row, this );
			plot->setMinimumHeight( 50 );

            QwtLegend *legend = new PlotLegend( this );
            connect( plot, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
                     legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

            connect( plot, SIGNAL( cursorMoved( double ) ), SLOT( onCursorMoved( double ) ) );
            plot->updateLegend();

            if ( m_streamInfo->PerGroup[row].Count > 3 )
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

        m_plots[row]->setAxisScale( QwtPlot::xBottom, 0, stats()->x_Max[m_dataTypeIndex] );
        layout->addWidget( m_plots[row], row, 0 );

        setPlotVisible( row, false );
    }

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
}

//---------------------------------------------------------------------------
const QwtPlot* Plots::plot( size_t Type ) const
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

    setCursorPos( stats()->x[m_dataTypeIndex][pos] );
}

//---------------------------------------------------------------------------
void Plots::Marker_Update()
{
	setCursorPos( stats()->x[m_dataTypeIndex][framePos()] );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( double x )
{
    for ( int row = 0; row < m_streamInfo->CountOfGroups-1; ++row ) // Group_Axis
        plotAt( row )->setCursorPos( x );
}

//---------------------------------------------------------------------------
void Plots::syncPlots()
{
    for ( int i = 0; i < m_streamInfo->CountOfGroups-1; i++ ) // Group_Axis
    {
        if ( m_plots[i]->isVisibleTo( this ) )
            syncPlot( i );
    }

    if ( m_Data_FramePos_Max + 1 != stats()->x_Current_Max )
    {
        //Update of zoom in case of total duration change
        m_Data_FramePos_Max = stats()->x_Current_Max - 1;
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
void Plots::syncPlot( size_t group )
{
    Plot* plot = plotAt( group );
    if ( plot == NULL )
        return;

    CommonStats* Stats = stats();

    if ( m_streamInfo->PerGroup[group].Min != m_streamInfo->PerGroup[group].Max &&
         Stats->y_Max[group] >= m_streamInfo->PerGroup[group].Max / 2 )
    {
        Stats->y_Max[group] = m_streamInfo->PerGroup[group].Max;
    }

    const double yMax = Stats->y_Max[group];
    const double yMin = Stats->y_Min[group];
    if ( yMin != yMax )
    {
        if ( yMin < plot->axisInterval( QwtPlot::yLeft ).minValue()
          || yMax > plot->axisInterval( QwtPlot::yLeft ).maxValue() )
        {
			const double stepCount = m_streamInfo->PerGroup[group].StepsCount;
			const double stepSize = axisStepSize( ( yMax - yMin ) / stepCount );

            if ( stepSize )
                plot->setAxisScale( QwtPlot::yLeft, yMin, yMax, stepSize );
        }
    }
    else
    {
        //Special case, in order to force a scale of 0 to 1
        plot->setAxisScale( QwtPlot::yLeft, 0, 1, 1 );
    }

    for( unsigned j = 0; j < m_streamInfo->PerGroup[group].Count; ++j )
    {
        plot->setCurveSamples( j, Stats->x[m_dataTypeIndex],
            Stats->y[m_streamInfo->PerGroup[group].Start + j], Stats->x_Current );
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

    const double x = stats()->x[m_dataTypeIndex][Begin];
    const double width = stats()->x_Max[m_dataTypeIndex] / m_zoomLevel;

    for ( int row = 0; row < m_streamInfo->CountOfGroups; row++ )
        m_plots[row]->setAxisScale( QwtPlot::xBottom, x, x + width );
}


//---------------------------------------------------------------------------
void Plots::alignYAxes()
{
    double maxExtent = 0;

    for ( int i = 0; i < m_streamInfo->CountOfGroups; i++ )
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

    for ( int i = 0; i < m_streamInfo->CountOfGroups; i++ )
    {
        QwtScaleWidget *scaleWidget = m_plots[i]->axisWidget( QwtPlot::yLeft );
        scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
    }
}

//---------------------------------------------------------------------------
void Plots::onCursorMoved( double cursorX )
{
    const double* xData = stats()->x[m_dataTypeIndex];

    size_t pos = 0;
    while ( pos < stats()->x_Current_Max && cursorX >= xData[pos] )
        pos++;

    if ( pos )
    {
        if ( pos >= stats()->x_Current )
            pos = stats()->x_Current - 1;

        double Distance1 = cursorX - xData[pos - 1];
        double Distance2 = xData[pos] - cursorX;
        if ( Distance1 < Distance2 )
            pos--;
    }

    m_fileInfoData->Frames_Pos_Set( pos, m_statsPos );
    //setCursorPos( xData[pos] );
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
void Plots::setPlotVisible( size_t Type, bool on )
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
    for ( int i = 0; i < m_streamInfo->CountOfGroups; i++ )
    {
        if ( m_plots[i]->isVisibleTo( this ) )
            m_plots[i]->replot();
    }
}

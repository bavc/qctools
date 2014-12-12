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

#include <QScrollArea>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>
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

class ScaleWidget: public QwtScaleWidget
{
public:
    ScaleWidget( QWidget* parent = NULL ):
        QwtScaleWidget( QwtScaleDraw::BottomScale, parent )
    {
        ScaleDraw* sd = new ScaleDraw();
        sd->setFormat( Plots::AxisTime );
        setScaleDraw( new ScaleDraw() );
    }

	void setScale( double from, double to )
	{
		QwtLinearScaleEngine se;
		setScaleDiv( se.divideScale( from, to, 5, 8 ) );
	}

    void setFormat( int format )
    {
        ScaleDraw* sd = dynamic_cast<ScaleDraw*>( scaleDraw() );
        if ( sd )
            sd->setFormat( format );
    }

	QwtScaleDiv scaleDiv() const
	{
		return scaleDraw()->scaleDiv();
	}

	QwtInterval interval() const
	{
		return scaleDraw()->scaleDiv().interval();
	}

private:
    class ScaleDraw: public QwtScaleDraw
    {
    public:
        void setFormat( int format )
        {
            if ( format != m_format )
            {
                m_format = format;
                invalidateCache();
            }
        }

        virtual QwtText label( double value ) const
        {
            if ( m_format == Plots::AxisTime )
            {
                const int h = static_cast<int>( value / 3600 );
                const int m = static_cast<int>( value / 60 );
                const int s = static_cast<int>( value );

                QString label;

                if ( scaleDiv().interval().width() > 10.0 )
                {
                    label.sprintf( "%02d:%02d:%02d", 
                        h, m - h * 60, s - m * 60 );
                }
                else
                {
                    const int ms = qRound( ( value - s ) * 1000.0 );
                    label.sprintf( "%02d:%02d:%02d.%03d", 
                        h, m - h * 60, s - m * 60, ms);
                }

                return label;
            }

            return QwtScaleDraw::label( value );
        }

    private:
        int m_format;
    };
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

        QLayout* layout = contentsWidget()->layout();
        layout->setAlignment( Qt::AlignLeft | Qt::AlignTop );
        layout->setSpacing( 0 );

        QScrollArea *scrollArea = findChild<QScrollArea *>();
        if ( scrollArea )
        {
            scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        }

#if 1
        QFont fnt = font();
        if ( fnt.pointSize() > 8 )
        {
            fnt.setPointSize( 8 );
            setFont( fnt );
        }
#endif
    }

protected:
    virtual QWidget *createWidget( const QwtLegendData &data ) const
    {
        QWidget *w = QwtLegend::createWidget( data );

        QwtLegendLabel *label = dynamic_cast<QwtLegendLabel *>( w );
        if ( label )
        {
            label->setMargin( 0 );
        }

        return w;
    }
};

//---------------------------------------------------------------------------
Plots::Plots( QWidget *parent, FileInformation* FileInformationData_ ) :
    QWidget( parent ),
    m_fileInfoData( FileInformationData_ ),
    m_zoomLevel( 1 ),
    m_dataTypeIndex( Plots::AxisSeconds ),
    m_Data_FramePos_Max( 0 )
{

    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 1 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    for ( int row = 0; row < PlotType_Max; row++ )
    {
		Plot* plot = new Plot( ( PlotType )row, this );

		// we allow to shrink the plot below height of the size hint
		plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
        plot->setAxisScale( QwtPlot::xBottom, 0, videoStats()->x_Max[m_dataTypeIndex] );

		QwtLegend *legend = new PlotLegend( this );
		connect( plot, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
				 legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

		connect( plot, SIGNAL( cursorMoved( double ) ), SLOT( onCursorMoved( double ) ) );
		plot->updateLegend();


        layout->addWidget( plot, row, 0 );
		layout->addWidget( legend, row, 1 );

		m_plots[row] = plot;
    }

	// bottom scale
	m_scaleWidget = new ScaleWidget();
	m_scaleWidget->setScale( 0, videoStats()->x_Max[m_dataTypeIndex] );
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

	onXAxisFormatChanged( xAxisBox->currentIndex() );
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
void Plots::syncXAxis()
{
	// position of the current frame has changed 
    const size_t pos = framePos();

    // Put the current frame in center
    if ( isZoomed() )
    {
        const size_t increment = zoomIncrement2();

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

	syncMarker();
}

//---------------------------------------------------------------------------
void Plots::syncMarker()
{
    setCursorPos( videoStats()->x[m_dataTypeIndex][framePos()] );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( double x )
{
    for ( int row = 0; row < PlotType_Max; ++row )
        plotAt( row )->setCursorPos( x );
}

//---------------------------------------------------------------------------
void Plots::syncPlots()
{
    for ( int i = 0; i < PlotType_Max; i++ )
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
    const size_t increment = zoomIncrement2();

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
void Plots::shiftXAxes( size_t Begin )
{
    size_t increment = zoomIncrement2();
    if ( Begin + increment > m_Data_FramePos_Max )
        Begin = m_Data_FramePos_Max - increment;

    const double x = videoStats()->x[m_dataTypeIndex][Begin];
    const double width = videoStats()->x_Max[m_dataTypeIndex] / m_zoomLevel;

	m_scaleWidget->setScale( x, x + width );
}


//---------------------------------------------------------------------------
void Plots::Zoom_Move( size_t Begin )
{
    shiftXAxes( Begin );
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
    m_scaleWidget->setFormat( format );

    if ( format == AxisTime )
        m_dataTypeIndex = AxisSeconds;
    else
        m_dataTypeIndex = format;

    syncPlots();
    syncMarker();
    shiftXAxes();
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
        if ( m_zoomLevel > 1 )
            m_zoomLevel /= 2;
    }

    size_t Position = framePos();
    size_t Increment = zoomIncrement3();

    if ( Position + Increment / 2> videoStats()->x_Current_Max )
        Position = videoStats()->x_Current_Max - Increment / 2;

    if ( Position > Increment / 2 )
        Position -= Increment/2;
    else
        Position=0;

    Zoom_Move( Position );
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
	return zoomLevel() > 1;
}

bool Plots::isZoomable() const
{
	return visibleFrameCount() > 4;
}

size_t Plots::visibleFrameCount() const
{
	// current scale width tanslated into frames
	const double w = m_scaleWidget->interval().width();
	return qRound( w * videoStats()->x_Current_Max / videoStats()->x_Max[m_dataTypeIndex] );
}

size_t Plots::zoomIncrement3() const
{
//qDebug() << "zoomIncrement3: " << m_zoomLevel << zoomLevel();
	return videoStats()->x_Current_Max / m_zoomLevel;
}

size_t Plots::zoomIncrement2() const
{
//qDebug() << "zoomIncrement2: " << m_zoomLevel << zoomLevel();
	return m_Data_FramePos_Max / m_zoomLevel;
}

int Plots::zoomLevel() const
{
	return qRound( videoStats()->x_Max[m_dataTypeIndex] / m_scaleWidget->interval().width() );
}

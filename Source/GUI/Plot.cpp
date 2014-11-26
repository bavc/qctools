/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/Plot.h"
#include "Core/VideoCore.h" //Only for some specific colors
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_widget_overlay.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_canvas.h>
#include <qwt_series_data.h>
#include <QResizeEvent>

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};

class PlotPicker: public QwtPlotPicker
{
public:
    PlotPicker( QWidget *canvas ):
        QwtPlotPicker( canvas )
    {
        setAxis( QwtPlot::xBottom, QwtPlot::yLeft );
        setRubberBand( QwtPlotPicker::CrossRubberBand );
        setRubberBandPen( QColor( Qt::green ) );

        setTrackerMode( QwtPicker::AlwaysOn );
        setTrackerPen( QColor( Qt::black ) );

        setStateMachine( new QwtPickerDragPointMachine () );
    }

    virtual QwtText trackerTextF( const QPointF &pos ) const
    {
        // the white text is hard to see over a light canvas background

        QColor bg( Qt::darkGray );
        bg.setAlpha( 160 );

        QwtText text = infoText( indexLower( pos.x() ) );
        text.setBackgroundBrush( QBrush( bg ) );

        return text;
    }

protected:
    virtual QString infoText( int index ) const
    {
        QString info( "Frame: %1" );
        return info.arg( index + 1 );
    }

private:
    const QwtPlotCurve* curve0() const
    {
        const QwtPlotItemList curves = plot()->itemList( QwtPlotItem::Rtti_PlotCurve );
        if ( curves.isEmpty() )
            return NULL;

        return dynamic_cast<const QwtPlotCurve*>( curves.first() );
    }

    int indexLower( double x ) const
    {
        const QwtPlotCurve* curve = curve0();
        if ( curve == NULL )
            return -1;

        int index = qwtUpperSampleIndex<QPointF>(
            *curve->data(), x, compareX() );

        if ( index == -1 )
            index = curve->dataSize();

        return index - 1;
    }
};

class PlotCursor: public QwtWidgetOverlay
{
public:
    PlotCursor( QWidget *canvas ):
        QwtWidgetOverlay( canvas ),
        m_pos( 0.0 )
    {
    }

    void setPosition( double pos )
    {
        if ( m_pos != pos )
        {
            m_pos = pos;
            updateOverlay();
        }
    }

    virtual void drawOverlay( QPainter *painter ) const
    {
        const int pos = translatedPos( m_pos );

        const QRect cr = parentWidget()->contentsRect();
        if ( pos >= cr.left() && pos < cr.right() )
        {
            painter->setPen( Qt::magenta );
            painter->drawLine( pos, cr.top(), pos, cr.bottom() );
        }
    }

    virtual QRegion maskHint() const
    {
        const QRect cr = parentWidget()->contentsRect();
        return QRect( translatedPos( m_pos ), cr.top(), 1, cr.height() );
    }

    virtual bool eventFilter( QObject *object, QEvent *event )
    {
        if ( object == parent() && event->type() == QEvent::Resize )
        {
            const QResizeEvent *resizeEvent = 
                static_cast<const QResizeEvent *>( event );
            resize( resizeEvent->size() );
            updateOverlay();

            return true;
        }

        return QObject::eventFilter( object, event );
    }

private:
    int translatedPos( double pos ) const
    {
        // translate from plot into widget coordinate

        const QwtPlot* plot = dynamic_cast<QwtPlot*>( parent()->parent() );
        if ( plot )
        {
            const QwtScaleMap scaleMap = plot->canvasMap( QwtPlot::xBottom );
            return qRound( scaleMap.transform( pos ) );
        }

        return -1;
    }

    double m_pos;
    int m_widgetPos;
};

class PlotScaleDrawY: public QwtScaleDraw
{
public:
    PlotScaleDrawY()
    {
    }

protected:
    virtual void drawLabel( QPainter *painter, double val ) const
    {
        const int fh = painter->fontMetrics().height();

        if ( length() < fh )
        {
            const QList<double> ticks = scaleDiv().ticks( QwtScaleDiv::MajorTick );
            if ( val != ticks.last() )
                return;
        }
        else if ( length() < 3 * painter->fontMetrics().height() )
        {
            const QList<double> ticks = scaleDiv().ticks( QwtScaleDiv::MajorTick );
            if ( val != ticks.last() && val != ticks.first() )
                return;
        }

        QwtScaleDraw::drawLabel( painter, val );
    }
};

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Plot::Plot( const struct stream_info* streamInfo, size_t group, QWidget *parent) :
    QwtPlot( parent ),
    m_streamInfo( streamInfo ),
    m_group( group )
{
    setAutoReplot( false );

    QwtPlotCanvas* canvas = dynamic_cast<QwtPlotCanvas*>( this->canvas() );
    if ( canvas )
    {
        canvas->setFrameStyle( QFrame::Plain | QFrame::Panel );
        canvas->setLineWidth( 1 );
#if 1
        canvas->setPalette( QColor("Cornsilk") );
#endif
    }

    setAxisMaxMajor( QwtPlot::yLeft, 0 );
    setAxisMaxMinor( QwtPlot::yLeft, 0 );
    setAxisScaleDraw( QwtPlot::yLeft, new PlotScaleDrawY() );

    enableAxis( QwtPlot::xBottom, false );

    // something invalid
    setAxisScale( QwtPlot::xBottom, -1, 0 );
    setAxisScale( QwtPlot::yLeft, -1, 0 );

    // Plot grid
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableXMin( true );
    grid->enableYMin( true );
    grid->setMajorPen( Qt::darkGray, 0, Qt::DotLine );
    grid->setMinorPen( Qt::gray, 0 , Qt::DotLine );
    grid->attach( this );

    m_cursor = new PlotCursor( canvas );
    m_cursor->setPosition( 0 );

    // curves

    for( unsigned j = 0; j < m_streamInfo->PerGroup[m_group].Count; ++j )
    {
        QwtPlotCurve* curve = new QwtPlotCurve( m_streamInfo->PerItem[m_streamInfo->PerGroup[m_group].Start + j].Name );

        curve->setPen( curveColor( j ) );
        curve->setRenderHint( QwtPlotItem::RenderAntialiased );
        curve->setZ( curve->z() - j ); //Invert data order (e.g. MAX before MIN)
        curve->attach( this );

        m_curves += curve;
    }

    PlotPicker* picker = new PlotPicker( canvas );
    connect( picker, SIGNAL( moved( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );
    connect( picker, SIGNAL( selected( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );

    connect( axisWidget( QwtPlot::xBottom ), SIGNAL( scaleDivChanged() ), SLOT( onXScaleChanged() ) );
}

//---------------------------------------------------------------------------
Plot::~Plot()
{
}

QSize Plot::sizeHint() const
{
    const QSize hint = QwtPlot::minimumSizeHint();

    const int fh = axisWidget( QwtPlot::yLeft )->fontMetrics().height();
    const int spacing = 0;
    const int fw = dynamic_cast<const QwtPlotCanvas*>( canvas() )->frameWidth();

    // 4 tick labels 
    return QSize( hint.width(), 4 * fh + 3 * spacing + 2 * fw );
}

QSize Plot::minimumSizeHint() const
{
    const QSize hint = QwtPlot::minimumSizeHint();
    return QSize( hint.width(), -1 );
}

void Plot::setCurveSamples( int index,
    const double *xData, const double *yData, int size )
{
    if ( index >= 0 && index < m_curves.size() )
        m_curves[index]->setRawSamples( xData, yData, size );
}

void Plot::setCursorPos( double x )
{
    m_cursor->setPosition( x );
}

QColor Plot::curveColor( int index ) const
{
    QColor c = Qt::black;

    switch ( m_streamInfo->PerGroup[m_group].Count )
    {
        case 1 :
        {
            switch ( m_group )
            {
                case Group_YDiff: c = Qt::darkGreen; break;
                case Group_UDiff: c = Qt::darkBlue; break;
                case Group_VDiff: c = Qt::darkRed; break;
                default: c = Qt::black;
            }
            break;
        }
        case 2 :
        {
            switch ( index )
            {
                case 0: c = Qt::darkGreen; break;
                case 1: c = Qt::darkRed; break;
                default: c = Qt::black;
            }
            break;
        }
        case 3 :
        {
            switch ( index )
            {
                case 0: c = Qt::darkRed; break;
                case 1: c = Qt::darkBlue; break;
                case 2: c = Qt::darkGreen; break;
                default: c = Qt::black;
            }
            break;
        }
        case 5 :
        {
            switch ( index )
            {
                case 0: c = Qt::red; break;
                case 1: c = QColor::fromRgb( 0x00, 0x66, 0x00 ); break; //Qt::green
                case 2: c = Qt::black; break;
                case 3: c = Qt::green; break;
                case 4: c = Qt::red; break;
                default: c = Qt::black;
            }
            break;
        }
        default:
        {
            c = Qt::black;
        }
    }

    return c;
}

void Plot::onPickerMoved( const QPointF& pos )
{
    Q_EMIT cursorMoved( qMax( pos.x(), 0.0 ) );
}

void Plot::onXScaleChanged()
{
    m_cursor->updateOverlay();
}

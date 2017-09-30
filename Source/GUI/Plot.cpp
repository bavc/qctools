/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/Plot.h"
#include "GUI/PlotLegend.h"
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_widget_overlay.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_marker.h>
#include <qwt_series_data.h>
#include <QResizeEvent>

#include "Core/FileInformation.h"
#include "Core/VideoCore.h"

static double stepSize( double distance, int numSteps )
{
    const double s = distance / numSteps;
    for ( int d = 1; d <= 1000000; d *= 10 )
    {
        const double step = floor( s * d ) / d;
        if ( step > 0.0 )
            return step;
    }

    return 0.0;
}

class PlotPicker: public QwtPlotPicker
{
public:
    PlotPicker( QWidget *canvas , const struct stream_info* streamInfo, const size_t group, const QVector<QwtPlotCurve*>* curves, const FileInformation* fileInformation):
        m_streamInfo( streamInfo ),
        m_group( group ),
        m_curves( curves ),
        QwtPlotPicker( canvas ),
        m_fileInformation(fileInformation)
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

        QwtText text;

        const int idx = dynamic_cast<const Plot*>( plot() )->frameAt( pos.x() );
        if ( idx >= 0 )
        {
            text = infoText( idx );
            text.setBackgroundBrush( QBrush( bg ) );
        }

        return text;
    }

protected:
    virtual QString infoText( int index ) const
    {
        QString info = QString( "Frame %1 [%2]" ).arg(index).arg(m_fileInformation ? m_fileInformation->Frame_Type_Get(-1, index) : "");
        for( unsigned i = 0; i < m_streamInfo->PerGroup[m_group].Count; ++i )
        {
            const per_item &itemInfo = m_streamInfo->PerItem[m_streamInfo->PerGroup[m_group].Start + i];

            info += i?", ":": ";
            info += itemInfo.Name;
            info += "=";

            if ((*m_curves)[i]->dataSize() != 0)
                info += QString::number((*m_curves)[i]->sample(index).ry(), 'f', itemInfo.DigitsAfterComma);
            else
                info += QString("n/a");
        }

        return info;
    }

    const struct stream_info*       m_streamInfo; 
    const size_t                    m_group;
    const QVector<QwtPlotCurve*>*   m_curves;
    const FileInformation*          m_fileInformation;
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
// Helpers
//***************************************************************************

void Plot_AddHLine(QwtPlot* plot, double value , double r , double g, double b)
{
    QwtPlotMarker *marker = new QwtPlotMarker();
    marker->setLineStyle ( QwtPlotMarker::HLine );
    marker->setLinePen(QPen(QColor(r, g, b, 128), 1, Qt::DashDotLine));
    marker->attach( plot );
    marker->setYValue( value );
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
void Plot::addGuidelines(int bitsPerRawSample)
{
    int defaultBitsPerRawSample = 8;
    if(bitsPerRawSample == 0)
        bitsPerRawSample = defaultBitsPerRawSample;

    int multiplier = pow(2, (bitsPerRawSample - defaultBitsPerRawSample));

    if ( m_type == Type_Video )
    switch (m_group)
    {
        case Group_Y :
                        Plot_AddHLine( this,  16 * multiplier,  61,  89, 171);
                        Plot_AddHLine( this, 235 * multiplier, 220,  20,  60);
                        break;
        case Group_U :
        case Group_V :
                        Plot_AddHLine( this,  16 * multiplier,  61,  89, 171);
                        Plot_AddHLine( this, 240 * multiplier, 220,  20,  60);
                        break;
        case Group_Sat :
                        Plot_AddHLine( this,  88 * multiplier, 255,   0, 255);
                        Plot_AddHLine( this, 118 * multiplier, 220,  20,  60);
                        break;
        default      :  ;
    }
}

Plot::Plot( size_t streamPos, size_t Type, size_t Group, const FileInformation* fileInformation, QWidget *parent ) :
    QwtPlot( parent ),
    m_streamPos( streamPos ),
    m_type( Type ),
    m_group( Group ),
    m_fileInformation( fileInformation )
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

    for( unsigned j = 0; j < PerStreamType[m_type].PerGroup[m_group].Count; ++j )
    {
        QwtPlotCurve* curve = new QwtPlotCurve( PerStreamType[m_type].PerItem[PerStreamType[m_type].PerGroup[m_group].Start + j].Name );
        switch (m_group)
        {
            case Group_IDET_S :
            case Group_IDET_M :
            case Group_IDET_R :
                curve->setPen( curveColor( j ), 2 );
                break;
            default :
                curve->setPen( curveColor( j ) );
        }
        curve->setRenderHint( QwtPlotItem::RenderAntialiased );
        curve->setZ( curve->z() - j ); //Invert data order (e.g. MAX before MIN)
        curve->attach( this );

        m_curves += curve;
    }

    PlotPicker* picker = new PlotPicker( canvas, &PerStreamType[m_type], m_group, &m_curves, fileInformation );
    connect( picker, SIGNAL( moved( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );
    connect( picker, SIGNAL( selected( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );

    connect( axisWidget( QwtPlot::xBottom ), SIGNAL( scaleDivChanged() ), SLOT( onXScaleChanged() ) );

    // legend
    m_legend = new PlotLegend();
    
    connect( this, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
         m_legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

    updateLegend();
}

//---------------------------------------------------------------------------
Plot::~Plot()
{
    delete m_legend;
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

const QwtPlotCurve* Plot::curve( int index ) const
{
    const QwtPlotItemList curves = itemList( QwtPlotItem::Rtti_PlotCurve );
    if ( index >= 0 && index < curves.size() )
        return dynamic_cast<const QwtPlotCurve*>( curves[index] );

    return NULL;
}   

void Plot::setYAxis( double min, double max, int numSteps )
{
    setAxisScale( QwtPlot::yLeft, min, max, ::stepSize( max - min, numSteps ) );
}

void Plot::setCursorPos( double x )
{
    m_cursor->setPosition( x );
}

void Plot::setData(int curveIndex, QwtSeriesData<QPointF> *series)
{
    m_curves[curveIndex]->setData(series);
}

QColor Plot::curveColor( int index ) const
{
    QColor c = Qt::black;

    switch ( PerStreamType[m_type].PerGroup[m_group].Count )
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
            switch ( m_group )
            {
                case Group_IDET_R:
                {
                    switch ( index )
                    {
                        case 0: c = Qt::red; break;
                        case 1: c = Qt::blue; break;
                        case 2: c = Qt::magenta; break;
                        default: c = Qt::black;
                    }
                    break;
                }
                default:
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
            }
            break;
        }
        case 4 :
        {
            switch ( m_group )
            {
                case Group_IDET_S:
                case Group_IDET_M:
                {
                    switch ( index )
                    {
                        case 0: c = Qt::red; break;
                        case 1: c = Qt::blue; break;
                        case 2: c = Qt::darkGreen; break;
                        case 3: c = Qt::magenta; break;
                        default: c = Qt::black;
                    }
                    break;
                }
                default:
                {
                    switch ( index )
                    {
                        case 0: c = Qt::darkRed; break;
                        case 1: c = Qt::darkBlue; break;
                        case 2: c = Qt::darkGreen; break;
                        case 3: c = Qt::black; break;
                        default: c = Qt::black;
                    }
                    break;
                }
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
    const int idx = frameAt( pos.x() );
    if ( idx >= 0 )
        Q_EMIT cursorMoved( idx );
}

int Plot::frameAt( double x ) const
{
    const QwtPlotCurve* curve = this->curve(0);
    if ( curve == NULL )
        return -1;

    const QwtSeriesData<QPointF> &data = *curve->data();

    int idx = ::indexLower( x, data );
    if ( idx < 0 )
    {
        idx = 0;
    }
    else if ( idx < data.size() - 1 )
    {
        // index, where x is closer
        const double x1 = data.sample( idx ).x();
        const double x2 = data.sample( idx + 1 ).x();

        if ( qAbs( x - x2 ) < qAbs( x - x1 ) )
            idx++;
    }

    return idx;
}

void Plot::onXScaleChanged()
{
    m_cursor->updateOverlay();
}

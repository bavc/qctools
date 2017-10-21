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
#include <qwt_symbol.h>
#include <QResizeEvent>
#include <qwt_point_mapper.h>
#include <qwt_painter.h>
#include <qwt_symbol.h>

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

static inline QRectF qwtIntersectedClipRect( const QRectF &rect, QPainter *painter )
{
    QRectF clipRect = rect;
    if ( painter->hasClipping() )
    {
#if QT_VERSION >= 0x040800
        const QRectF r = painter->clipBoundingRect();
#else
        const QRectF r = painter->clipRegion().boundingRect();
#endif
        clipRect &= r;
    }

    return clipRect;
}

class PlotSymbol : public QwtSymbol {
    // QwtSymbol interface
public:
    PlotSymbol() : QwtSymbol(QwtSymbol::Rect) {

    }
    QRect boundingRect() const {
        QRect rect = QwtSymbol::boundingRect();
        // rect.moveCenter(QPoint(0, -rect.top()));
        return rect;
    }
protected:
    void renderSymbols(QPainter *painter, const QPointF *points, int numPoints) const {
        const QSize size = this->size();

        QPen pen = this->pen();
        pen.setJoinStyle( Qt::MiterJoin );
        painter->setPen( Qt::black );

        for ( int i = 0; i < numPoints; i++ )
        {
            const QPointF &pos = points[i];

            const double x1 = pos.x() - 0.5 * size.width();
            const double y1 = pos.y(); //  - 0.5 * size.height();

            QwtPainter::drawRect(painter, x1, y1, size.width(), size.height());
        }
    }
};

class PlotCurve : public QwtPlotCurve {
    // QwtPlotCurve interface
public:
    explicit PlotCurve( const QString &title = QString::null ) : QwtPlotCurve(title), m_index(0), m_count(0) {

    }
    explicit PlotCurve( const QwtText &title ) : QwtPlotCurve(title), m_index(0), m_count(0) {

    }

    void setIndex(int index, int count) {
        m_index = index;
        m_count = count;
    }

protected:
    void drawSymbols(QPainter *p, const QwtSymbol &s, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const {

        QwtPointMapper mapper;
        mapper.setFlag( QwtPointMapper::RoundPoints,
            QwtPainter::roundingAlignment( p ) );
        mapper.setFlag( QwtPointMapper::WeedOutPoints,
            testPaintAttribute( QwtPlotCurve::FilterPoints ) );

        const QRectF clipRect = qwtIntersectedClipRect( canvasRect, p );
        mapper.setBoundingRect( clipRect );

        double booleanZero = double(m_index) / m_count;

        for ( int i = from; i <= to; ++i )
        {
            auto y = data()->sample(i).ry();
            if(!qFuzzyCompare(y, booleanZero))
            {
                const QPolygonF points = mapper.toPointsF(xMap, yMap,
                        data(), i, i);

                s.drawSymbols( p, points );
            }
        }
    }

private:
    int m_index;
    int m_count;
};

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

void Plot::setVisible(bool visible)
{
    if(isVisible() != visible)
    {
        QWidget::setVisible(visible);
        Q_EMIT visibilityChanged(visible);
    }
}

void Plot::initYAxis()
{
    if(m_boolean)
    {
        setYAxis(0.0, 1.0, 1);
    }
    else
    {
        const size_t plotType = type();
        const size_t plotGroup = group();

        CommonStats* stat = stats( streamPos() );
        const struct per_group& group = PerStreamType[plotType].PerGroup[plotGroup];

        double yMin = stat->y_Min[plotGroup];
        double yMax = stat->y_Max[plotGroup];

        if ( ( group.Min != group.Max ) && ( yMax - yMin >= ( group.Max - group.Min) / 2 ) )
            yMax = group.Max;

        if ( yMin != yMax )
        {
            setYAxis( yMin, yMax, group.StepsCount );
        }
        else
        {
            //Special case, in order to force a scale of 0 to 1
            setYAxis( 0.0, 1.0, 1 );
        }
    }
}

void Plot::updateSymbols()
{
    if(m_boolean)
    {
        for(auto curve : m_curves)
        {
            curve->setStyle(QwtPlotCurve::Dots);

            QwtSymbol *symbol = new PlotSymbol();
            symbol->setCachePolicy(QwtSymbol::NoCache);
            symbol->setBrush(curve->pen().color());
            symbol->setPen(Qt::transparent);

            QwtPointMapper mapper;
            mapper.setBoundingRect(this->canvas()->rect());

            int symbolWidth = 0;
            auto curveData = curve->data();
            if(curveData->size() > 1) {
                auto map = canvasMap(QwtPlot::xBottom);

                double transformed1 = map.transform(curve->data()->sample(0).x());
                double transformed2 = map.transform(curve->data()->sample(1).x());

                double dt = transformed2 - transformed1;

                symbolWidth = qRound(qRound(dt) * 0.9);
            }
            if(symbolWidth == 0)
                symbolWidth = 1;

            auto symbolHeight = int(double(canvas()->size().height()) / m_curves.size() * 0.8);

            symbol->setSize(QSize(symbolWidth, symbolHeight));
            curve->setSymbol( symbol );
        }
    }
    else
    {
        for(auto curve : m_curves)
        {
            curve->setStyle(QwtPlotCurve::Lines);
            curve->setSymbol(nullptr);
        }
    }
}

void Plot::setBoolean(bool value)
{
    if(m_boolean != value)
    {
        m_boolean = value;

        updateSymbols();
        initYAxis();
        replot();
    }
}

Plot::Plot( size_t streamPos, size_t Type, size_t Group, const FileInformation* fileInformation, QWidget *parent ) :
    QwtPlot( parent ),
    m_streamPos( streamPos ),
    m_type( Type ),
    m_group( Group ),
    m_fileInformation( fileInformation ),
    m_boolean (false)
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
    m_curves.reserve(PerStreamType[m_type].PerGroup[m_group].Count);

    for( unsigned j = 0; j < PerStreamType[m_type].PerGroup[m_group].Count; ++j )
    {
        PlotCurve* curve = new PlotCurve( PerStreamType[m_type].PerItem[PerStreamType[m_type].PerGroup[m_group].Start + j].Name );
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
        curve->setIndex(j, PerStreamType[m_type].PerGroup[m_group].Count);
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

const CommonStats *Plot::stats(size_t statsPos) const {
    if ( statsPos == (size_t)-1 )
        return m_fileInformation->ReferenceStat();
    else
        return m_fileInformation->Stats[statsPos];
}

CommonStats *Plot::stats(size_t statsPos) {
    if ( statsPos == (size_t)-1 )
        return m_fileInformation->ReferenceStat();
    else
        return m_fileInformation->Stats[statsPos];
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

void Plot::setData(size_t curveIndex, QwtSeriesData<QPointF> *series)
{
    m_curves[curveIndex]->setData(series);
}

const QwtSeriesData<QPointF>* Plot::getData(size_t curveIndex) const
{
    return m_curves[curveIndex]->data();
}

const QwtPlotCurve* Plot::getCurve(size_t curveIndex) const
{
    return m_curves[curveIndex];
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

static int indexLower( double x, const QwtSeriesData<QPointF> &data )
{
    int index = qwtUpperSampleIndex<QPointF>( data, x, compareX() );
    if ( index == -1 )
        index = data.size();

    return index - 1;
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

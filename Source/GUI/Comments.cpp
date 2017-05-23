#include <GUI/Comments.h>
#include <qwt_scale_widget.h>
#include <qwt_symbol.h>
#include <qwt_scale_div.h>
#include <qwt_plot_curve.h>
#include <qwt_painter.h>
#include <qwt_point_mapper.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_series_data.h>

static inline void qwtDrawRectSymbols( QPainter *painter,
    const QPointF *points, int numPoints, const QwtSymbol &symbol )
{
    const QSize size = symbol.size();

    QPen pen = symbol.pen();
    pen.setJoinStyle( Qt::MiterJoin );
    painter->setPen( pen );
    painter->setBrush( symbol.brush() );
    painter->setRenderHint( QPainter::Antialiasing, false );

    if ( QwtPainter::roundingAlignment( painter ) )
    {
        const int sw = size.width();
        const int sh = size.height();
        const int sw2 = size.width() / 2;
        const int sh2 = size.height() / 2;

        for ( int i = 0; i < numPoints; i++ )
        {
            const int x = qRound( points[i].x() );
            const int y = qRound( points[i].y() );

            if(y != 0)
            {
                const QRect r( x - sw2, 0, sw, sh);
                QwtPainter::drawRect( painter, r );
            }
        }
    }
    else
    {
        const double sw = size.width();
        const double sh = size.height();
        const double sw2 = 0.5 * size.width();
        const double sh2 = 0.5 * size.height();

        for ( int i = 0; i < numPoints; i++ )
        {
            const double x = points[i].x();
            const double y = points[i].y();

            const QRectF r( x - sw2, y - sh2, sw, sh );
            QwtPainter::drawRect( painter, r );
        }
    }
}

class CommentsSymbol : public QwtSymbol
{
    // QwtSymbol interface
public:
    CommentsSymbol() : QwtSymbol(QwtSymbol::UserStyle) {

    }

protected:
    void renderSymbols(QPainter *painter, const QPointF *points, int numPoints) const {
        qwtDrawRectSymbols(painter, points, numPoints, *this);
    }
};

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

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};

static int indexLower( double x, const QwtSeriesData<QPointF> &data )
{
    int index = qwtUpperSampleIndex<QPointF>( data, x, compareX() );
    if ( index == -1 )
        index = data.size();

    return index - 1;
}

class CommentsPlotCurve : public QwtPlotCurve {
    // QwtPlotCurve interface
public:
    int frameAt( double x ) const
    {
        const QwtSeriesData<QPointF>& seriesData = *data();

        int idx = ::indexLower( x, seriesData );
        if ( idx < 0 )
        {
            idx = 0;
        }
        else if ( idx < seriesData.size() - 1 )
        {
            // index, where x is closer
            const double x1 = seriesData.sample( idx ).x();
            const double x2 = seriesData.sample( idx + 1 ).x();

            if ( qAbs( x - x2 ) < qAbs( x - x1 ) )
                idx++;
        }

        return idx;
    }

protected:
    void drawSymbols(QPainter *painter, const QwtSymbol &symbol, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const {
        QwtPointMapper mapper;
        mapper.setFlag( QwtPointMapper::RoundPoints,
            QwtPainter::roundingAlignment( painter ) );
        mapper.setFlag( QwtPointMapper::WeedOutPoints,
            testPaintAttribute( QwtPlotCurve::FilterPoints ) );

        const QRectF clipRect = qwtIntersectedClipRect( canvasRect, painter );
        mapper.setBoundingRect( clipRect );

        for ( int i = from; i <= to; ++i )
        {
            if(!qFuzzyIsNull(data()->sample(i).ry()))
            {
                const QPolygonF points = mapper.toPointsF(xMap, yMap,
                        data(), i, i);

                symbol.drawSymbols( painter, points );
            }
        }
    }
private:

};

class CommentsPlotPicker: public QwtPlotPicker
{
public:
    CommentsPlotPicker(QWidget* w, CommonStats* stats) : QwtPlotPicker(w), stats(stats)
    {
        setAxis( QwtPlot::xBottom, QwtPlot::yLeft );
        setRubberBand( QwtPlotPicker::CrossRubberBand );
        setRubberBandPen( QColor( Qt::green ) );

        setTrackerMode( QwtPicker::AlwaysOn );
        setTrackerPen( QColor( Qt::black ) );

        setStateMachine( new QwtPickerDragPointMachine () );
    }

    const QwtPlotCurve* curve( int index ) const
    {
        const QwtPlotItemList curves = plot()->itemList( QwtPlotItem::Rtti_PlotCurve );
        if ( index >= 0 && index < curves.size() )
            return dynamic_cast<const QwtPlotCurve*>( curves[index] );

        return NULL;
    }

    virtual QwtText trackerTextF( const QPointF &pos ) const
    {
        // the white text is hard to see over a light canvas background

        QColor bg( Qt::darkGray );
        bg.setAlpha( 160 );

        QwtText text;

        const int idx = dynamic_cast<const CommentsPlotCurve*>( curve(0) )->frameAt( pos.x() );
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
        if(stats->comments[index])
            return QString::fromUtf8(stats->comments[index]);

        return "";
    }

private:
    CommonStats* stats;
};

QwtPlot* createCommentsPlot(FileInformation* fileInfo, const int* dataTypeIndex) {
    QwtPlot* plot = new QwtPlot;
    CommentsPlotPicker* picker = new CommentsPlotPicker(plot->canvas(), fileInfo->ReferenceStat());

    QwtPlotCurve *curve = new CommentsPlotCurve();
    curve->setPen( Qt::transparent, 4 ), curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );

    QwtPlotCanvas* canvas = dynamic_cast<QwtPlotCanvas*>( plot->canvas() );
    if ( canvas )
    {
        canvas->setFrameStyle( QFrame::Plain | QFrame::Panel );
        canvas->setLineWidth( 1 );
#if 1
        canvas->setPalette( QColor("Cornsilk") );
#endif
    }

    QwtSymbol *symbol = new CommentsSymbol;
    symbol->setBrush(QBrush(Qt::red));
    symbol->setPen( QPen( Qt::red, 2 ));
    symbol->setCachePolicy(QwtSymbol::NoCache);

    int plotHeight = 30;
    symbol->setSize(QSize(10, plotHeight));

    curve->setSymbol( symbol );

    curve->setData(new CommentsSeriesData(fileInfo->ReferenceStat(), dataTypeIndex));
    curve->attach( plot );

    plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    plot->setMinimumHeight(plotHeight);
    plot->setMaximumHeight(plotHeight);

    plot->setAxisAutoScale(QwtPlot::yLeft, false);
    plot->setAxisScale(QwtPlot::yLeft, 0, 1);

    plot->enableAxis(QwtPlot::xBottom, false);
    plot->setAxisAutoScale(QwtPlot::xBottom, true);

    plot->axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Backbone, false);
    plot->axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    plot->axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    QObject::connect(fileInfo, SIGNAL(commentsUpdated(CommonStats*)), plot, SLOT(replot()));

    plot->replot();

    return plot;
}

/*
QwtPlot* createNotesPlot(FileInformation* fileInfo, const int* dataTypeIndex) {

    QwtPlot* plot = new QwtPlot;
    QwtPlotBarChart* barchartPlot = new QwtPlotBarChart;
    barchartPlot->setLayoutPolicy(QwtPlotBarChart::FixedSampleSize);
    barchartPlot->attach(plot);
    barchartPlot->setData(new SeriesData(fileInfo->ReferenceStat(), dataTypeIndex));

    plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    plot->setMinimumHeight(60);
    plot->setMaximumHeight(60);

    plot->plotLayout()->setAlignCanvasToScales( false ); // this line removes weird spacing between yAxis and 'zero' on xAxis

    plot->enableAxis(QwtPlot::xBottom, true);
    plot->setAxisAutoScale(QwtPlot::xBottom, false);

    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisAutoScale(QwtPlot::yLeft, false);
    plot->setAxisScale(QwtPlot::yLeft, 0, 1);

    plot->axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    plot->axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    QObject::connect(fileInfo, SIGNAL(commentsUpdated(CommonStats*)), plot, SLOT(replot()));

    plot->replot();

    return plot;
}
*/

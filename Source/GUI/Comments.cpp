#include <GUI/Comments.h>
#include <GUI/Plot.h>

#include <qwt_scale_widget.h>
#include <qwt_symbol.h>
#include <qwt_scale_div.h>
#include <qwt_plot_curve.h>
#include <qwt_painter.h>
#include <qwt_point_mapper.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_series_data.h>
#include <qwt_plot_grid.h>

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

CommentsPlot* createCommentsPlot(FileInformation* fileInfo, const int* dataTypeIndex) {
    CommentsPlot* plot = new CommentsPlot(fileInfo, fileInfo->ReferenceStat(), dataTypeIndex);

    plot->replot();
    return plot;
}

CommentsPlot::CommentsPlot(FileInformation* fileInfo, CommonStats* stats, const int* dataTypeIndex) {

    setAxisMaxMajor( QwtPlot::yLeft, 0 );
    setAxisMaxMinor( QwtPlot::yLeft, 0 );

    // Plot grid
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableXMin( true );
    grid->enableYMin( true );
    grid->setMajorPen( Qt::darkGray, 0, Qt::DotLine );
    grid->setMinorPen( Qt::gray, 0 , Qt::DotLine );
    grid->attach( this );

    QwtPlotCanvas* canvas = dynamic_cast<QwtPlotCanvas*>(this->canvas() );
    if ( canvas )
    {
        canvas->setFrameStyle( QFrame::Plain | QFrame::Panel );
        canvas->setLineWidth( 1 );
#if 1
        canvas->setPalette( QColor("Cornsilk") );
#endif
    }

    QwtPlotCurve *curve = new CommentsPlotCurve();
    curve->setPen( Qt::transparent, 4 ), curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );

    QwtSymbol *symbol = new CommentsSymbol;
    symbol->setBrush(QBrush(Qt::red));
    symbol->setPen( QPen( Qt::red, 2 ));
    symbol->setCachePolicy(QwtSymbol::NoCache);

    int plotHeight = 30;
    symbol->setSize(QSize(10, plotHeight));

    curve->setSymbol( symbol );

    curve->setData(new CommentsSeriesData(stats, dataTypeIndex));
    curve->attach( this );

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(plotHeight);
    setMaximumHeight(plotHeight);

    setAxisAutoScale(QwtPlot::yLeft, false);
    setAxisScale(QwtPlot::yLeft, 0, 1);

    enableAxis(QwtPlot::xBottom, false);
    setAxisAutoScale(QwtPlot::xBottom, true);

    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Backbone, false);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    QObject::connect(fileInfo, SIGNAL(commentsUpdated(CommonStats*)), SLOT(replot()));

    m_cursor = new PlotCursor( canvas );
    m_cursor->setPosition( 0 );

    CommentsPlotPicker* picker = new CommentsPlotPicker(this->canvas(), stats);
    connect(picker, SIGNAL(moved(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));
    connect(picker, SIGNAL(selected(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));

    connect( axisWidget( QwtPlot::xBottom ), SIGNAL( scaleDivChanged() ), SLOT( onXScaleChanged() ) );

}

int CommentsPlot::frameAt(double x) const
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

const QwtPlotCurve *CommentsPlot::curve(int index) const
{
    const QwtPlotItemList curves = itemList( QwtPlotItem::Rtti_PlotCurve );
    if ( index >= 0 && index < curves.size() )
        return dynamic_cast<const QwtPlotCurve*>( curves[index] );

    return NULL;
}

void CommentsPlot::onPickerMoved(const QPointF & pos)
{
    const int idx = frameAt( pos.x() );
    if ( idx >= 0 )
        Q_EMIT cursorMoved( idx );
}

void CommentsPlot::onXScaleChanged()
{
    m_cursor->updateOverlay();
}

void CommentsPlot::setCursorPos(double x)
{
    m_cursor->setPosition( x );
}

CommentsPlotPicker::CommentsPlotPicker(QWidget *w, CommonStats *stats) : QwtPlotPicker(w), stats(stats)
{
    setAxis( QwtPlot::xBottom, QwtPlot::yLeft );
    setRubberBand( QwtPlotPicker::CrossRubberBand );
    setRubberBandPen( QColor( Qt::green ) );

    setTrackerMode( QwtPicker::AlwaysOn );
    setTrackerPen( QColor( Qt::black ) );

    setStateMachine( new QwtPickerDragPointMachine () );
}

const QwtPlotCurve *CommentsPlotPicker::curve(int index) const
{
    const QwtPlotItemList curves = plot()->itemList( QwtPlotItem::Rtti_PlotCurve );
    if ( index >= 0 && index < curves.size() )
        return dynamic_cast<const QwtPlotCurve*>( curves[index] );

    return NULL;
}

QwtText CommentsPlotPicker::trackerTextF(const QPointF &pos) const
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

QString CommentsPlotPicker::infoText(int index) const
{
    if(stats->comments[index])
        return QString::fromUtf8(stats->comments[index]);

    return "";
}

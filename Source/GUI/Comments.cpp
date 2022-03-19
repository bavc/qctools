#include <GUI/Comments.h>
#include <GUI/Plot.h>
#include <GUI/PlotLegend.h>

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
#include <QTextDocument>

constexpr int plotHeight = 30;

static inline void qwtDrawRhombSymbols( QPainter *painter,
    const QPointF *points, int numPoints, const QwtSymbol &symbol )
{
    const QSize size = symbol.size();

    QPen pen = symbol.pen();
    pen.setJoinStyle( Qt::MiterJoin );
    painter->setPen( Qt::black );
    painter->setBrush( symbol.brush() );

    if ( QwtPainter::roundingAlignment( painter ) )
    {
        for ( int i = 0; i < numPoints; i++ )
        {
            const int x = qRound( points[i].x() );
            const int y = size.height() / 2;

            const int x1 = x - size.width() / 2;
            const int y1 = 0;
            const int x2 = x1 + size.width();
            const int y2 = size.height();

            QPolygonF polygon;
            polygon += QPointF( x, y1 );
            polygon += QPointF( x1, y );
            polygon += QPointF( x, y2 );
            polygon += QPointF( x2, y );

            QwtPainter::drawPolygon( painter, polygon );
        }
    }
    else
    {
        for ( int i = 0; i < numPoints; i++ )
        {
            const QPointF &pos = points[i];

            const double x1 = pos.x() - 0.5 * size.width();
            const double y1 = pos.y() - 0.5 * size.height();
            const double x2 = x1 + size.width();
            const double y2 = y1 + size.height();

            QPolygonF polygon;
            polygon += QPointF( pos.x(), y1 );
            polygon += QPointF( x2, pos.y() );
            polygon += QPointF( pos.x(), y2 );
            polygon += QPointF( x1, pos.y() );

            QwtPainter::drawPolygon( painter, polygon );
        }
    }
}

class CommentsSymbol : public QwtSymbol
{
    // QwtSymbol interface
public:
    CommentsSymbol() : QwtSymbol(QwtSymbol::Diamond) {
    }

protected:
    void renderSymbols(QPainter *painter, const QPointF *points, int numPoints) const {
        qwtDrawRhombSymbols(painter, points, numPoints, *this);
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
    curve->setTitle("Comments");
    curve->setStyle(QwtPlotCurve::Dots);
    curve->setPen( Qt::red, 0);
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );

    QwtSymbol *symbol = new CommentsSymbol;
    symbol->setBrush(QBrush(Qt::red));
    symbol->setPen( QPen( Qt::red, 1 ));
    symbol->setCachePolicy(QwtSymbol::NoCache);

    symbol->setSize(QSize(11, plotHeight));

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

    axisWidget(QwtPlot::xBottom)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Backbone, false);
    axisWidget(QwtPlot::xBottom)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    axisWidget(QwtPlot::xBottom)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Backbone, false);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    axisWidget(QwtPlot::yLeft)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Backbone, false);
    axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Labels, false);
    axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtAbstractScaleDraw::Ticks, false);

    QObject::connect(fileInfo, SIGNAL(commentsUpdated(CommonStats*)), SLOT(replot()));

    m_cursor = new PlotCursor( canvas );
    m_cursor->setPosition( 0 );

    CommentsPlotPicker* picker = new CommentsPlotPicker(this->canvas(), stats);
    connect(picker, SIGNAL(moved(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));
    connect(picker, SIGNAL(selected(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));

    connect( axisWidget( QwtPlot::xBottom ), SIGNAL( scaleDivChanged() ), SLOT( onXScaleChanged() ) );

    m_plotLegend = new PlotLegend();
    m_plotLegend->setMaximumHeight(plotHeight);

    connect( this, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
         m_plotLegend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

    updateLegend();
}

CommentsPlot::~CommentsPlot()
{
    delete m_legend;
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

void CommentsPlot::restorePlotHeight()
{
    setMinimumHeight(plotHeight);
    setMaximumHeight(plotHeight);
    resize(size().width(), plotHeight);
}

CommentsPlotPicker::CommentsPlotPicker(QWidget *w, CommonStats *stats) : QwtPlotPicker(w), stats(stats)
{
#if QWT_VERSION >= 0x060200
    setAxes( QwtPlot::xBottom, QwtPlot::yLeft );
#else
    setAxis( QwtPlot::xBottom, QwtPlot::yLeft );
#endif
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
        QTextDocument doc;
        doc.setHtml(infoText(idx));

        text = doc.toPlainText();
        text.setBackgroundBrush( QBrush( bg ) );
    }

    return text;
}

QString CommentsPlotPicker::infoText(int index) const
{
    if(stats->comments && stats->comments[index])
        return QString::fromUtf8(stats->comments[index]);

    return "";
}

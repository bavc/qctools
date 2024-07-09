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
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_series_data.h>
#include <qwt_symbol.h>
#include <QResizeEvent>
#include <qwt_point_mapper.h>
#include <qwt_painter.h>
#include <qwt_symbol.h>
#include <qwt_clipper.h>

#include "Core/FileInformation.h"
#include <QMetaEnum>
#include <QSettings>
#include <cassert>
#include <optional>

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
    PlotPicker( Plot *plot, const struct stream_info* streamInfo, const size_t group, const QVector<QwtPlotCurve*>* curves, const FileInformation* fileInformation):
        m_plot(plot),
        m_streamInfo( streamInfo ),
        m_group( group ),
        m_curves( curves ),
        QwtPlotPicker(plot->canvas()),
        m_fileInformation(fileInformation)
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

    virtual QwtText trackerTextF( const QPointF &pos ) const
    {
        // the white text is hard to see over a light canvas background

        QColor bg( Qt::darkGray );
        bg.setAlpha( 160 );

        QwtText text;

        const int idx = dynamic_cast<const Plot*>( plot() )->frameAt( pos.x() );
        if ( idx >= 0 )
        {
            text = QwtText(infoText( text.font(), idx ), QwtText::RichText);
            text.setBackgroundBrush( QBrush( bg ) );
        }

        return text;
    }

    virtual QRect trackerRect( const QFont & font) const {
        QRect rect = QwtPlotPicker::trackerRect(font);

        return rect;
    }

    virtual void drawTracker( QPainter *painter ) const
    {
        QRect rect = trackerRect(painter->font());

        QwtPlotPicker::drawTracker(painter);
    }

protected:
    virtual QString infoText(const QFont& font, int index ) const
    {
        QFontMetrics metrics(font);
        auto fontHeight = metrics.height();

        QString info = QString( "Frame %1 [%2]" ).arg(index).arg(m_fileInformation ? m_fileInformation->Frame_Type_Get(-1, index) : "");
        for( unsigned i = 0; i < m_streamInfo->PerGroup[m_group].Count; ++i )
        {
            const per_item &itemInfo = m_streamInfo->PerItem[m_streamInfo->PerGroup[m_group].Start + i];

            info += i?", ":": ";
            info += itemInfo.Name;
            info += "=";

            if ((*m_curves)[i]->dataSize() != 0)
                info += QString::number(static_cast<PlotSeriesData*>((*m_curves)[i]->data())->originalSample(index).ry(), 'f', itemInfo.DigitsAfterComma);
            else
                info += QString("n/a");
        }

        if(m_plot->isBarchart())
        {
            for( unsigned i = 0; i < m_streamInfo->PerGroup[m_group].Count; ++i )
            {
                auto curve = (*m_curves)[i];
                auto sample = static_cast<PlotSeriesData*>(curve->data())->sample(index);
                Q_UNUSED(sample);

                const per_item &itemInfo = m_streamInfo->PerItem[m_streamInfo->PerGroup[m_group].Start + i];
                auto curveData = static_cast<PlotSeriesData*>((*m_curves)[i]->data());
                if(curveData->getLastCondition()) {
                    info += QString("\n<table><tr><td width=%1 bgcolor=%2>&nbsp;</td><td>&nbsp;-&nbsp;%3</td><td>(%4)</td</tr></table>")
                            .arg(fontHeight)
                            .arg(curveData->getLastCondition()->m_color.name())
                            .arg(curveData->getLastCondition()->m_label)
                            .arg(curve->title().text());
                }
            }

        }

        return info;
    }

    const Plot*                     m_plot;
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
    explicit PlotCurve( const QString &title = QString() ) : QwtPlotCurve(title), m_index(0), m_count(0) {

    }
    explicit PlotCurve( const QwtText &title ) : QwtPlotCurve(title), m_index(0), m_count(0) {

    }

    void setIndex(int index, int count) {
        m_index = index;
        m_count = count;
    }

    void setFillCurve(QwtPlotCurve* curve) {
        m_fillCurve = curve;
    }

    void setFillBaseLine(float value) {
        m_fillBaseline = value;
    }

    void setFillBrush(QBrush brush) {
        m_fillBrush = brush;
    }

protected:
    virtual void drawCurve( QPainter* painter , int style, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to ) const {
        QwtPlotCurve::drawCurve(painter, style, xMap, yMap, canvasRect, from, to);

        if((m_fillBaseline || m_fillCurve) && !symbol())
        {
            auto brush = m_fillBrush;

            if ( brush.style() == Qt::NoBrush )
                return;

            const bool doFit = ( testCurveAttribute( Fitted ) && curveFitter() );
            const bool doAlign = !doFit && QwtPainter::roundingAlignment( painter );

            QwtPointMapper mapper;

            if ( doAlign )
            {
                mapper.setFlag( QwtPointMapper::RoundPoints, true );
                mapper.setFlag( QwtPointMapper::WeedOutIntermediatePoints,
                               testPaintAttribute( FilterPointsAggressive ) );
            }

            mapper.setFlag( QwtPointMapper::WeedOutPoints,
                           testPaintAttribute( FilterPoints ) ||
                               testPaintAttribute( FilterPointsAggressive ) );

            mapper.setBoundingRect( canvasRect );
            QPolygonF polygon = mapper.toPolygonF( xMap, yMap, data(), from, to);
            QPolygonF baselinePolygon;
            if(m_fillCurve)
                baselinePolygon = mapper.toPolygonF( xMap, yMap, m_fillCurve->data(), from, to);
            else if(m_fillBaseline) {
                const PlotSeriesData* plotSeriesData = static_cast<const PlotSeriesData*>(data());
                auto fromSample = plotSeriesData->sample(from);
                auto toSample = plotSeriesData->sample(to);

                baselinePolygon += QPointF(xMap.transform(qreal(fromSample.x())), yMap.transform(m_fillBaseline.value()));
                baselinePolygon += QPointF(xMap.transform(qreal(toSample.x())), yMap.transform(m_fillBaseline.value()));
            }

            for(auto it = baselinePolygon.rbegin(); it != baselinePolygon.rend(); ++it) {
                polygon += *it;
            }

            if ( polygon.count() <= 2 ) // a line can't be filled
                return;

            if ( !brush.color().isValid() )
                brush.setColor( this->pen().color() );

            if ( testPaintAttribute(ClipPolygons) )
            {
                const QRectF clipRect = qwtIntersectedClipRect( canvasRect, painter );
                QwtClipper::clipPolygonF( clipRect, polygon, true );
            }

            painter->save();

            painter->setPen( Qt::NoPen );
            painter->setBrush( brush );

            QwtPainter::drawPolygon( painter, polygon );

            painter->restore();
        }
    }

    virtual void fillCurve( QPainter *painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, QPolygonF& polygon) const {
        QwtPlotCurve::fillCurve(painter, xMap, yMap, canvasRect, polygon);
    }

    virtual void drawLines(QPainter* p, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to) const {
        QwtPlotCurve::drawLines(p, xMap, yMap, canvasRect, from, to);
    }

    void drawSymbols(QPainter *p, const QwtSymbol &s, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const {

        QwtPointMapper mapper;
        mapper.setFlag( QwtPointMapper::RoundPoints,
            QwtPainter::roundingAlignment( p ) );
        mapper.setFlag( QwtPointMapper::WeedOutPoints,
            testPaintAttribute( QwtPlotCurve::FilterPoints ) );

        const QRectF clipRect = qwtIntersectedClipRect( canvasRect, p );
        mapper.setBoundingRect( clipRect );

        double barchartZero = double(m_index) / m_count;
        p->setPen(Qt::NoPen);

        for ( int i = from; i <= to; ++i )
        {
            auto y = data()->sample(i).ry();
            if(!qFuzzyCompare(y, barchartZero))
            {
                const QPolygonF points = mapper.toPointsF(xMap, yMap,
                        data(), i, i);

                const PlotSeriesData* plotSeriesData = static_cast<const PlotSeriesData*>(data());
                const PlotSeriesData::Condition* lastCondition = plotSeriesData->getLastCondition();

                if(lastCondition)
                    p->setBrush(lastCondition->m_color);

                s.drawSymbols( p, points );
            }
        }
    }

private:
    int m_index;
    int m_count;
    QBrush m_fillBrush;
    QwtPlotCurve* m_fillCurve { nullptr };
    std::optional<float> m_fillBaseline;
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
    if(m_barchart)
    {
        setYAxis(0.0, 1.0, 1);

        auto streamInfo = PerStreamType[m_type];
        for(size_t j = 0; j < streamInfo.PerGroup[m_group].Count; ++j)
        {
            PlotSeriesData* data = getData(j);
            data->mutableConditions().updateAll(m_fileInformation->BitsPerRawSample());
        }
    }
    else
    {
        auto yMin = 0.0;
        auto yMax = 0.0;
        const size_t plotType = type();
        const size_t plotGroup = group();

        CommonStats* stat = stats( streamPos() );
        const struct per_group& group = PerStreamType[plotType].PerGroup[plotGroup];

        if(m_yminMaxMode == Formula)
        {
            if(m_minValue.isNumber())
                yMin = m_minValue.toNumber();
            else if(m_minValue.isCallable())
                yMin = m_minValue.call().toNumber();

            if(m_maxValue.isNumber())
                yMax = m_maxValue.toNumber();
            else if(m_maxValue.isCallable())
                yMax = m_maxValue.call().toNumber();
        }
        else if(m_yminMaxMode == MinMaxOfThePlot)
        {
            yMin = stat->y_Min[plotGroup]; // auto-select min
            yMax = stat->y_Max[plotGroup]; // auto-select max
        } else if(m_yminMaxMode == Custom)
        {
            yMin = m_customYMin;
            yMax = m_customYMax;
        }

        setYAxis( yMin, yMax, group.StepsCount );
    }
}

void Plot::updateSymbols()
{
    if(m_barchart)
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

                symbolWidth = qRound(dt * 1.1f); // for some reasons qwt add some spacing between samples so * 1.1 is just workaround for it
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

bool Plot::isBarchart() const
{
    return m_barchart;
}

void Plot::loadYAxisMinMaxMode()
{
    const struct per_group& group = PerStreamType[m_type].PerGroup[m_group];

    auto applyYMinMaxMode = [&](QString value) {
        QMetaEnum metaEnum = QMetaEnum::fromType<Plot::YMinMaxMode>();
        auto splitted = value.split(";");
        auto yMinMaxMode = (Plot::YMinMaxMode) metaEnum.keyToValue(splitted[0].toLatin1().constData());

        if(yMinMaxMode == Plot::Custom) {
            auto min = splitted[1].toDouble();
            auto max = splitted[2].toDouble();

            setYAxisCustomMinMax(min, max);
        }

        setYAxisMinMaxMode(yMinMaxMode);
    };

    if(group.YAxisMinMaxMode) {
        QString yMinMaxModeStringValue = group.YAxisMinMaxMode;
        qDebug() << "applying default yMinMaxMode: " << yMinMaxModeStringValue;
        applyYMinMaxMode(yMinMaxModeStringValue);
    }

    QSettings settings;
    settings.beginGroup("yminmax");

    QString value = settings.value(QString::number(m_group)).toString();
    if(!value.isEmpty()) {
        qDebug() << "applying yMinMaxMode from settings: " << value;
        applyYMinMaxMode(value);
    }

    settings.endGroup();
}

void Plot::setYAxisMinMaxMode(YMinMaxMode mode)
{
    m_yminMaxMode = mode;
    QColor color;
    switch(m_yminMaxMode) {
    case MinMaxOfThePlot:
        color = "darkblue";
        break;
    case Formula:
        color = "black";
        break;
    case Custom:
        color = QColor(85, 0, 127);
        break;
    }

    setYAxisColor(color);
    initYAxis();
}

Plot::YMinMaxMode Plot::yAxisMinMaxMode() const
{
    return m_yminMaxMode;
}

void Plot::setYAxisCustomMinMax(double min, double max)
{
    m_customYMin = min;
    m_customYMax = max;
}

void Plot::getYAxisCustomMinMax(double &min, double &max)
{
    min = m_customYMin;
    max = m_customYMax;
}

bool Plot::hasMinMaxFormula() const
{
    if(m_minValue.isNull() || m_minValue.isError() || m_minValue.isUndefined()) {
        return false;
    }

    if(m_maxValue.isNull() || m_maxValue.isError() || m_maxValue.isUndefined()) {
        return false;
    }

    return true;
}

const CommonStats *Plot::getStats() const
{
    return stats( streamPos() );
}

void Plot::setBarchart(bool value)
{
    if(m_barchart != value)
    {
        m_barchart = value;

        canvas()->setPalette(m_barchart ? m_barchartBackground : m_charBackground);

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
    m_barchart (false),
    m_charBackground(QColor("Cornsilk"))
{
    int h = m_charBackground.hue();
    int s = m_charBackground.saturation();
    int v = m_charBackground.value();

    const struct per_group& group = PerStreamType[m_type].PerGroup[m_group];
    auto bitsPerRawSample = m_fileInformation->BitsPerRawSample(type());

    m_engine.globalObject().setProperty("bitsPerRawSample", bitsPerRawSample);
    m_engine.globalObject().setProperty("two_pow_bitsPerRawSample_minus_one", (1 << bitsPerRawSample) - 1);
    m_engine.globalObject().setProperty("sqrt_pow_bitsPerRawSample_2", sqrt(2) * (1 << bitsPerRawSample) / 2);

    if(m_type == Type_Audio) {
        auto ranges = m_fileInformation->audioRanges();
        m_engine.globalObject().setProperty("audio_min", ranges.first);
        m_engine.globalObject().setProperty("audio_max", ranges.second);
    }

    m_minValue = m_engine.evaluate(group.MinFormula == nullptr ? "" : group.MinFormula);
    m_maxValue = m_engine.evaluate(group.MaxFormula == nullptr ? "" : group.MaxFormula);

    m_barchartBackground = QColor::fromHsv(h + 60, s, v);

    setAutoReplot( false );

    QwtPlotCanvas* canvas = dynamic_cast<QwtPlotCanvas*>( this->canvas() );
    if ( canvas )
    {
        canvas->setFrameStyle( QFrame::Plain | QFrame::Panel );
        canvas->setLineWidth( 1 );
        canvas->setPalette(m_charBackground);
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
    QMap<QString, PlotCurve*> curvesByName;

    for( unsigned j = 0; j < PerStreamType[m_type].PerGroup[m_group].Count; ++j )
    {
        auto item = PerStreamType[m_type].PerItem[PerStreamType[m_type].PerGroup[m_group].Start + j];
        PlotCurve* curve = new PlotCurve( item.Name );
        QColor color = curveColor(j);
        int thickness = 0;

        switch (m_group)
        {
            case Group_IDET_S :
            case Group_IDET_M :
            case Group_IDET_R :
                thickness = 2;
                break;
        }

        if(item.color && *item.color)
            color = QColor(item.color);
        if(item.thickness > -1)
            thickness = item.thickness;

        qDebug() << "setting color for " << PerStreamType[m_type].PerGroup[m_group].Name << "/" << item.Name << ": color = " << color << ", thickness = " << thickness;
        curve->setPen(color, thickness);
        curve->setRenderHint( QwtPlotItem::RenderAntialiased );
        curve->setZ( curve->z() - j ); //Invert data order (e.g. MAX before MIN)
        curve->setIndex(j, PerStreamType[m_type].PerGroup[m_group].Count);
        curve->attach( this );

        m_curves += curve;

        curvesByName[item.Name] = curve;
    }

    for( unsigned j = 0; j < PerStreamType[m_type].PerGroup[m_group].Count; ++j )
    {
        auto item = PerStreamType[m_type].PerItem[PerStreamType[m_type].PerGroup[m_group].Start + j];
        if(item.fillInfo) {
            auto splitted = QString(item.fillInfo).split(";");
            auto curveName = splitted[0];
            auto color = QColor(splitted[1]);
            auto alpha = QString(splitted[2]).toFloat();

            color.setAlphaF(alpha);
            auto curve = curvesByName[item.Name];
            curve->setFillBrush(QBrush(color));

            if(curvesByName.contains(curveName)) {
                auto fillCurve = curvesByName[curveName];
                curve->setFillCurve(fillCurve);
            } else {
                auto fillValue = curveName.toFloat();
                curve->setFillBaseLine(fillValue);
            }
        }
    }

    PlotPicker* picker = new PlotPicker( this, &PerStreamType[m_type], m_group, &m_curves, fileInformation );
    connect( picker, SIGNAL( moved( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );
    connect( picker, SIGNAL( selected( const QPointF& ) ), SLOT( onPickerMoved( const QPointF& ) ) );

    connect( axisWidget( QwtPlot::xBottom ), SIGNAL( scaleDivChanged() ), SLOT( onXScaleChanged() ) );

    // legend
    m_plotLegend = new PlotLegend();
    
    connect( this, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
         m_plotLegend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );

    updateLegend();

    // Setting Magnifier
    QwtPlotMagnifier* zoom_x = new QwtPlotMagnifier(this->canvas());
    QwtPlotMagnifier* zoom_y = new QwtPlotMagnifier(this->canvas());

    // Shift+MouseWheel --> Magnifier x
    zoom_x->setWheelModifiers(Qt::ShiftModifier);
    zoom_x->setAxisEnabled(QwtPlot::xBottom, true);
    zoom_x->setAxisEnabled(QwtPlot::yLeft, false);

    // CTRL + MouseWheel --> Magnifier y
    zoom_y->setWheelModifiers(Qt::ControlModifier);
    zoom_y->setAxisEnabled(QwtPlot::xBottom,false);
    zoom_y->setAxisEnabled(QwtPlot::yLeft,true);

    class CustomPanner: public QwtPlotPanner
    {

    public:
        explicit CustomPanner(QWidget* parent) : QwtPlotPanner(parent){
            connect(this, &CustomPanner::moved, [&](int dx, int dy) {
                if(panning) {
                    Q_EMIT panned(dx - lastMovePos.x(), dy - lastMovePos.y());
                    lastMovePos = QPoint(dx, dy);
                }
            });
        }

    private:
        bool panning;
        QPoint lastMovePos;

        virtual void paintEvent(QPaintEvent *) {
        }

        virtual bool eventFilter( QObject * object, QEvent * event)
        {
            if ( object == NULL || object != parentWidget() )
                    return false;

            switch ( event->type() )
            {
                case QEvent::MouseButtonPress:
                {
                    auto mousePress = static_cast<QMouseEvent *>(event);
                    widgetMousePressEvent(mousePress);
                    lastMovePos = QPoint(0, 0);

                    Qt::MouseButton button;
                    Qt::KeyboardModifiers modifiers;

                    getMouseButton(button, modifiers);
                    if(mousePress->button() == button && mousePress->modifiers() == modifiers) {
                        panning = true;
                    }
                    break;
                }
                case QEvent::MouseMove:
                {
                    QMouseEvent * evr = static_cast<QMouseEvent *>( event );
                    widgetMouseMoveEvent( evr );
                    break;
                }
                case QEvent::MouseButtonRelease:
                {
                    panning = false;
                    break;
                }
                default:;
            }

            return false;
        }
    };

    QwtPlotPanner *panner = new CustomPanner( this->canvas() );
    panner->setOrientations(Qt::Vertical);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    panner->setMouseButton( Qt::MidButton );
#else
    panner->setMouseButton( Qt::MiddleButton );
#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

    loadYAxisMinMaxMode();
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

void Plot::setYAxisColor(QColor color)
{
    auto yAxis = axisWidget(QwtPlot::yLeft);
    QPalette palette = yAxis->palette();
    palette.setColor(QPalette::WindowText, color);	// for ticks
    palette.setColor(QPalette::Text, color); // for ticks' labels
    yAxis->setPalette(palette);
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

void Plot::setData(size_t curveIndex, PlotSeriesData *series)
{
    m_curves[curveIndex]->setData(series);
}

const PlotSeriesData* Plot::getData(size_t curveIndex) const
{
    return static_cast<PlotSeriesData*> (m_curves[curveIndex]->data());
}

PlotSeriesData *Plot::getData(size_t curveIndex)
{
    return static_cast<PlotSeriesData*> (m_curves[curveIndex]->data());
}

const QwtPlotCurve* Plot::getCurve(size_t curveIndex) const
{
    return m_curves[curveIndex];
}

int Plot::curvesCount() const
{
    return m_curves.size();
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
        Q_EMIT cursorMoved(pos, idx);
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

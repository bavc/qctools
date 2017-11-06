/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plot_H
#define GUI_Plot_H
//---------------------------------------------------------------------------

#include <Core/CommonStats.h>
#include <Core/Core.h>
#include "Core/VideoCore.h"
#include <QEvent>
#include <QJSEngine>
#include <QJSValue>
#include <QResizeEvent>
#include <qwt_plot.h>
#include <qwt_series_data.h>
#include <qwt_widget_overlay.h>
#include <cassert>

class QwtPlotCurve;
class PlotCursor;
class PlotLegend;
class FileInformation;
class QCheckBox;

//***************************************************************************
// Class
//***************************************************************************

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

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};

class PlotSeriesData : public QObject, public QwtPointSeriesData
{
    // QwtSeriesData interface
    Q_OBJECT
public:
    PlotSeriesData(CommonStats* stats, const QString& title, int bitDepth, const int& xDataIndex, const size_t yDataIndex, size_t plotGroup, size_t curveIndex, size_t curvesCount)
        : m_boolean(false), m_conditions(stats, this, plotGroup, title, bitDepth), m_lastCondition(nullptr),
          m_stats(stats), m_xDataIndex(xDataIndex), m_yDataIndex(yDataIndex), m_plotGroup(plotGroup), m_curveIndex(curveIndex),
          m_curvesCount(curvesCount)
    {

    }

    size_t size() const {
        return m_stats->x_Current;
    }
    QPointF sample(size_t i) const {

        auto xData = m_stats->x[m_xDataIndex];
        auto yData = m_stats->y[m_yDataIndex];

        return QPointF(xData[i], (m_boolean ? toBoolean(yData[i], 1.0) : yData[i]));
    }

    double toBoolean(double y) const {
        for(auto i = 0; i < m_conditions.m_items.size(); ++i) {
            const auto& condition = m_conditions.m_items[i];

            if(condition.match(y)) {
                m_lastCondition = &condition;
                return 1.0;
            }
        }

        return 0.0;
    }

    double toBoolean(double y, double globalMax) const {
        auto value = toBoolean(y);

        auto min = globalMax * (m_curveIndex) / m_curvesCount;
        auto max = globalMax * (m_curveIndex + 1) / m_curvesCount;

        auto adjustedValue = min + value * (max - min) * 0.9;

        return adjustedValue;
    }

    struct Condition
    {
        Condition() : m_engine(nullptr) {
        }

        Condition(QJSEngine* engine, CommonStats* stats, size_t plotGroup) : m_engine(engine), m_stats(stats), m_plotGroup(plotGroup) {
        }

        Condition(const Condition& other) = default;
        Condition(Condition&& other) = default;
        Condition& operator=(const Condition&) = default;

        QJSEngine* m_engine;

        CommonStats* m_stats;
        size_t m_plotGroup;

        QColor m_color;
        QString m_conditionString;
        mutable QJSValue m_conditionFunction;

        bool match(double y) const {

            if(m_conditionFunction.isCallable() && m_conditionFunction.call(QJSValueList() << y).toBool())
                return true;

            return false;
        }

        static QJSValue makeConditionFunction(QJSEngine* engine, const QString& condition) {
            return engine->evaluate(QString("function(y) { return %1; }").arg(condition));
        }

        QJSValue makeConditionFunction(const QString& condition) const {
            return makeConditionFunction(m_engine, condition);
        }

        void update() {
            if(m_conditionString.isEmpty())
                m_conditionFunction = QJSValue();
            else
                m_conditionFunction = makeConditionFunction(m_conditionString);
        }
    };

    struct Conditions
    {
        Conditions(CommonStats* stats, PlotSeriesData* seriesData, size_t plotGroup, const QString& title, int bitdepth) : m_stats(stats), m_seriesData(seriesData), m_plotGroup(plotGroup), m_curveTitle(title), m_bitdepth(bitdepth) {

        }

        void add() {
            m_items.append(Condition(&m_engine, m_stats, m_plotGroup));
            m_items.back().update();
        }

        void remove() {
            m_items.removeLast();
        }

        void update(int i, const QString& conditionString, const QColor& color) {
            auto & condition = m_items[i];
            qDebug() << "updateCondition: " << i << ", string = " << conditionString << ", color = " << color;

            condition.m_conditionString = conditionString;
            condition.m_color = color;
            condition.update();
        }

        void updateAll(int bitdepth)
        {
            QList<QPair<QString, QString>> autocomplete;
            autocomplete << QPair<QString, QString>("y", "y value of chart");

            auto & engine = m_engine;
            auto plotGroup = m_plotGroup;

            engine.globalObject().setProperty("yHalf", (m_stats->y_Max[m_plotGroup] - m_stats->y_Min[m_plotGroup]) / 2);
            autocomplete << QPair<QString, QString>("yHalf", QString("(plot max - plot min) / 2 ({%1})").arg(engine.globalObject().property("yHalf").toInt()));

            auto pow2 = engine.evaluate("function(value) { return Math.pow(value, 2); }");
            engine.globalObject().setProperty("pow2", pow2);
            autocomplete << QPair<QString, QString>("pow2", "pow2(exponent)");

            auto pow = engine.evaluate("function(base, exponent) { return Math.pow(base, exponent); }");
            engine.globalObject().setProperty("pow", pow);
            autocomplete << QPair<QString, QString>("pow", "pow(base, exponent)");

            if(bitdepth == 0)
            {
                qWarning("bitdepth is 0, assuming 8...");
                bitdepth = 8;
            }

            m_bitdepth = bitdepth;

            if(plotGroup == Group_Y || plotGroup == Group_U || plotGroup == Group_V || plotGroup == Group_YDiff || plotGroup == Group_UDiff || plotGroup == Group_VDiff)
            {
                engine.globalObject().setProperty("maxval", ::pow(2, bitdepth));
                autocomplete << QPair<QString, QString>("maxval", QString("2^bitdepth - %1").arg(engine.globalObject().property("maxval").toInt()));

                engine.globalObject().setProperty("minval", 0);
                autocomplete << QPair<QString, QString>("minval", QString("0"));

                if(plotGroup == Group_Y || plotGroup == Group_YDiff)
                {
                    engine.globalObject().setProperty("broadcastmaxval", 235 * (::pow(2, bitdepth - 8)));
                    autocomplete << QPair<QString, QString>("broadcastmaxval", QString("235 * (2^(bitdepth - 8)) - %1").arg(engine.globalObject().property("broadcastmaxval").toInt()));

                } else if(plotGroup == Group_U || plotGroup == Group_UDiff || plotGroup == Group_V || plotGroup == Group_VDiff)
                {
                    engine.globalObject().setProperty("broadcastmaxval", 240 * (::pow(2, bitdepth - 8)));
                    autocomplete << QPair<QString, QString>("broadcastmaxval", QString("240 * (2^(bitdepth - 8)) - %1").arg(engine.globalObject().property("broadcastmaxval").toInt()));
                }

                engine.globalObject().setProperty("broadcastminval", 16 * (::pow(2, bitdepth - 8)));
                autocomplete << QPair<QString, QString>("broadcastminval", QString("16 * (2^(bitdepth - 8)) - %1").arg(engine.globalObject().property("broadcastminval").toInt()));
            }

            engine.setProperty("autocomplete", QVariant::fromValue(autocomplete));

            for(auto & condition : m_items)
            {
                condition.update();
            }

            Q_EMIT m_seriesData->conditionsUpdated();
        }

        mutable QJSEngine m_engine;
        QVector<Condition> m_items;

        CommonStats* m_stats;
        PlotSeriesData* m_seriesData;

        size_t m_plotGroup;
        QString m_curveTitle;
        int m_bitdepth;
    };

    const Conditions& conditions() const{
        return m_conditions;
    }

    Conditions& mutableConditions() {
        return m_conditions;
    }

    const Condition* getLastCondition() const {
        return m_lastCondition;
    }

public Q_SLOTS:
    void setBoolean(bool enable) {
        qDebug() << "boolean mode: " << enable;
        m_boolean = enable;
        m_conditions.updateAll(m_conditions.m_bitdepth);
    }

Q_SIGNALS:
    void conditionsUpdated();

private:
    bool m_boolean;
    Conditions m_conditions;
    mutable const Condition* m_lastCondition;
    CommonStats* m_stats;
    const int& m_xDataIndex;
    const size_t m_yDataIndex;
    size_t m_plotGroup;
    size_t m_curveIndex;
    size_t m_curvesCount;
};

class Plot : public QwtPlot
{
    Q_OBJECT

public:
    explicit Plot( size_t streamPos, size_t Type, size_t Group, const FileInformation* fileInformation, QWidget *parent );
    virtual ~Plot();

    const CommonStats*          stats( size_t statsPos = (size_t)-1 ) const;
    CommonStats*                stats( size_t statsPos = (size_t)-1 );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setYAxis( double min, double max, int numSteps );
    void setCursorPos( double x );

    void setData(size_t curveIndex, PlotSeriesData *series);
    const QwtSeriesData<QPointF>* getData(size_t curveIndex) const;
    const QwtPlotCurve* getCurve(size_t curveIndex) const;

    size_t streamPos() const { return m_streamPos; }
    size_t type() const { return m_type; }
    size_t group() const { return m_group; }

    PlotLegend *legend() { return m_legend; }

    int frameAt( double x ) const;

    void addGuidelines(int bitsPerRawSample);
    virtual void setVisible(bool visible) override;

    void updateSymbols();
    bool isBoolean() const;

Q_SIGNALS:
    void cursorMoved( int index );
    void visibilityChanged(bool visible);

public Q_SLOTS:
    void initYAxis();
    void setBoolean(bool value);

private Q_SLOTS:
    void onPickerMoved( const QPointF& );
    void onXScaleChanged();

private:
    const QwtPlotCurve* curve( int index ) const;
    QColor curveColor( int index ) const;

    const size_t            m_streamPos;
    const size_t            m_type;
    const size_t            m_group;
    QVector<QwtPlotCurve*>  m_curves;
    PlotCursor*             m_cursor;
    QCheckBox*              m_booleanPlotCheckbox;

    PlotLegend*             m_legend;
    const FileInformation*  m_fileInformation;

    bool                    m_boolean;
};

#endif // GUI_Plot_H

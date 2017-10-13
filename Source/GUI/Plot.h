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
#include <QJSEngine>
#include <QJSValue>
#include <QEvent>
#include <QResizeEvent>
#include <qwt_plot.h>
#include <qwt_series_data.h>
#include <qwt_widget_overlay.h>

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

static int indexLower( double x, const QwtSeriesData<QPointF> &data )
{
    int index = qwtUpperSampleIndex<QPointF>( data, x, compareX() );
    if ( index == -1 )
        index = data.size();

    return index - 1;
}

class PlotSeriesData : public QObject, public QwtPointSeriesData
{
    // QwtSeriesData interface
    Q_OBJECT
public:
    PlotSeriesData(CommonStats* stats, const int& xDataIndex, const size_t yDataIndex, size_t plotGroup, size_t curveIndex, size_t curvesCount)
        : m_boolean(false), m_stats(stats), m_xDataIndex(xDataIndex), m_yDataIndex(yDataIndex), m_plotGroup(plotGroup), m_curveIndex(curveIndex), m_curvesCount(curvesCount),
        m_condition(stats, plotGroup)
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
        return m_condition.match(y) ? 1.0 : 0.0;
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
        Condition(CommonStats* stats, size_t plotGroup) : m_stats(stats), m_plotGroup(plotGroup) {
            m_conditionString = "y < yHalf";
            m_engine.globalObject().setProperty("yHalf", (m_stats->y_Max[m_plotGroup] - m_stats->y_Min[m_plotGroup]) / 2);

            auto pow2 = m_engine.evaluate("function(value) { return Math.pow(value, 2); }");
            m_engine.globalObject().setProperty("pow2", pow2);

            auto pow = m_engine.evaluate("function(base, exponent) { return Math.pow(base, exponent); }");
            m_engine.globalObject().setProperty("pow", pow);

            update();
        }

        CommonStats* m_stats;
        size_t m_plotGroup;

        QString m_conditionString;
        mutable QJSValue m_conditionFunction;
        mutable QJSEngine m_engine;

        bool match(double y) const {

            if(m_conditionFunction.isCallable() && m_conditionFunction.call(QJSValueList() << y).toBool())
                return true;

            return false;
        }

        QJSValue makeConditionFunction(const QString& condition) const {
            return m_engine.evaluate(QString("function(y) { return %1; }").arg(condition));
        }

        void update() {
            if(m_conditionString.isEmpty())
                m_conditionFunction = QJSValue();
            else
                m_conditionFunction = makeConditionFunction(m_conditionString);
        }
    };

    Condition& condition() {
        return m_condition;
    }

public Q_SLOTS:
    void setBoolean(bool enable) {
        qDebug() << "boolean mode: " << enable;
        m_boolean = enable;
    }

private:
    bool m_boolean;
    Condition m_condition;
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

    void setData(size_t curveIndex, QwtSeriesData<QPointF> *series);
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

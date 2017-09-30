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
#include <QEvent>
#include <QResizeEvent>
#include <qwt_plot.h>
#include <qwt_series_data.h>
#include <qwt_widget_overlay.h>

class QwtPlotCurve;
class PlotCursor;
class PlotLegend;
class FileInformation;

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

class Plot : public QwtPlot
{
    Q_OBJECT

public:
    class SeriesData : public QwtPointSeriesData
    {
        // QwtSeriesData interface
    public:
        SeriesData(CommonStats* stats, const int& xDataIndex, const int yDataIndex)
            : stats(stats), xDataIndex(xDataIndex), yDataIndex(yDataIndex) {

        }

        size_t size() const {
            return stats->x_Current;
        }
        QPointF sample(size_t i) const {
            auto xData = stats->x[xDataIndex];
            auto yData = stats->y[yDataIndex];

            return QPointF(xData[i], yData[i]);
        }

    private:
        CommonStats* stats;
        const int& xDataIndex;
        const int yDataIndex;
    };

    explicit Plot( size_t streamPos, size_t Type, size_t Group, const FileInformation* fileInformation, QWidget *parent );
    virtual ~Plot();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setYAxis( double min, double max, int numSteps );
    void setCursorPos( double x );

    void setData(int curveIndex, QwtSeriesData<QPointF> *series);

    size_t streamPos() const { return m_streamPos; }
    size_t type() const { return m_type; }
    size_t group() const { return m_group; }

    PlotLegend *legend() { return m_legend; }

    int frameAt( double x ) const;

    void addGuidelines(int bitsPerRawSample);

Q_SIGNALS:
    void cursorMoved( int index );

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

    PlotLegend*             m_legend;
    const FileInformation*  m_fileInformation;
};

#endif // GUI_Plot_H

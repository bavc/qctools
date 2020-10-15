#ifndef PANELSVIEW_H
#define PANELSVIEW_H

#include "PlotLegend.h"

#include <QFrame>
#include <QSize>
#include <functional>
#include <qwt_legend.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_picker.h>

class QwtPlot;
class PlotCursor;
class CommentsPlot;

class QwtPanelPicker: public QwtPicker
{
    Q_OBJECT

public:
    explicit QwtPanelPicker( QWidget *canvas, QwtPlot* p ) : QwtPicker(canvas), m_plot(p),
        d_xAxis( -1 ),
        d_yAxis( -1 )
    {
        if ( !canvas )
            return;

        // attach axes

        int xAxis = QwtPlot::xBottom;

        const QwtPlot *plot = m_plot;
        if ( !plot->axisEnabled( QwtPlot::xBottom ) &&
            plot->axisEnabled( QwtPlot::xTop ) )
        {
            xAxis = QwtPlot::xTop;
        }

        int yAxis = QwtPlot::yLeft;
        if ( !plot->axisEnabled( QwtPlot::yLeft ) &&
            plot->axisEnabled( QwtPlot::yRight ) )
        {
            yAxis = QwtPlot::yRight;
        }

        setAxis( xAxis, yAxis );

        setRubberBand( QwtPlotPicker::CrossRubberBand );
        setRubberBandPen( QColor( Qt::green ) );

        setTrackerMode( QwtPicker::AlwaysOn );
        setTrackerPen( QColor( Qt::black ) );

        setStateMachine( new QwtPickerDragPointMachine () );
    }

    virtual ~QwtPanelPicker() {}

    virtual void setAxis( int xAxis, int yAxis ) {
        const QwtPlot *plt = plot();
        if ( !plt )
            return;

        if ( xAxis != d_xAxis || yAxis != d_yAxis )
        {
            d_xAxis = xAxis;
            d_yAxis = yAxis;
        }
    }

    int xAxis() const;
    int yAxis() const;

    QwtPlot *plot() {
        return m_plot;
    }
    const QwtPlot *plot() const {
        return m_plot;
    }

    QWidget *canvas() {
        return parentWidget();
    }
    const QWidget *canvas() const {
        return parentWidget();
    }

    /*!
      Translate a pixel position into a position string

      \param pos Position in pixel coordinates
      \return Position string
    */
    QwtText trackerText( const QPoint &pos ) const
    {
        if ( plot() == NULL )
            return QwtText();

        return trackerTextF( invTransform( pos ) );
    }

    /*!
      \brief Translate a position into a position string

      In case of HLineRubberBand the label is the value of the
      y position, in case of VLineRubberBand the value of the x position.
      Otherwise the label contains x and y position separated by a ',' .

      The format for the double to string conversion is "%.4f".

      \param pos Position
      \return Position string
    */
    QwtText trackerTextF( const QPointF &pos ) const
    {
        QString text;

        switch ( rubberBand() )
        {
            case HLineRubberBand:
                text.sprintf( "%.4f", pos.y() );
                break;
            case VLineRubberBand:
                text.sprintf( "%.4f", pos.x() );
                break;
            default:
                text.sprintf( "%.4f, %.4f", pos.x(), pos.y() );
        }
        return QwtText( text );
    }

    /*!
      Append a point to the selection and update rubber band and tracker.

      \param pos Additional point
      \sa isActive, begin(), end(), move(), appended()

      \note The appended(const QPoint &), appended(const QDoublePoint &)
            signals are emitted.
    */
    void append( const QPoint &pos )
    {
        QwtPicker::append( pos );
        Q_EMIT appended( invTransform( pos ) );
    }

    /*!
      Move the last point of the selection

      \param pos New position
      \sa isActive, begin(), end(), append()

      \note The moved(const QPoint &), moved(const QDoublePoint &)
            signals are emitted.
    */
    void move( const QPoint &pos )
    {
        QwtPicker::move( pos );
        Q_EMIT moved( invTransform( pos ) );
    }

    /*!
      Close a selection setting the state to inactive.

      \param ok If true, complete the selection and emit selected signals
                otherwise discard the selection.
      \return True if the selection has been accepted, false otherwise
    */

    bool end( bool ok )
    {
        ok = QwtPicker::end( ok );
        if ( !ok )
            return false;

        QwtPlot *plot = m_plot;
        if ( !plot )
            return false;

        const QPolygon points = selection();
        if ( points.count() == 0 )
            return false;

        QwtPickerMachine::SelectionType selectionType =
            QwtPickerMachine::NoSelection;

        if ( stateMachine() )
            selectionType = stateMachine()->selectionType();

        switch ( selectionType )
        {
            case QwtPickerMachine::PointSelection:
            {
                const QPointF pos = invTransform( points.first() );
                Q_EMIT selected( pos );
                break;
            }
            case QwtPickerMachine::RectSelection:
            {
                if ( points.count() >= 2 )
                {
                    const QPoint p1 = points.first();
                    const QPoint p2 = points.last();

                    const QRect rect = QRect( p1, p2 ).normalized();
                    Q_EMIT selected( invTransform( rect ) );
                }
                break;
            }
            case QwtPickerMachine::PolygonSelection:
            {
                QVector<QPointF> dpa( points.count() );
                for ( int i = 0; i < points.count(); i++ )
                    dpa[i] = invTransform( points[i] );

                Q_EMIT selected( dpa );
            }
            default:
                break;
        }

        return true;
    }

    /*!
        Translate a rectangle from pixel into plot coordinates

        \return Rectangle in plot coordinates
        \sa transform()
    */
    QRectF invTransform( const QRect &rect ) const
    {
        const QwtScaleMap xMap = plot()->canvasMap( d_xAxis );
        const QwtScaleMap yMap = plot()->canvasMap( d_yAxis );

        return QwtScaleMap::invTransform( xMap, yMap, rect );
    }

    /*!
        Translate a rectangle from plot into pixel coordinates
        \return Rectangle in pixel coordinates
        \sa invTransform()
    */
    QRect transform( const QRectF &rect ) const
    {
        const QwtScaleMap xMap = plot()->canvasMap( d_xAxis );
        const QwtScaleMap yMap = plot()->canvasMap( d_yAxis );

        return QwtScaleMap::transform( xMap, yMap, rect ).toRect();
    }

    /*!
        Translate a point from pixel into plot coordinates
        \return Point in plot coordinates
        \sa transform()
    */
    QPointF invTransform( const QPoint &pos ) const
    {
        QwtScaleMap xMap = plot()->canvasMap( d_xAxis );
        QwtScaleMap yMap = plot()->canvasMap( d_yAxis );

        return QPointF(
            xMap.invTransform( pos.x() - canvas()->contentsMargins().left() ),
            yMap.invTransform( pos.y() )
        );
    }

    /*!
        Translate a point from plot into pixel coordinates
        \return Point in pixel coordinates
        \sa invTransform()
    */
    QPoint transform( const QPointF &pos ) const
    {
        QwtScaleMap xMap = plot()->canvasMap( d_xAxis );
        QwtScaleMap yMap = plot()->canvasMap( d_yAxis );

        const QPointF p( xMap.transform( pos.x() ),
            yMap.transform( pos.y() ) );

        return p.toPoint();
    }

Q_SIGNALS:

    /*!
      A signal emitted in case of QwtPickerMachine::PointSelection.
      \param pos Selected point
    */
    void selected( const QPointF &pos );

    /*!
      A signal emitted in case of QwtPickerMachine::RectSelection.
      \param rect Selected rectangle
    */
    void selected( const QRectF &rect );

    /*!
      A signal emitting the selected points,
      at the end of a selection.

      \param pa Selected points
    */
    void selected( const QVector<QPointF> &pa );

    /*!
      A signal emitted when a point has been appended to the selection

      \param pos Position of the appended point.
      \sa append(). moved()
    */
    void appended( const QPointF &pos );

    /*!
      A signal emitted whenever the last appended point of the
      selection has been moved.

      \param pos Position of the moved last point of the selection.
      \sa move(), appended()
    */
    void moved( const QPointF &pos );

protected:
    QRectF scaleRect() const;

private:
    int d_xAxis;
    int d_yAxis;

    QwtPlot* m_plot;
};

class CommentsPlot;
class PanelsView : public QFrame
{
    Q_OBJECT
public:
    explicit PanelsView(QWidget *parent = nullptr, const QString & panelTitle = QString(), const QString & yaxis = QString(), const QString & legend = QString(), CommentsPlot* plot = nullptr);
    void setProvider(const std::function<int()>& getPanelsCount,
                     const std::function<QImage(int)>& getPanelImage);

    int panelIndexByFrame(int frameIndex) const;
    void getPanelsBounds(int& startPanelIndex, int& startPanelOffset, int& endPanelIndex, int& endPanelLength);
    void refresh();

    QString panelTitle() const { return m_panelTitle; }
    uint panelGroup() const { return m_panelGroup; }
    PlotLegend *legend() { return m_legend; }

public Q_SLOTS:
    void setVisibleFrames(int from, int to);
    void setActualWidth(int width);
    void setCursorPos(double x);
    void setLeftOffset(int leftOffset);

    void onPickerMoved( const QPointF& pos );

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *event);

Q_SIGNALS:
    void cursorMoved( int index );

private:

    PlotLegend* m_legend;
    PlotCursor* m_PlotCursor;
    CommentsPlot* m_plot;
    QString m_panelTitle;
    uint m_panelGroup;

    QString m_bottomYLabel;
    QString m_topYLabel;
    QString m_middleYLabel;

    int m_leftOffset;
    int m_startFrame;
    int m_endFrame;
    int m_actualWidth;
    std::function<int()> getPanelsCount;
    std::function<QImage(int)> getPanelImage;
};

#endif // PANELSVIEW_H

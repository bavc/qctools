/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plot_H
#define GUI_Plot_H
//---------------------------------------------------------------------------

#include <Core/Core.h>
#include <qwt_plot.h>

class QwtPlotCurve;
class PlotCursor;
class PlotLegend;

//***************************************************************************
// Class
//***************************************************************************

class Plot : public QwtPlot
{
    Q_OBJECT

public:
    explicit Plot( PlotType Type, QWidget *parent );
    virtual ~Plot();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setYAxis( double max, int numSteps );
    void setCursorPos( double x );

    void setCurveSamples( int index,
        const double *xData, const double *yData, int size );

    PlotType type() const { return m_type; }

    PlotLegend *legend() { return m_legend; }

	int frameAt( double x ) const;

Q_SIGNALS:
    void cursorMoved( int index );

private Q_SLOTS:
    void onPickerMoved( const QPointF& );
    void onXScaleChanged();

private:
    const QwtPlotCurve* curve( int index ) const;
    QColor curveColor( int index ) const;

    const PlotType          m_type;
    QVector<QwtPlotCurve*>  m_curves;
    PlotCursor*             m_cursor;

    PlotLegend*             m_legend;
};

#endif // GUI_Plot_H

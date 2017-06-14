/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Notes_H
#define GUI_Notes_H
//---------------------------------------------------------------------------

#include "Plot.h"

#include <Core/Core.h>
#include <Core/CommonStats.h>
#include <Core/FileInformation.h>
#include <qwt_plot.h>
#include <qwt_plot_barchart.h>
#include <qwt_plot.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_multi_barchart.h>
#include <qwt_column_symbol.h>
#include <qwt_plot_layout.h>
#include <qwt_legend.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_picker.h>

class CommentsSeriesData : public QwtPointSeriesData
{
    // QwtSeriesData interface
public:
    CommentsSeriesData(CommonStats* stats, const int* dataTypeIndex = 0) : stats(stats), pDataTypeIndex(dataTypeIndex) {

    }

    size_t size() const {
        return stats->x_Current;
    }
    QPointF sample(size_t i) const {
        int dataTypeIndex = pDataTypeIndex ? *pDataTypeIndex : 0;
        return (stats->comments[i] != nullptr) ? QPointF(stats->x[dataTypeIndex][i], 1) : QPointF(stats->x[dataTypeIndex][i], 0);
    }

private:
    CommonStats* stats;
    const int* pDataTypeIndex;
};

class CommentsPlotPicker: public QwtPlotPicker
{
public:
    CommentsPlotPicker(QWidget* w, CommonStats* stats);

    const QwtPlotCurve* curve( int index ) const;
    virtual QwtText trackerTextF( const QPointF &pos ) const;

protected:
    virtual QString infoText( int index ) const;

private:
    CommonStats* stats;
};

class PlotLegend;
class CommentsPlot : public QwtPlot {
    Q_OBJECT
public:
    CommentsPlot(FileInformation* fileInfo, CommonStats* stats, const int* dataTypeIndex = nullptr);

    int frameAt( double x ) const;
    void setCursorPos( double x );

    PlotLegend *legend() { return m_legend; }

Q_SIGNALS:
    void cursorMoved(int index);

private Q_SLOTS:
    void onPickerMoved(const QPointF& );
    void onXScaleChanged();

private:
    const QwtPlotCurve* curve( int index ) const;

    PlotCursor* m_cursor;
    PlotLegend* m_legend;
};

CommentsPlot* createCommentsPlot(FileInformation* fileInfo, const int* dataTypeIndex);

#endif // GUI_Notes_H

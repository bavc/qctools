/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Notes_H
#define GUI_Notes_H
//---------------------------------------------------------------------------

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

class CommentsHistogramSeriesData : public QwtIntervalSeriesData
{
    // QwtSeriesData interface
public:
    CommentsHistogramSeriesData(CommonStats* stats, const int* dataTypeIndex = 0) : stats(stats), pDataTypeIndex(dataTypeIndex) {

    }

    size_t size() const {
        return stats->x_Current;
    }

    QwtIntervalSample sample(size_t i) const {
        int dataTypeIndex = pDataTypeIndex ? *pDataTypeIndex : 0;
        return (stats->comments[i] != nullptr) ? QwtIntervalSample(stats->x[dataTypeIndex][i], 0, stats->x[dataTypeIndex][i]) : QwtIntervalSample(stats->x[dataTypeIndex][i], 0, 0);
    }

private:
    CommonStats* stats;
    const int* pDataTypeIndex;
};

class CommentsBarChart : public QwtPlotBarChart
{
    // QwtPlotBarChart interface
protected:

    // QwtPlotBarChart interface
protected:
    void drawBar(QPainter *painter, int sampleIndex, const QPointF &sample, const QwtColumnRect &rect) const {

        // QwtPlotBarChart::drawBar(painter, sampleIndex, sample, rect);

        if(sample.y() != 0.0f)
        {
            /*
            QwtColumnSymbol sym( QwtColumnSymbol::Box );
            sym.setLineWidth( 1 );
            sym.setFrameStyle( QwtColumnSymbol::Plain );
            sym.draw( painter, rect );
            */

            QRectF r = rect.toRect();
            {
                r.setLeft( qRound( r.left() ) );
                r.setRight( qRound( r.right() ) );
                r.setTop( qRound( r.top() ) );
                r.setBottom( qRound( r.bottom() ) );
            }

            painter->setBrush(Qt::red);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawEllipse(r.center(), 3, r.height() / 2);
        }
    }
};

QwtPlot* createCommentsPlot(FileInformation* fileInfo, const int* dataTypeIndex);

#endif // GUI_Notes_H

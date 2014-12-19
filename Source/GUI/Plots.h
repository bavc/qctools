/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plots_H
#define GUI_Plots_H
//---------------------------------------------------------------------------

#include "Core/Core.h"
#include "GUI/FileInformation.h"

#include <QWidget>

class QwtPlot;
class Plot;
class PlotScaleWidget;

//***************************************************************************
// Class
//***************************************************************************

class Plots : public QWidget
{
    Q_OBJECT

public:
    enum XAxisFormat
    {
        AxisFrames,
        AxisSeconds,
        AxisMinutes,
        AxisHours,
        AxisTime
    };

    explicit                    Plots( QWidget *parent, FileInformation* );
    virtual                     ~Plots();

    void                        scrollXAxis();
    void                        setPlotVisible( PlotType, bool on );

    const QwtPlot*              plot( PlotType ) const;

    void                        Zoom_Move( int Begin );

    void                        zoomXAxis( bool up );
    bool                        isZoomed() const;
    bool                        isZoomable() const;
    size_t                      visibleFramesCount() const;

    virtual bool                eventFilter( QObject *, QEvent * );

private Q_SLOTS:
    void                        onCursorMoved( int index );
    void                        onXAxisFormatChanged( int index );

private:
    void                        replotAll();

    void                        initAxisFormat( int index );

    void                        initYAxis( Plot* );
    void                        updateSamples( Plot* );
    void                        setCursorPos( int framePos );

    void                        alignXAxis( const Plot* );
    void                        alignYAxes();
    const VideoStats*           videoStats() const { return m_fileInfoData->Videos[0]; }
    VideoStats*                 videoStats() { return m_fileInfoData->Videos[0]; }
    int                         framePos() const { return m_fileInfoData->Frames_Pos_Get(); }
    int                         numFrames() const { return videoStats()->x_Current_Max; }

    void                        setFrameRange( int from, int to );

private:
    PlotScaleWidget*            m_scaleWidget;
    Plot*                       m_plots[PlotType_Max];

    int                         m_visibleFrame[2];
    // X axis info
    int                         m_dataTypeIndex;
    FileInformation*            m_fileInfoData;
};

#endif // GUI_Plots_H

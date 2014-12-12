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
class ScaleWidget;

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

    explicit                    Plots( QWidget *parent, FileInformation* FileInfoData );
    virtual                     ~Plots();

    void                        Plots_Update();
    void                        Marker_Update();
    void                        setPlotVisible( PlotType, bool on );

    const QwtPlot*              plot( PlotType ) const;

    void                        Zoom_Move( size_t Begin );

    void                        zoom( bool up );
    int                         zoomLevel() const { return m_zoomLevel; }

private Q_SLOTS:
    void                        onCursorMoved( double x );
    void                        onXAxisFormatChanged( int index );

private:
    Plot*                       plotAt( int row );
    const Plot*                 plotAt( int row ) const;

    void                        replotAll();

    void                        shiftXAxes();
    void                        shiftXAxes( size_t Begin );

    void                        syncPlots();
    void                        syncPlot( PlotType Type );
    void                        setCursorPos( double X );

    void                        alignYAxes();
    double                      axisStepSize( double s ) const;
    const VideoStats*           videoStats() const { return m_fileInfoData->Videos[0]; }
    VideoStats*                 videoStats() { return m_fileInfoData->Videos[0]; }
    int                         framePos() const { return m_fileInfoData->Frames_Pos_Get(); }

private:
	ScaleWidget*                m_scaleWidget;
    QwtPlot*                    m_plots[PlotType_Max];
    size_t                      m_zoomLevel;

    // X axis info
    int                         m_dataTypeIndex;
    size_t                      m_Data_FramePos_Max;

    FileInformation*            m_fileInfoData;
};

#endif // GUI_Plots_H

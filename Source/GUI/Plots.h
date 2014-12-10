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
struct per_item;

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

    explicit                    Plots( QWidget *parent, const struct stream_info* streamInfo, FileInformation* FileInfoData, size_t StatsPos );
    virtual                     ~Plots();

    void                        updateAll();
    void                        Plots_Update();
    void                        Marker_Update();
    void                        setPlotVisible( size_t, bool on );

    const QwtPlot*              plot( size_t ) const;

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
    void                        syncPlot( size_t Type );
    void                        setCursorPos( double X );

    void                        alignYAxes();
    double                      axisStepSize( double s ) const;
    const CommonStats*          stats() const { return m_fileInfoData->Stats[m_statsPos]; }
    CommonStats*                stats() { return m_fileInfoData->Stats[m_statsPos]; }
    int                         framePos() const { return m_fileInfoData->Frames_Pos_Get(m_statsPos); }

private:
    QwtPlot**                   m_plots;
    size_t                      m_zoomLevel;

    // X axis info
    int                         m_dataTypeIndex;
    size_t                      m_Data_FramePos_Max;

    FileInformation*            m_fileInfoData;

    size_t                      m_statsPos;

    // Arrays
    const struct stream_info*   m_streamInfo; 
};

#endif // GUI_Plots_H

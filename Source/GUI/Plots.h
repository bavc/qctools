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
#include "Core/CommonStats.h"
#include "GUI/FileInformation.h"

#include <QWidget>

class QwtPlot;
class Plot;
class PlotScaleWidget;

class FrameInterval
{
public:
    FrameInterval():
        from( 0 ),
        to( 0 )
    {
    }

    int count() const
    {
        return to - from + 1;
    }

    int from;
    int to;
};

class TimeInterval
{
public:
    TimeInterval():
        from( 0 ),
        to( 0 )
    {
    }

    int count() const
    {
        return to - from + 1;
    }

    double from;
    double to;
};

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

    enum ZoomTypes
    {
        ZoomIn,
        ZoomOut,
        ZoomOneToOne
    };

    explicit                    Plots( QWidget *parent, FileInformation* );
    virtual                     ~Plots();

    void                        setPlotVisible( size_t type, size_t group, bool on );

    const QwtPlot*              plot( size_t streamPos, size_t group ) const;

    void                        Zoom_Move( int Begin );
    void                        refresh();

    void                        zoomXAxis( ZoomTypes type );
    bool                        isZoomed() const;
    FrameInterval               visibleFrames() const;
    int                         numFrames() const { return stats()->x_Current_Max; }

    virtual bool                eventFilter( QObject *, QEvent * );
    void                        adjustGroupMax(int group, int bitsPerRawSample);
    void                        changeOrder(QList<QPair<int, int> > filterSelectorsInfo);

public Q_SLOTS:
    void                        onCurrentFrameChanged();

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

    void                        setVisibleFrames( int from, int to, bool force = false );

    const CommonStats*          stats( size_t statsPos = (size_t)-1 ) const { if ( statsPos == (size_t)-1 ) return m_fileInfoData->ReferenceStat(); else return m_fileInfoData->Stats[statsPos]; }
    CommonStats*                stats( size_t statsPos = (size_t)-1 ) { if ( statsPos == (size_t)-1 ) return m_fileInfoData->ReferenceStat(); else return m_fileInfoData->Stats[statsPos]; }
    int                         framePos( size_t statsPos = (size_t)-1 ) const { return m_fileInfoData->Frames_Pos_Get(statsPos); }
    void                        setFramePos( size_t framePos, size_t statsPos = (size_t)-1 ) const { m_fileInfoData->Frames_Pos_Set(framePos, statsPos); }

private:
    PlotScaleWidget*            m_scaleWidget;
    Plot***                     m_plots; // pointer on an array of streams and groups per stream and Plot* per group
    int                         m_plotsCount;

    FrameInterval               m_frameInterval;
    TimeInterval                m_timeInterval;
    int                         m_zoomFactor;
    ZoomTypes                   m_zoomType;

    // X axis info
    int                         m_dataTypeIndex;
    FileInformation*            m_fileInfoData;
};

#endif // GUI_Plots_H

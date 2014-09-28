/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plots_H
#define GUI_Plots_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/Core.h"
#include "GUI/FileInformation.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QWidget>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
class QwtPlot;
class QwtLegend;
class QwtPlotZoomer;
class QwtPlotCurve;
class QwtPlotPicker;
class QwtPlotMarker;

class QLabel;
class QToolButton;
class QPushButton;
class QComboBox;
class QVBoxLayout;
class QHBoxLayout;

class PerPicture;
class TinyDisplay;
class Control;
class Info;
//---------------------------------------------------------------------------

//***************************************************************************
// Class
//***************************************************************************

class Plots : public QWidget
{
    Q_OBJECT
    
public:
    explicit Plots(QWidget *parent, FileInformation* FileInfoData);
    ~Plots();

    // File information
    FileInformation*            FileInfoData;

    // To update
    TinyDisplay*                TinyDisplayArea;
    Control*                    ControlArea;
    Info*                       InfoArea;
    
    // Positioning info
    size_t                      ZoomScale;

    // Plots
    void                        Plots_Create                ();
    void                        Plots_Create                (PlotType Type);
    void                        Plots_Update                ();
    void                        Marker_Update               ();
    void                        Marker_Update               (double X);
    void                        createData_Init             ();
    void                        createData_Update           ();
    void                        createData_Update           (PlotType Type);

    // UI
    bool                        Status[PlotType_Max];
    void                        refreshDisplay              ();
    void                        refreshDisplay_Axis         ();
    
    // Zoom
    void                        Zoom_Move                   (size_t Begin);
    void                        Zoom_In                     ();
    void                        Zoom_Out                    ();
    
    // Widgets
    QVBoxLayout*                Layout;
    QHBoxLayout*                Layouts[PlotType_Max];
    QWidget*                    paddings[PlotType_Max];
    QwtPlot*                    plots[PlotType_Max];
    QwtLegend*                  legends[PlotType_Max];

    // Widgets addons
    QwtPlotCurve*               plotsCurves[PlotType_Max][5];
    QwtPlotZoomer*              plotsZoomers[PlotType_Max];
    QwtPlotPicker*              plotsPickers[PlotType_Max];
    QwtPlotMarker*              plotsMarkers[PlotType_Max];

    // X axis info
    QComboBox*                  XAxis_Kind;
    int                         XAxis_Kind_index;
    qreal                       Zoom_Left;
    qreal                       Zoom_Width;

    // Y axis info
    double                      plots_YMax[PlotType_Max];

private Q_SLOTS:
    void plot_moved( const QPointF & );
    void on_XAxis_Kind_currentIndexChanged(int index);
};

#endif // GUI_Plots_H

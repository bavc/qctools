/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Control_H
#define GUI_Control_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QMainWindow>

class FileInformation;
class PerPicture;
class TinyDisplay;
class Info;
class Plots;

class QwtPlot;
class QwtLegend;
class QwtPlotZoomer;
class QwtPlotCurve;
class QwtPlotPicker;

class QLabel;
class QToolButton;
class QPushButton;
class QTimer;
class QTime;
//---------------------------------------------------------------------------

//***************************************************************************
// Class
//***************************************************************************

class Control : public QWidget
{
    Q_OBJECT

public:
    enum style
    {
        Style_Cols,
        Style_Grid,
    };
    explicit Control(QWidget *parent, FileInformation* FileInfoData, Plots* PlotsArea, style Style, bool IsSlave=false);
    ~Control();

    // Commands
    void                        Update                      ();

    // Info
    bool                        ShouldUpate;

    // To update
    TinyDisplay*                TinyDisplayArea;
    Info*                       InfoArea;

    // Widgets
    QToolButton*                M9;
    QToolButton*                M2;
    QToolButton*                M1;
    QToolButton*                M0;
    QToolButton*                Minus;
    QLabel*                     Info_Time;
    QToolButton*                PlayPause;
    QToolButton*                Pause;
    QLabel*                     Info_Frames;
    QToolButton*                Plus;
    QToolButton*                P0;
    QToolButton*                P1;
    QToolButton*                P2;
    QToolButton*                P9;

protected:
    // File information
    FileInformation*            FileInfoData;
    Plots*                      PlotsArea;
    int                         Frames_Pos;
    style                       Style;

    // For speed
    int                         Frames_Total;

    // Time
    QTimer* Timer;
    QTime*  Time;
    bool    Time_MinusPlus;
    int     Timer_Duration;
public:
    enum selectedspeed
    {
        Speed_M2,
        Speed_M1,
        Speed_M0,
        Speed_O,
        Speed_P0,
        Speed_P1,
        Speed_P2,
    };
    selectedspeed SelectedSpeed;
protected:
    bool    IsSlave;

    void TimeOut_Init();

public Q_SLOTS:
    void TimeOut();
    void on_M9_clicked(bool checked);
    void on_M2_clicked(bool checked);
    void on_M1_clicked(bool checked);
    void on_M0_clicked(bool checked);
    void on_Minus_clicked(bool checked=false);
    void on_PlayPause_clicked(bool checked);
    void on_Pause_clicked(bool checked);
    void on_Plus_clicked(bool checked=false);
    void on_P0_clicked(bool checked);
    void on_P1_clicked(bool checked);
    void on_P2_clicked(bool checked);
    void on_P9_clicked(bool checked);
};

#endif // GUI_Control_H

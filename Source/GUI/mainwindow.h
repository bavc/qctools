/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>
#include <QTemporaryDir>

#include <vector>
using namespace std;

#include "Core/Core.h"
#include "GUI/FileInformation.h"
#include "GUI/Plots.h"
#include "GUI/TinyDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"

namespace Ui {
class MainWindow;
}

class QwtPlot;
class QwtLegend;
class QwtPlotZoomer;
class QwtPlotCurve;
class QwtPlotPicker;

class QPixmap;
class QLabel;
class QToolButton;
class QDropEvent;
class QDragEnterEvent;
class QPushButton;
class QComboBox;

class PerPicture;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //Functions
    void dragEnterEvent         (QDragEnterEvent *event);
    void dropEvent              (QDropEvent *event);
    
    // UI
    void                        Ui_Init                     ();
    void                        configureZoom               ();
    void                        Update                      ();
    void                        processFile                 (const QString &FileName);
    void                        openFile                    ();
    void                        Zoom_Move                   (size_t Begin);
    void                        Zoom_In                     ();
    void                        Zoom_Out                    ();
    void                        Export_CSV                  ();
    void                        Export_PDF                  ();
    void                        refreshDisplay              ();
    void                        refreshDisplay_Axis         ();
    void                        Help_GettingStarted         ();
    void                        Help_HowToUse               ();
    void                        Help_FilterDescriptions     ();
    void                        Help_PlaybackFilters        ();
    void                        Help_About                  ();

    // Visual elements
    Plots*                      PlotsArea;
    TinyDisplay*                TinyDisplayArea;
    Control*                    ControlArea;
    Info*                       InfoArea;

    // Files
    std::vector<FileInformation*> Files;
    size_t                      Files_Pos;

private Q_SLOTS:

    void TimeOut();
    void TimeOut_Refresh();

    void on_actionQuit_triggered();

    void on_actionOpen_triggered();

    void on_horizontalScrollBar_valueChanged(int value);

    void on_actionZoomIn_triggered();

    void on_actionZoomOut_triggered();

    void on_actionGoTo_triggered();

    void on_actionToolbar_triggered();

    void on_Toolbar_visibilityChanged(bool visible);

    void on_actionCSV_triggered();

    void on_actionPrint_triggered();

    void on_actionGettingStarted_triggered();

    void on_actionHowToUseThisTool_triggered();

    void on_actionFilterDescriptions_triggered();

    void on_actionPlaybackFilters_triggered();

    void on_actionAbout_triggered();

    void on_check_Y_toggled(bool checked);

    void on_check_U_toggled(bool checked);

    void on_check_V_toggled(bool checked);

    void on_check_YDiff_toggled(bool checked);

    void on_check_YDiffX_toggled(bool checked);

    void on_check_UDiff_toggled(bool checked);

    void on_check_VDiff_toggled(bool checked);

    void on_check_Diffs_toggled(bool checked);

    void on_check_TOUT_toggled(bool checked);

    void on_check_VREP_toggled(bool checked);

    void on_check_HEAD_toggled(bool checked);

    void on_check_BRNG_toggled(bool checked);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

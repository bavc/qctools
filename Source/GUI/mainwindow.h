#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>

#include <vector>
using namespace std;

#include <Core/Core.h>

namespace Ui {
class MainWindow;
}

class QwtPlot;
class QwtPlotZoomer;
class QwtPlotCurve;

class QDropEvent;
class QDragEnterEvent;


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //Functions
    void init();
    void configureZoom          ();
    void processFile            ();
    void openFile               ();
    void Zoom_Move              (size_t Begin);
    void Zoom_In                ();
    void Zoom_Out               ();
    void Export_CSV             ();
    void Export_PDF             ();
    void refreshDisplay         ();
    void createData_Init        ();
    void createData_Update      ();
    void createData_Update      (PlotType Type);
    void createData             ();
    void createPlot             (PlotType Type);
    void Help_FilterDescriptions();

    //Data
    double*     x; //PTS
    double**    y; //Data
    double      y_Max[PlotType_Max];
    
    //Plots
    QwtPlot*        plots[PlotType_Max];
    QwtPlotCurve*   curves[PlotType_Max][5];
    QwtPlotZoomer*  plotsZoomers[PlotType_Max];
    size_t          ZoomScale;

    //Temp
    QProcess*       Process;
    QByteArray      Data;
    int             Data_Pos;
    int             Data_Plot_Pos;
    enum analysisstatus
    {
        Status_GettingFrames,
        Status_Parsing,
        Status_Ok,
    };
    analysisstatus Process_Status;
    int             Frames_Total;
    int             Frames_Pos;
    int             Frames_Plot_Pos;
    double          Duration;
    QByteArray      Data_EOL;
    QString         FileName;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    
private Q_SLOTS:
    void ProcessMessage();
    void ProcessError();
    void ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_actionQuit_triggered();

    void on_actionOpen_triggered();

    void on_horizontalScrollBar_valueChanged(int value);

    void on_actionZoomIn_triggered();

    void on_actionZoomOut_triggered();

    void on_actionCSV_triggered();

    void on_actionPrint_triggered();

    void on_actionFilterDescriptions_triggered();

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

    void on_check_RANG_toggled(bool checked);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

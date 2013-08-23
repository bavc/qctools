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
#include "GUI/PerFile.h"

namespace Ui {
class MainWindow;
}

class QwtPlot;
class QwtPlotZoomer;
class QwtPlotCurve;
class QwtPlotPicker;

class QPixmap;
class QLabel;
class QToolButton;
class QDropEvent;
class QDragEnterEvent;
class QPushButton;

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

    //Temp
    QString                     FileName;
    
    // UI
    void                        Ui_Init                     ();
    void                        configureZoom               ();
    void                        processFile                 ();
    void                        openFile                    ();
    void                        Zoom_Move                   (size_t Begin);
    void                        Zoom_In                     ();
    void                        Zoom_Out                    ();
    void                        Export_CSV                  ();
    void                        Export_PDF                  ();
    void                        refreshDisplay              ();
    void                        Help_FilterDescriptions     ();

    // Plots
    void                        Plots_Init                  ();
    void                        Plots_Create                ();
    void                        Plots_Create                (PlotType Type);
    void                        createData_Init             ();
    void                        createData_Update           ();
    void                        createData_Update           (PlotType Type);
    QwtPlot*                    plots[PlotType_Max];
    QwtPlotCurve*               curves[PlotType_Max][5];
    QwtPlotZoomer*              plotsZoomers[PlotType_Max];
    QwtPlotPicker*              plotsPicker[PlotType_Max];
    size_t                      ZoomScale;
    int                         Frames_Total;
    int                         Frames_Pos;
    double                      plots_YMax[PlotType_Max];

    // Pictures
    void                        Pictures_Init               ();
    void                        Pictures_Create             ();
    void                        Pictures_Update             (size_t Picture_Pos);
    QWidget*                    Pictures_Widgets;
    QLabel*                     Labels[9];
    QToolButton*                Labels_Middle;
    PerPicture*                 Picture_Main;
    size_t                      Picture_Main_X;
    QLabel*                     FirstDisplay;
    QWidget*                    Control_Widgets;
    QLabel*                     Control_Info;
    QPushButton*                Control_Minus;
    QPushButton*                Control_Plus;

    // Per file
    std::vector<PerFile*>       Files;
    size_t                      Files_Pos;

    // Callbacks
    void                        BasicInfo_Finished          ();
    void                        Thumbnails_Updated          ();
    void                        Thumbnails_Finished         ();
    void                        Stats_Updated               ();
    void                        Stats_Finished              ();

private Q_SLOTS:
    void plot_moved( const QPoint & );
    void plot_mousePressEvent(QMouseEvent *ev);
    void on_Labels_Middle_clicked(bool checked);
    void on_Control_Minus1_clicked(bool checked);
    void on_Control_Plus1_clicked(bool checked);

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

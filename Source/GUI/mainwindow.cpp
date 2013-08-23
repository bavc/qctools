#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QSizePolicy>
#include <QScrollArea>
#include <QPrinter>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QShortcut>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLabel>
#include <QToolButton>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_widget.h>

#include "GUI/PerPicture.h"

#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;


//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Ui_Init();
    Plots_Init();
    Pictures_Init();
    configureZoom();

    // Per file
    Files_Pos=(size_t)-1;
}

//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];    
    delete Picture_Main;

    delete ui;
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::on_actionQuit_triggered()
{
    close();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionOpen_triggered()
{
    openFile();
}

//---------------------------------------------------------------------------
void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
    Zoom_Move(value);
}

//---------------------------------------------------------------------------
void MainWindow::on_actionZoomIn_triggered()
{
    Zoom_In();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionZoomOut_triggered()
{
    Zoom_Out();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionCSV_triggered()
{
    Export_CSV();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionPrint_triggered()
{
    Export_PDF();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionFilterDescriptions_triggered()
{
    Help_FilterDescriptions();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_Y_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_U_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_V_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_YDiff_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_YDiffX_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_UDiff_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_VDiff_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_Diffs_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_TOUT_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_VREP_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_HEAD_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::on_check_RANG_toggled(bool checked)
{
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
 
//---------------------------------------------------------------------------
void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* Data=event->mimeData ();
    if (event->mimeData()->hasUrls())
    {
        //foreach (QUrl url, event->mimeData()->urls())
        {
          
            FileName=event->mimeData()->urls()[0].toLocalFile();

            //break; //TEMP: currently only one file
        }
    }

    processFile();
}

void MainWindow::plot_moved( const QPoint &pos )
{
    QString info;

    double X=plots[0]->invTransform(QwtPlot::xBottom, pos.x());
    double Y=plots[0]->invTransform(QwtPlot::yLeft, pos.y());
    if (X<0)
        X=0;
    if (Y<0)
        Y=0;
    double FrameRate=Frames_Total/Files[Files_Pos]->BasicInfo->Duration_Get();
    Picture_Main_X=(size_t)(X*FrameRate);
    if (Picture_Main_X>=Frames_Total)
        Picture_Main_X=Frames_Total-1;
    
    Pictures_Update(Picture_Main_X);
}

void MainWindow::plot_mousePressEvent(QMouseEvent *ev) 
{
    QPoint Point(ev->x(), ev->y());
    plot_moved(Point);
}

void MainWindow::on_Labels_Middle_clicked(bool checked)
{
    if (Picture_Main)
    {
        delete Picture_Main; Picture_Main=NULL;
    }
    else
    {
        Picture_Main=new PerPicture(this);
        Picture_Main->move(geometry().left()+geometry().width(), geometry().top());
        Picture_Main->show();
        Picture_Main->ShowPicture(Picture_Main_X, Files[Files_Pos]->Stats->y, FileName, Files[0]);
    }
}

void MainWindow::on_Control_Minus1_clicked(bool checked)
{
    if (Picture_Main_X==0)
        return;
    
    Pictures_Update(Picture_Main_X-1);
}

void MainWindow::on_Control_Plus1_clicked(bool checked)
{
    if (Picture_Main_X+1>=Frames_Total)
        return;
    
    Pictures_Update(Picture_Main_X+1);
}
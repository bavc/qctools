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
    ui->setupUi(this);

    //shortcuts
    QShortcut *shortcutPlus = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Plus), this);
    QObject::connect(shortcutPlus, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutEqual = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Equal), this);
    QObject::connect(shortcutEqual, SIGNAL(activated()), this, SLOT(on_actionZoomIn_triggered()));
    QShortcut *shortcutMinus = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus), this);
    QObject::connect(shortcutMinus, SIGNAL(activated()), this, SLOT(on_actionZoomOut_triggered()));

    //Temp
    Frames_Plot_Pos=0;
    Frames_Total=(int)-1;
    Process=NULL;
    Process_Status=Status_Ok;
    Ztring A(EOL);
    Data_EOL=A.To_UTF8().c_str();

    //Drag n drop
    setAcceptDrops(true);

    init();
}

//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
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

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
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
#include <QPushButton>
#include <QinputDialog>


//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Plots
    PlotsArea=NULL;

    // Pictures
    TinyDisplayArea=NULL;

    // Control
    ControlArea=NULL;

    // Info
    InfoArea=NULL;

    // Files
    Files_Pos=(size_t)-1;

    // UI
    Ui_Init();
}

//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    // Files (must be deleted first in order to stop ffmpeg processes)
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];    

    // Plots
    delete PlotsArea;

    // Pictures
    delete TinyDisplayArea;

    // Controls
    delete ControlArea;

    // Info
    delete InfoArea;

    // UI
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
void MainWindow::on_actionGoTo_triggered()
{
    if (!ControlArea && !TinyDisplayArea) //TODO: without TinyDisplayArea
        return;

    if (Files_Pos>=Files.size())
        return;
    
    bool ok;
    int i = QInputDialog::getInt(this, tr("Go to frame at position..."), tr("frame position (0-based):"), Files[Files_Pos]->Glue->VideoFramePos, 0, Files[Files_Pos]->Glue->VideoFrameCount, 1, &ok);
    if (ok)
    {
        Files[Files_Pos]->Frames_Pos_Set(i);
    }
}

//---------------------------------------------------------------------------
void MainWindow::on_actionToolbar_triggered()
{
    ui->toolBar->setVisible(ui->actionToolbar->isChecked());
}

//---------------------------------------------------------------------------
void MainWindow::on_Toolbar_visibilityChanged(bool visible)
{
    ui->actionToolbar->setChecked(visible);
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
void MainWindow::on_actionGettingStarted_triggered()
{
    Help_GettingStarted();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionHowToUseThisTool_triggered()
{
    Help_HowToUse();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionFilterDescriptions_triggered()
{
    Help_FilterDescriptions();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionPlaybackFilters_triggered()
{
    Help_PlaybackFilters();
}

//---------------------------------------------------------------------------
void MainWindow::on_actionAbout_triggered()
{
    Help_About();
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
void MainWindow::on_check_BRNG_toggled(bool checked)
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
          
            processFile(event->mimeData()->urls()[0].toLocalFile());

            //break; //TEMP: currently only one file
        }
    }
}


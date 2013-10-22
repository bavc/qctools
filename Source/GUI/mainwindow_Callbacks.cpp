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
#include <QColor>
#include <QPixmap>
#include <QLabel>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDialog>
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
#include <qwt_picker_machine.h>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE
#include <GUI/Help.h>
#include <Core/Core.h>
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

//***************************************************************************
// BasicInfo
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::BasicInfo_Finished ()
{
    // Coherency
    if (Files[Files_Pos]->BasicInfo->Frames_Total_Get()==0)
    {
        statusBar()->showMessage("Problem", 10000);
        return;
    }

    //Configuring plots
    createData_Init();

    // Init
    Frames_Total=Files[Files_Pos]->BasicInfo->Frames_Total_Get();
    Frames_Pos=0;
    
    // Pictures
    Pictures_Update(0);
}

//***************************************************************************
// Thumbnails
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::Thumbnails_Updated ()
{
    Pictures_Update(Picture_Main_X);
}

//---------------------------------------------------------------------------
void MainWindow::Thumbnails_Finished ()
{
    Pictures_Update(Picture_Main_X);
}

//***************************************************************************
// Stats
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::Stats_Updated ()
{
    //Configuring plots
    createData_Update();

    Frames_Pos=Files[Files_Pos]->Stats->Frames_Current;

    // Status
    double Stats_Progress=Files[Files_Pos]->Stats->Frames_Progress_Get();
    Ztring Message;
    if (Stats_Progress<1)
    {
        Message+=__T("Parsing frame ")+Ztring().From_Number(Files[Files_Pos]->Stats->Frames_Current);
        if (Files[Files_Pos]->Stats->Frames_Total)
            Message+=__T("/")+Ztring().From_Number(Frames_Total)+__T(" (")+Ztring().From_Number(Stats_Progress*100, 0)+__T("%)");
    }
    double Thumbnails_Progress=Files[Files_Pos]->Thumbnails->Frames_Progress_Get();
    if (Thumbnails_Progress<1)
    {
        if (!Message.empty())
            Message+=__T(", ");
        Message+=__T("Creating thumbnail ")+Ztring().From_Number(Files[Files_Pos]->Thumbnails->Frames_Current);
        if (Files[Files_Pos]->Thumbnails->Frames_Total)
            Message+=__T("/")+Ztring().From_Number(Frames_Total)+__T(" (")+Ztring().From_Number(Thumbnails_Progress*100, 0)+__T("%)");
    }

    statusBar()->showMessage(Message.To_UTF8().c_str());
}

//---------------------------------------------------------------------------
void MainWindow::Stats_Finished ()
{
    //Configuring plots
    createData_Update();

    statusBar()->showMessage("Parsing complete", 10000);
}

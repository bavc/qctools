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
#include <GUI/PerPicture.h>
#include <Core/Core.h>
#include <ZenLib/ZtringListList.h>
#include <ZenLib/Ztring.h>
#include <ZenLib/File.h>
using namespace ZenLib;

//***************************************************************************
// Constants
//***************************************************************************


//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::openFile()
{
    ZtringListList List;
    List.Separator_Set(1, __T(","));
        
    FileName=QFileDialog::getOpenFileName(this, "Open file", "", "Video files (*.avi *.mkv *.mov *.mxf);;Statistic files (*.csv);;All (*.*)");
    if (FileName.isEmpty())
        return;

    processFile();
}

//---------------------------------------------------------------------------
void MainWindow::processFile()
{
    statusBar()->showMessage("Scanning "+QFileInfo(FileName).fileName()+"...");

    // Delete previous
    for (size_t Pos=0; Pos<Files.size(); Pos++)
        delete Files[Pos];    
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            ui->verticalLayout->removeWidget(plots[Type]);
            delete plotsZoomers[Type]; plotsZoomers[Type]=NULL;
            delete plots[Type]; plots[Type]=NULL;
        }
    if (Pictures_Widgets)
    {
        ui->verticalLayout->removeWidget(Pictures_Widgets);
        delete Pictures_Widgets; Pictures_Widgets=NULL;
    }
    if (Control_Widgets)
    {
        ui->verticalLayout->removeWidget(Control_Widgets);
        delete Control_Widgets; Control_Widgets=NULL;
    }
    if (Picture_Main)
    {
        delete Picture_Main; Picture_Main=NULL;
    }

    // Launch analysis
    Files.clear();
    Files.push_back(new PerFile());
    Files_Pos=0;
    Files[0]->Launch(this, FileName);
}

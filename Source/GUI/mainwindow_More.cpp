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
#include <QFileInfo>
#include <QCoreApplication>

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
// Constants
//***************************************************************************


//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void MainWindow::init()
{
    //Data
    x=NULL;
    y=NULL;
    memset(y_Max, 0, sizeof(double));
    
    //Plots
    memset(plots, 0, sizeof(QwtPlot*)*PlotType_Max);
    memset(curves, 0, sizeof(QwtPlotCurve*)*5*PlotType_Max);
    memset(plotsZoomers, 0, sizeof(QwtPlotZoomer*)*PlotType_Max);
    ZoomScale=1;

    //
    ui->actionOpen->setIcon(QIcon(":/icon/document-open.png"));
    ui->actionZoomIn->setIcon(QIcon(":/icon/zoom-in.png"));
    ui->actionZoomOut->setIcon(QIcon(":/icon/zoom-out.png"));
    ui->actionPrint->setIcon(QIcon(":/icon/document-print.png"));
    ui->actionFilterDescriptions->setIcon(QIcon(":/icon/help.png"));

    setWindowTitle("QC Tools");

    configureZoom();
}

//---------------------------------------------------------------------------
void MainWindow::ProcessMessage()
{
    Data+=Process->readAllStandardOutput();

    if (Process_Status==Status_Parsing)
    {
        int Pos;
        for (;;)
        {
            Pos=Data.indexOf('\n', Data_Pos);
            if (Pos==-1)
                break;
            Frames_Pos++;
            Data_Pos=Pos+1;
        }

        if ((Frames_Pos%10)==0)
        {
            //Configuring plots
            createData_Update();

            Ztring Number; Number.From_Number(Frames_Pos);
            Ztring Message=__T("Parsing frame ")+Number;
            if (Frames_Total)
                Message+=__T("/")+Ztring().From_Number(Frames_Total)+__T(" (")+Ztring().From_Number(Frames_Pos*100/Frames_Total)+__T("%)");
            statusBar()->showMessage(Message.To_UTF8().c_str());
        }
    }

}

//---------------------------------------------------------------------------
void MainWindow::ProcessError()
{
    //QByteArray Data = Process->readAllStandardError();
    //Ztring A; A.From_UTF8(Data.data());
    //A.clear();
    //ZtringListList List;
    //List.Separator_Set(1, __T(","));
    //List.Write(Ztring().From_UTF8(Data.data()));
}

//---------------------------------------------------------------------------
void MainWindow::ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ProcessMessage();
    QByteArray ErrorPart = Process->readAllStandardError();
    Ztring ErrorPartZ; ErrorPartZ.From_UTF8(ErrorPart.data());

    if (Process_Status==Status_GettingFrames)
    {
        Ztring Data2 = Ztring().From_UTF8(Data.data());
        Ztring Frames=Data2.SubString(__T("nb_frames="), __T("\n"));
        Frames_Total=Frames.To_int32u();
        Ztring DurationZ=Data2.SubString(__T("duration="), __T("\n"));
        Duration=DurationZ.To_float32();
        Data.clear();

        Frames_Pos=0;
        Frames_Plot_Pos=0;
        Data_Pos=0;
        Data_Plot_Pos=0;

        if (Frames_Total==0)
        {
            statusBar()->showMessage("Problem", 10000);
            return;
        }

        //Configuring plots
        createData_Init();
    
        Process_Status=Status_Parsing;
        QFileInfo FileInfo(FileName);
        
        //Process->kill();
        Process->start(QCoreApplication::applicationDirPath()+"/ffprobe", QStringList() << "-f" << "lavfi" << "movie="+FileInfo.fileName()+",values=stat=tout|vrep|rang|head" << "-show_frames" << "-of" << "csv");
        bool A=Process->waitForStarted();
        return;
    }

    if (Process_Status==Status_Parsing)
    {
        if (exitStatus==QProcess::NormalExit)
        {
            statusBar()->showMessage("OK", 10000);
            //statusBar()->showMessage("Parsing complete, creating plots...");
            //createData();
        }
    }
}

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
    delete Process;
    QFileInfo FileInfo(FileName);
    statusBar()->showMessage("Scanning "+FileInfo.fileName()+"...");
    if (FileInfo.suffix()=="csv")
    {
        QFile F(FileName);
        F.open(QIODevice::ReadOnly);
        Data=F.readAll();
        createData();
        return;
    }

    //Running ffprobe
    Process=new QProcess(this);
    connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(ProcessMessage()));
    connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(ProcessError()));
    connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ProcessFinished(int, QProcess::ExitStatus)));
    Process->setWorkingDirectory(FileInfo.absolutePath());
    Data.clear();
    Data_Pos=0;
    Data_Plot_Pos=0;
    Frames_Pos=0;
    Frames_Plot_Pos=0;
    Process_Status=Status_GettingFrames;
    //Process->kill();
    Process->start(QCoreApplication::applicationDirPath()+"/ffprobe", QStringList() << "-show_format" << "-show_streams" << FileInfo.fileName());
    bool A=Process->waitForStarted();
    return;
}

//---------------------------------------------------------------------------
void MainWindow::configureZoom()
{
    if (Frames_Plot_Pos==0 || ZoomScale==1)
    {
        ui->horizontalScrollBar->setRange(0, 0);
        ui->horizontalScrollBar->setMaximum(0);
        ui->horizontalScrollBar->setPageStep(1);
        ui->horizontalScrollBar->setSingleStep(1);
        ui->horizontalScrollBar->setEnabled(false);
        ui->actionZoomOut->setEnabled(false);
        ui->actionZoomIn->setEnabled(Frames_Plot_Pos);
        ui->actionCSV->setEnabled(Frames_Plot_Pos);
        ui->actionPrint->setEnabled(Frames_Plot_Pos);
        return;
    }

    size_t Increment=Frames_Total/ZoomScale;
    ui->horizontalScrollBar->setMaximum(Frames_Total-Increment);
    ui->horizontalScrollBar->setPageStep(Increment);
    ui->horizontalScrollBar->setSingleStep(Increment);
    ui->horizontalScrollBar->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
    ui->actionCSV->setEnabled(true);
    ui->actionPrint->setEnabled(true);
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Move(size_t Begin)
{
    size_t Increment=Frames_Total/ZoomScale;
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            QwtPlotZoomer* zoomer = new QwtPlotZoomer(plots[Type]->canvas());
            QRectF Rect=zoomer->zoomBase();
            Rect.setLeft(Duration*Begin/Frames_Total);
            Rect.setWidth(Duration*Increment/Frames_Total);
            zoomer->zoom(Rect);
            if (Type<=PlotType_V)
                plots[Type]->setAxisScale(QwtPlot::yLeft, 0, 255, 85);
            plots[Type]->replot();
        }
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_In()
{
    ZoomScale*=2;
    configureZoom();
    Zoom_Move(ui->horizontalScrollBar->value());
}

//---------------------------------------------------------------------------
void MainWindow::Zoom_Out()
{
    if (ZoomScale>1)
        ZoomScale/=2;
    configureZoom();
    Zoom_Move(ui->horizontalScrollBar->value());
}

//---------------------------------------------------------------------------
void MainWindow::Export_CSV()
{
    if (plots[PlotType_Y]==NULL)
        return; //Problem

    QString SaveFileName=QFileDialog::getSaveFileName(this, "Export to CSV", FileName+".qctools_development.csv", "Statistic files (*.csv)");
    if (SaveFileName.isEmpty())
        return;

    QFile F(SaveFileName);
    F.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text);
    F.write(Data);
}

//---------------------------------------------------------------------------
void MainWindow::Export_PDF()
{
    if (plots[PlotType_Y]==NULL)
        return; //Problem

    /*
    QFileDialog dialog;
    dialog.setOptions(QFileDialog::DontUseNativeDialog);  // with or without this
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDirectory(".");
    dialog.selectFile("somefile.txt");   // magic happens here
    if (dialog.exec()!=QDialog::Accepted)
        return;
    QString SaveFileName=dialog.selectedFiles()[0];
    */
    QString SaveFileName=QFileDialog::getSaveFileName(this, "Acrobat Reader file (PDF)", FileName+".qctools_development.pdf", "PDF (*.pdf)");

    if (SaveFileName.isEmpty())
        return;

    /*QPrinter printer(QPrinter::HighResolution);  
    printer.setOutputFormat(QPrinter::PdfFormat);  
    printer.setOutputFileName(FileName);  
    printer.setPageMargins(8,3,3,5,QPrinter::Millimeter);  
    QPainter painter(&printer);  */

    QwtPlotRenderer PlotRenderer;
    PlotRenderer.renderDocument(plots[PlotType_Y], SaveFileName, "PDF", QSizeF(210, 297), 150);
    QDesktopServices::openUrl(QUrl("file:///"+SaveFileName, QUrl::TolerantMode));
}

//---------------------------------------------------------------------------
void MainWindow::refreshDisplay()
{
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            plots[Type]->setVisible(false);
            ui->verticalLayout->removeWidget(plots[Type]);
        }

    //Per checkbox
    bool Status[PlotType_Max];
    Status[PlotType_Y]=ui->check_Y->checkState()==Qt::Checked;
    Status[PlotType_U]=ui->check_U->checkState()==Qt::Checked;
    Status[PlotType_V]=ui->check_V->checkState()==Qt::Checked;
    Status[PlotType_YDiff]=ui->check_YDiff->checkState()==Qt::Checked;
    Status[PlotType_YDiffX]=ui->check_YDiffX->checkState()==Qt::Checked;
    Status[PlotType_UDiff]=ui->check_UDiff->checkState()==Qt::Checked;
    Status[PlotType_VDiff]=ui->check_VDiff->checkState()==Qt::Checked;
    Status[PlotType_Diffs]=ui->check_Diffs->checkState()==Qt::Checked;
    Status[PlotType_TOUT]=ui->check_TOUT->checkState()==Qt::Checked;
    Status[PlotType_VREP]=ui->check_VREP->checkState()==Qt::Checked;
    Status[PlotType_HEAD]=ui->check_HEAD->checkState()==Qt::Checked;
    Status[PlotType_RANG]=ui->check_RANG->checkState()==Qt::Checked;
    PlotType Latest=PlotType_Max;
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (Status[Type])
        {
            if (plots[Type])
                plots[Type]->setVisible(true);
            ui->verticalLayout->addWidget(plots[Type]);
            Latest=(PlotType)Type;
        }

    //RePlot
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
            plots[Type]->enableAxis(QwtPlot::xBottom, Latest==Type);
    
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
            plots[Type]->replot();
}

//---------------------------------------------------------------------------
void MainWindow::createData_Init()
{
    statusBar()->showMessage("Initializing...");

    //Reset
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            ui->verticalLayout->removeWidget(plots[Type]);
            delete plotsZoomers[Type];
            delete plots[Type];
        }
    init();

    if (y)
    {
        delete x;
        for(size_t j=0; j<PlotName_Max; ++j)
            delete[] y[j];
        delete y;
    }

    x = new double[Frames_Total];
    memset(x, 0x00, Frames_Total*sizeof(double));
    y = new double*[PlotName_Max];
    for(size_t j=0; j<PlotName_Max; ++j)
    {
        y[j] = new double[Frames_Total];
        memset(y[j], 0x00, Frames_Total*sizeof(double));
    }

    //Creating data
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type]==NULL)
            createPlot((PlotType)Type);

    //UI
    configureZoom();
    refreshDisplay();

    ui->fileNamesBox->clear();
    ui->fileNamesBox->addItem(FileName);

    y_Max[PlotType_YDiff]=0;
    y_Max[PlotType_YDiffX]=0;
    y_Max[PlotType_UDiff]=0;
    y_Max[PlotType_VDiff]=0;
    y_Max[PlotType_Diffs]=0;
    y_Max[PlotType_TOUT]=0;
    y_Max[PlotType_VREP]=0;
    y_Max[PlotType_HEAD]=0;
    y_Max[PlotType_RANG]=0;
}

//---------------------------------------------------------------------------
void MainWindow::createData_Update()
{
    Ztring B; B.From_UTF8(Data.mid(Data_Plot_Pos, Data_Pos-Data_Plot_Pos).data());
    ZtringListList List;
    List.Separator_Set(1, __T(","));
    List.Write(B);
    for (size_t Pos=0; Pos<List.size(); Pos++)
        if (List[Pos].size()!=PlotName_Begin+PlotName_Max)
            return;


    //i=line number
    //j=column
    for(unsigned i=0; i<Frames_Pos-Frames_Plot_Pos; ++i)
    {
        x[Frames_Plot_Pos+i]=List[i][6].To_float64();
        for(unsigned j=0; j<PlotName_Max; ++j)
        {
            y[j][Frames_Plot_Pos+i]=List[i][PlotName_Begin+j].To_float64();
            switch (j)
            {
                case PlotName_YDIF :    if (y_Max[PlotType_YDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_UDIF :    if (y_Max[PlotType_UDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_UDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_VDIF :    if (y_Max[PlotType_VDiff]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_VDiff]=y[j][Frames_Plot_Pos+i];
                                        if (y_Max[PlotType_Diffs]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_Diffs]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_YDIF1 :   if (y_Max[PlotType_YDiffX]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiffX]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_YDIF2 :   if (y_Max[PlotType_YDiffX]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_YDiffX]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_TOUT :    if (y_Max[PlotType_TOUT]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_TOUT]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_VREP :    if (y_Max[PlotType_VREP]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_VREP]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_RANG :    if (y_Max[PlotType_RANG]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_RANG]=y[j][Frames_Plot_Pos+i];
                                        break;
                case PlotName_HEAD :    if (y_Max[PlotType_HEAD]<y[j][Frames_Plot_Pos+i])
                                            y_Max[PlotType_HEAD]=y[j][Frames_Plot_Pos+i];
                                        break;
                default:                ;
            }
        }
    }

    Frames_Plot_Pos=Frames_Pos;
    Data_Plot_Pos=Data_Pos;
    
    //Creating data
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
            createData_Update((PlotType)Type);

    //UI
    configureZoom();
    refreshDisplay();
}

//---------------------------------------------------------------------------
void MainWindow::createData_Update(PlotType Type)
{
    size_t Position, Count;
    double y_Max_ForThisPlot=0;
    switch(Type)
    {
        default             : Position= 0; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_U     : Position= 5; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_V     : Position=10; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_YDiff : Position=15; Count=1; y_Max_ForThisPlot=y_Max[PlotType_YDiff];break;
        case PlotType_YDiffX: Position=18; Count=2; y_Max_ForThisPlot=y_Max[PlotType_YDiffX];break;
        case PlotType_UDiff : Position=16; Count=1; y_Max_ForThisPlot=y_Max[PlotType_UDiff];break;
        case PlotType_VDiff : Position=17; Count=1; y_Max_ForThisPlot=y_Max[PlotType_VDiff];break;
        case PlotType_Diffs : Position=15; Count=3; y_Max_ForThisPlot=y_Max[PlotType_Diffs];break;
        case PlotType_TOUT  : Position=20; Count=1; y_Max_ForThisPlot=y_Max[PlotType_TOUT];break;
        case PlotType_VREP  : Position=21; Count=1; y_Max_ForThisPlot=y_Max[PlotType_VREP];break;
        case PlotType_HEAD  : Position=23; Count=1; y_Max_ForThisPlot=y_Max[PlotType_HEAD];break;
        case PlotType_RANG  : Position=22; Count=1; y_Max_ForThisPlot=y_Max[PlotType_RANG];break;
        
    }
    
    QStringList units;
    units << "ns" << "ms" << "s" << "min";
    int unit_i=0;
    int divideBy=1,next=10;

    //plot->setMinimumHeight(0);
    if (Count>3)
        plots[Type]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //plot->enableAxis(QwtPlot::yLeft, false);
    if (y_Max_ForThisPlot)
    {
        double StepCount=3;
        if (Type==PlotType_Diffs)
            StepCount=2;
        double Step=floor(y_Max_ForThisPlot/StepCount);
        if (Step==0)
        {
            Step=floor(y_Max_ForThisPlot/StepCount*10)/10;
            if (Step==0)
            {
                Step=floor(y_Max_ForThisPlot/StepCount*100)/100;
                if (Step==0)
                {
                    Step=floor(y_Max_ForThisPlot/StepCount*1000)/1000;
                    if (Step==0)
                    {
                        Step=floor(y_Max_ForThisPlot/StepCount*10000)/10000;
                    }
                }
            }
        }
        if (Step)
            plots[Type]->setAxisScale(QwtPlot::yLeft, 0, y_Max_ForThisPlot, Step);
    }

    for(unsigned j=0; j<5; ++j)
        if (curves[Type][j])
            curves[Type][j]->setRawSamples(x, y[Position+j], Frames_Pos);
}

//---------------------------------------------------------------------------
void MainWindow::createData()
{
    //Reset
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type])
        {
            ui->verticalLayout->removeWidget(plots[Type]);
            delete plotsZoomers[Type];
            delete plots[Type];
        }
    init();

    Ztring B; B.From_UTF8(Data.data());
    ZtringListList List;
    List.Separator_Set(1, __T(","));
    List.Write(B);
    for (size_t Pos=0; Pos<List.size(); Pos++)
        if (List[Pos].size()!=PlotName_Begin+PlotName_Max)
            return;

    Frames_Total=List.size();
    Frames_Plot_Pos=Frames_Total;
    Frames_Pos=Frames_Total;

    if (y)
    {
        delete x;
        for(size_t j=0; j<PlotName_Max; ++j)
            delete[] y[j];
        delete y;
    }

    x = new double[Frames_Total];
    y = new double*[PlotName_Max];
    for(size_t j=0; j<PlotName_Max; ++j)
        y[j] = new double[Frames_Total];

    //i=line number
    //j=column
    y_Max[PlotType_YDiff]=0;
    y_Max[PlotType_YDiffX]=0;
    y_Max[PlotType_UDiff]=0;
    y_Max[PlotType_VDiff]=0;
    y_Max[PlotType_Diffs]=0;
    y_Max[PlotType_TOUT]=0;
    y_Max[PlotType_VREP]=0;
    y_Max[PlotType_HEAD]=0;
    y_Max[PlotType_RANG]=0;
    for(unsigned i=0; i<Frames_Total; ++i)
    {
        x[i]=List[i][6].To_float64();
        for(unsigned j=0; j<PlotName_Max; ++j)
        {
            y[j][i]=List[i][PlotName_Begin+j].To_float64();
            switch (j)
            {
                case PlotName_YDIF :    if (y_Max[PlotType_YDiff]<y[j][i])
                                            y_Max[PlotType_YDiff]=y[j][i];
                                        if (y_Max[PlotType_Diffs]<y[j][i])
                                            y_Max[PlotType_Diffs]=y[j][i];
                                        break;
                case PlotName_UDIF :    if (y_Max[PlotType_UDiff]<y[j][i])
                                            y_Max[PlotType_UDiff]=y[j][i];
                                        if (y_Max[PlotType_Diffs]<y[j][i])
                                            y_Max[PlotType_Diffs]=y[j][i];
                                        break;
                case PlotName_VDIF :    if (y_Max[PlotType_VDiff]<y[j][i])
                                            y_Max[PlotType_VDiff]=y[j][i];
                                        if (y_Max[PlotType_Diffs]<y[j][i])
                                            y_Max[PlotType_Diffs]=y[j][i];
                                        break;
                case PlotName_YDIF1 :   if (y_Max[PlotType_YDiffX]<y[j][i])
                                            y_Max[PlotType_YDiffX]=y[j][i];
                                        break;
                case PlotName_YDIF2 :   if (y_Max[PlotType_YDiffX]<y[j][i])
                                            y_Max[PlotType_YDiffX]=y[j][i];
                                        break;
                case PlotName_TOUT :    if (y_Max[PlotType_TOUT]<y[j][i])
                                            y_Max[PlotType_TOUT]=y[j][i];
                                        break;
                case PlotName_VREP :    if (y_Max[PlotType_VREP]<y[j][i])
                                            y_Max[PlotType_VREP]=y[j][i];
                                        break;
                case PlotName_RANG :    if (y_Max[PlotType_RANG]<y[j][i])
                                            y_Max[PlotType_RANG]=y[j][i];
                                        break;
                case PlotName_HEAD :    if (y_Max[PlotType_HEAD]<y[j][i])
                                            y_Max[PlotType_HEAD]=y[j][i];
                                        break;
                default:                ;
            }
        }
    }

    Duration=x[Frames_Total-1];

    //Creating data
    for (size_t Type=PlotType_Y; Type<PlotType_Max; Type++)
        if (plots[Type]==NULL)
            createPlot((PlotType)Type);

    //UI
    configureZoom();
    refreshDisplay();

    ui->fileNamesBox->clear();
    ui->fileNamesBox->addItem(FileName);
    statusBar()->showMessage("OK", 10000);
}

//---------------------------------------------------------------------------
void MainWindow::createPlot(PlotType Type)
{
    size_t Position, Count;
    double y_Max_ForThisPlot=0;
    switch(Type)
    {
        default             : Position= 0; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_U     : Position= 5; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_V     : Position=10; Count=5; y_Max_ForThisPlot=255; break;
        case PlotType_YDiff : Position=15; Count=1; y_Max_ForThisPlot=y_Max[PlotType_YDiff];break;
        case PlotType_YDiffX: Position=18; Count=2; y_Max_ForThisPlot=y_Max[PlotType_YDiffX];break;
        case PlotType_UDiff : Position=16; Count=1; y_Max_ForThisPlot=y_Max[PlotType_UDiff];break;
        case PlotType_VDiff : Position=17; Count=1; y_Max_ForThisPlot=y_Max[PlotType_VDiff];break;
        case PlotType_Diffs : Position=15; Count=3; y_Max_ForThisPlot=y_Max[PlotType_Diffs];break;
        case PlotType_TOUT  : Position=20; Count=1; y_Max_ForThisPlot=y_Max[PlotType_TOUT];break;
        case PlotType_VREP  : Position=21; Count=1; y_Max_ForThisPlot=y_Max[PlotType_VREP];break;
        case PlotType_HEAD  : Position=23; Count=1; y_Max_ForThisPlot=y_Max[PlotType_HEAD];break;
        case PlotType_RANG  : Position=22; Count=1; y_Max_ForThisPlot=y_Max[PlotType_RANG];break;
        
    }
    
    QwtPlot* plot = new QwtPlot;

    QStringList units;
    units << "ns" << "ms" << "s" << "min";
    int unit_i=0;
    int divideBy=1,next=10;

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin( true );
    grid->enableYMin( true );
    grid->setMajorPen( Qt::darkGray, 0, Qt::DotLine );
    grid->setMinorPen( Qt::gray, 0 , Qt::DotLine );
    grid->attach( plot );

    //plot->setMinimumHeight(0);
    if (Count>3)
        plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //plot->enableAxis(QwtPlot::yLeft, false);
    if (y_Max_ForThisPlot)
    {
        double StepCount=3;
        if (Type==PlotType_Diffs)
            StepCount=2;
        double Step=floor(y_Max_ForThisPlot/StepCount);
        if (Step==0)
        {
            Step=floor(y_Max_ForThisPlot/StepCount*10)/10;
            if (Step==0)
            {
                Step=floor(y_Max_ForThisPlot/StepCount*100)/100;
                if (Step==0)
                {
                    Step=floor(y_Max_ForThisPlot/StepCount*1000)/1000;
                    if (Step==0)
                    {
                        Step=floor(y_Max_ForThisPlot/StepCount*10000)/10000;
                    }
                }
            }
        }
        if (Step)
            plot->setAxisScale(QwtPlot::yLeft, 0, y_Max_ForThisPlot, Step);
    }
    plot->setAxisScale(QwtPlot::xBottom, 0, Duration); //x[Frames_Total-1]
    plot->setAxisMaxMajor(QwtPlot::yLeft, 2);
    plot->setAxisMaxMinor(QwtPlot::yLeft, 1);

    for(unsigned j=0; j<Count; ++j)
    {
        curves[Type][j] = new QwtPlotCurve(Names[Position+j]);
        QColor c;
        
        switch (Count)
        {
             case 1 :
                        c=Qt::black;
                        break;
             case 3 : 
                        switch (j)
                        {
                            case 0: c=Qt::black; break;
                            case 1: c=Qt::darkGray; break;
                            case 2: c=Qt::darkGray; break;
                            default: c=Qt::black;
                        }
                        break;
             case 5 : 
                        switch (j)
                        {
                            case 0: c=Qt::red; break;
                            case 1: c=QColor::fromRgb(0x00, 0x66, 0x00); break; //Qt::green
                            case 2: c=Qt::black; break;
                            case 3: c=Qt::green; break;
                            case 4: c=Qt::red; break;
                            default: c=Qt::black;
                        }
                        break;
            default:    c=Qt::black;
        }

        curves[Type][j]->setPen(c);
        /*
        if (Count==5 && (j==2 || j==3))
            curve->setBrush(Qt::lightGray);
        if (Count==5 && (j==1 || j==4))
            curve->setBrush(Qt::darkGray);
        if (Count==5 && (j==0))
            curve->setBrush(Qt::white);
        */

        curves[Type][j]->setRenderHint(QwtPlotItem::RenderAntialiased);
        curves[Type][j]->setRawSamples(x, y[Position+j], Frames_Total);
        curves[Type][j]->setZ(curves[Type][j]->z()-j);
        curves[Type][j]->attach(plot);
     }

    //adding the legend
    QwtLegend *legend = new QwtLegend;
    //legend->setDefaultItemMode(QwtLegendData::Checkable);
    plot->insertLegend(legend, QwtPlot::RightLegend);

    //same axis
    QwtScaleWidget *scaleWidget = plot->axisWidget(QwtPlot::yLeft);
    scaleWidget->scaleDraw()->setMinimumExtent(40);

    //same legend
    QwtPlotLayout *plotLayout=plot->plotLayout();
    QRectF canvasRect=plotLayout->canvasRect();
    QRectF legendRect=plotLayout->legendRect();
    //plotLayout->setLegendPosition(legendRect);
    QwtLegend* A=(QwtLegend*)plot->legend();
    QWidget* W=new QWidget();
    W->setMinimumWidth(100);
    A->contentsWidget()->layout()->addWidget(W);
    
    // Show the plots

    plots[Type]=plot;
}

//---------------------------------------------------------------------------
void MainWindow::Help_FilterDescriptions()
{
    Help* Frame=new Help(this);
    Frame->show();
}

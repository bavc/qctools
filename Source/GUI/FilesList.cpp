/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "FilesList.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/FileInformation.h"
#include "GUI/mainwindow.h"
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <sstream>
//---------------------------------------------------------------------------

enum statstype
{
    StatsType_None,
    StatsType_Average,
    StatsType_Count,
    StatsType_Count2,
    StatsType_Percent,
};

struct percolumn
{
    statstype       Stats_Type;
    PlotName        Stats_PlotName;
    PlotName        Stats_PlotName2;
    const char*     HeaderName;
    const char*     ToolTip;
};

enum col_names
{
    Col_Processed,
    Col_Yav,
    Col_Yrang,
    Col_Uav,
    Col_Vav,
    Col_TOUTav,
    Col_TOUTc,
    Col_SATb,
    Col_SATi,
    Col_BRNGav,
    Col_BRNGc,
    Col_MSEfY,
    Col_Format,
    Col_StreamCount,
    Col_Duration,
    Col_FileSize,
  //Col_Encoder,
    Col_VideoFormat,
    Col_Width,
    Col_Height,
    Col_DAR,
    Col_PixFormat,
    Col_FrameRate,
    Col_AudioFormat,
  //Col_SampleFormat,
    Col_SamplingRate,
  //Col_ChannelLayout,
  //Col_BitDepth,
    Col_Max
};

percolumn PerColumn[Col_Max]=
{
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Processed",        NULL, },
    { StatsType_Average,    PlotName_YAVG,          PlotName_Max,           "Yav",              "average of Y values", },
    { StatsType_Average,    PlotName_YHIGH,         PlotName_YLOW,          "Yrang",            "average of ( YHIGH - YLOW ), indicative of contrast range", },
    { StatsType_Average,    PlotName_UAVG,          PlotName_Max,           "Uav",              NULL, },
    { StatsType_Average,    PlotName_VAVG,          PlotName_Max,           "Vav",              "average of V values", },
    { StatsType_Average,    PlotName_TOUT,          PlotName_Max,           "TOUTav",           "average of TOUT values", },
    { StatsType_Count,      PlotName_TOUT,          PlotName_Max,           "TOUTc",            "count of TOUT > 0.005", },
    { StatsType_Count,      PlotName_SATMAX,        PlotName_Max,           "SATb",             "count of frames with MAXSAT > 88.7, outside of broadcast color levels", },
    { StatsType_Count2,     PlotName_SATMAX,        PlotName_Max,           "SATi",             "count of frames with MAXSAT > 118.2, illegal YUV color", },
    { StatsType_Percent,    PlotName_BRNG,          PlotName_Max,           "BRNGav",           "percent of frames with BRNG > 0", },
    { StatsType_Count,      PlotName_BRNG,          PlotName_Max,           "BRNGc",            "count of frames with BRNG > 0.02", },
    { StatsType_Count,      PlotName_MSE_y,         PlotName_Max,           "MSEfY",            "count of frames with MSEfY over 1000", },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Format",           NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Streams count",    NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Duration",         NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "File size",        NULL, },
  //{ StatsType_None,       PlotName_Max,           PlotName_Max,           "Encoder",          NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "V. Format",        NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Width",            NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Height",           NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "DAR",              NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Pix Format",       NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Frame rate",       NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "A. Format",        NULL, },
  //{ StatsType_None,       PlotName_Max,           PlotName_Max,           "Sample format",    NULL, },
    { StatsType_None,       PlotName_Max,           PlotName_Max,           "Sampling rate",    NULL, },
  //{ StatsType_None,       PlotName_Max,           PlotName_Max,           "Channel layout",   NULL, },
  //{ StatsType_None,       PlotName_Max,           PlotName_Max,           "Bit depth",        NULL, },
};


//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
FilesList::FilesList(MainWindow* Main_) :
    QTableWidget(Main_),
    Main(Main_)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

//---------------------------------------------------------------------------
FilesList::~FilesList()
{
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void FilesList::showEvent(QShowEvent * Event)
{
    connect(this, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(on_itemClicked(QTableWidgetItem*)));
    connect(this, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(on_itemDoubleClicked(QTableWidgetItem*)));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setColumnCount(Col_Max);
    for (size_t Col=0; Col<Col_Max; Col++)
    {
        QTableWidgetItem* Item=new QTableWidgetItem(PerColumn[Col].HeaderName);
        if (PerColumn[Col].ToolTip)
            Item->setToolTip(PerColumn[Col].ToolTip);
        setHorizontalHeaderItem(Col, Item);
    }

    UpdateAll();
}

//***************************************************************************
// Update
//***************************************************************************

//---------------------------------------------------------------------------
void FilesList::UpdateAll()
{
    QFont Font=QFont();
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32

    setRowCount(Main->Files.size());
    for (size_t Files_Pos=0; Files_Pos<Main->Files.size(); Files_Pos++)
    {
        QString     Format;
        QString     StreamCount;
        QString     FrameRate;
        string      Duration;
        QString     ShortFileName;
        QString     FileSize;
        QString     VideoFormat;
        QString     Width;
        QString     Height;
        QString     DAR_String;
        QString     PixFormat;
        QString     AudioFormat;
        QString     SampleFormat;
        QString     SamplingRate_String;
        QString     ChannelLayout;
        QString     BitDepth_String;

        QFileInfo   FileInfo(Main->Files[Files_Pos]->FileName);

        if (Main->Files[Files_Pos]->Glue)
        {
            // Data from FFmpeg
            Format=                             Main->Files[Files_Pos]->Glue->ContainerFormat_Get().c_str();
            StreamCount=QString::number(        Main->Files[Files_Pos]->Glue->StreamCount_Get());
            int Milliseconds=(int)(             Main->Files[Files_Pos]->Glue->VideoDuration_Get()*1000);
            VideoFormat=                        Main->Files[Files_Pos]->Glue->VideoFormat_Get().c_str();
            Width=QString::number(              Main->Files[Files_Pos]->Glue->Width_Get());
            Height=QString::number(             Main->Files[Files_Pos]->Glue->Height_Get());
            double DAR=                         Main->Files[Files_Pos]->Glue->DAR_Get();
            double FrameRated=                  Main->Files[Files_Pos]->Glue->VideoFrameRate_Get();
            PixFormat=                          Main->Files[Files_Pos]->Glue->PixFormat_Get().c_str();
            AudioFormat=                        Main->Files[Files_Pos]->Glue->AudioFormat_Get().c_str();
            SampleFormat=                       Main->Files[Files_Pos]->Glue->SampleFormat_Get().c_str();
            double SamplingRate=                Main->Files[Files_Pos]->Glue->SamplingRate_Get();
            ChannelLayout=                      Main->Files[Files_Pos]->Glue->ChannelLayout_Get().c_str();
            double BitDepth=                    Main->Files[Files_Pos]->Glue->BitDepth_Get();

            // Parsing
            FrameRate=QString::number(FrameRated, 'f', 3);

            if (Milliseconds)
            {
                int H1=Milliseconds/36000000; Milliseconds%=36000000;
                int H2=Milliseconds/ 3600000; Milliseconds%= 3600000;
                int M1=Milliseconds/  600000; Milliseconds%=  600000;
                int M2=Milliseconds/   60000; Milliseconds%=   60000;
                int S1=Milliseconds/   10000; Milliseconds%=   10000;
                int S2=Milliseconds/    1000; Milliseconds%=    1000;
                int m1=Milliseconds/     100; Milliseconds%=     100;
                int m2=Milliseconds/      10; Milliseconds%=      10;
                int m3=Milliseconds         ;
                Duration.append(1, '0'+H1);
                Duration.append(1, '0'+H2);
                Duration.append(1, ':');
                Duration.append(1, '0'+M1);
                Duration.append(1, '0'+M2);
                Duration.append(1, ':');
                Duration.append(1, '0'+S1);
                Duration.append(1, '0'+S2);
                Duration.append(1, '.');
                Duration.append(1, '0'+m1);
                Duration.append(1, '0'+m2);
                Duration.append(1, '0'+m3);
            }

                 if (DAR>=(float)1.23 && DAR<(float)1.27) DAR_String="5:4";
            else if (DAR>=(float)1.30 && DAR<(float)1.37) DAR_String="4:3";
            else if (DAR>=(float)1.45 && DAR<(float)1.55) DAR_String="3:2";
            else if (DAR>=(float)1.55 && DAR<(float)1.65) DAR_String="16:10";
            else if (DAR>=(float)1.74 && DAR<(float)1.82) DAR_String="16:9";
            else if (DAR>=(float)1.82 && DAR<(float)1.88) DAR_String="1.85:1";
            else if (DAR>=(float)2.15 && DAR<(float)2.22) DAR_String="2.2:1";
            else if (DAR>=(float)2.23 && DAR<(float)2.30) DAR_String="2.25:1";
            else if (DAR>=(float)2.30 && DAR<(float)2.37) DAR_String="2.35:1";
            else if (DAR>=(float)2.37 && DAR<(float)2.45) DAR_String="2.40:1";
            else              DAR_String=QString::number(DAR, 'f', 3);
                 if (SamplingRate==96000)
                SamplingRate_String="96 kHz";
            else if (SamplingRate==88200)
                SamplingRate_String="88.2 kHz";
            else if (SamplingRate==48000)
                SamplingRate_String="48 kHz";
            else if (SamplingRate==44100)
                SamplingRate_String="44.1 kHz";
            else if (SamplingRate==24000)
                SamplingRate_String="24 kHz";
            else if (SamplingRate==22500)
                SamplingRate_String="22.05 kHz";
            else if (SamplingRate)
                SamplingRate_String=QString::number(SamplingRate)+" Hz";
            if (BitDepth)
                BitDepth_String=QString::number(BitDepth)+"-bit";

            FileSize=QString::number(FileInfo.size());
        }


        QTableWidgetItem* VerticalHeaderItem=new QTableWidgetItem(FileInfo.fileName());
        VerticalHeaderItem->setToolTip(Main->Files[Files_Pos]->FileName);
        setVerticalHeaderItem((int)Files_Pos, VerticalHeaderItem);

        setItem((int)Files_Pos, Col_Format,         new QTableWidgetItem(Format));
        setItem((int)Files_Pos, Col_StreamCount,    new QTableWidgetItem(StreamCount));
        setItem((int)Files_Pos, Col_Duration,       new QTableWidgetItem(Duration.c_str()));
        setItem((int)Files_Pos, Col_FileSize,       new QTableWidgetItem(FileSize));
      //setItem((int)Files_Pos, Col_Encoder,        new QTableWidgetItem("(TODO)"));
        setItem((int)Files_Pos, Col_VideoFormat,    new QTableWidgetItem(VideoFormat));
        setItem((int)Files_Pos, Col_Width,          new QTableWidgetItem(Width));
        setItem((int)Files_Pos, Col_Height,         new QTableWidgetItem(Height));
        setItem((int)Files_Pos, Col_DAR,            new QTableWidgetItem(DAR_String));
        setItem((int)Files_Pos, Col_PixFormat,      new QTableWidgetItem(PixFormat));
        setItem((int)Files_Pos, Col_FrameRate,      new QTableWidgetItem(FrameRate));
        setItem((int)Files_Pos, Col_AudioFormat,    new QTableWidgetItem(AudioFormat));
      //setItem((int)Files_Pos, Col_SampleFormat,   new QTableWidgetItem(SampleFormat));
        setItem((int)Files_Pos, Col_SamplingRate,   new QTableWidgetItem(SamplingRate_String));
      //setItem((int)Files_Pos, Col_ChannelLayout,  new QTableWidgetItem(ChannelLayout));
      //setItem((int)Files_Pos, Col_BitDepth,       new QTableWidgetItem(BitDepth_String));

        for (int Pos=0; Pos<Col_Max; Pos++)
        {
            QTableWidgetItem* Item=item((int)Files_Pos, Pos);
            if (Item)
            {
                Item->setFlags(Item->flags()&((Qt::ItemFlags)-1-Qt::ItemIsEditable));
                Item->setFont(Font);
            }
        }

        connect(verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(on_verticalHeaderDoubleClicked(int)));
        connect(verticalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(on_verticalHeaderClicked(int)));
    }

    Update();
}

//---------------------------------------------------------------------------
void FilesList::Update()
{
    for (size_t Files_Pos=0; Files_Pos<Main->Files.size(); Files_Pos++)
    {
        QTableWidgetItem* Item=item((int)Files_Pos, 0);
        if (!Item || Item->text()!="100%")
            Update(Files_Pos);
    }

    resizeColumnsToContents();
}

//---------------------------------------------------------------------------
void FilesList::Update(size_t Files_Pos)
{
    stringstream Message;
    Message<<(int)(Main->Files[Files_Pos]->Videos[0]->State_Get()*100)<<"%";
    setItem((int)Files_Pos, Col_Processed, new QTableWidgetItem(QString::fromStdString(Message.str())));
    
    // Stats
    for (size_t Col=0; Col<Col_Max; Col++)
        if (PerColumn[Col].Stats_Type!=StatsType_None)
        {
            QTableWidgetItem* Item=new QTableWidgetItem();
            switch (PerColumn[Col].Stats_Type)
            {
                case StatsType_Average : 
                                            if (PerColumn[Col].Stats_PlotName2==PlotName_Max)
                                                Item->setText(Main->Files[Files_Pos]->Videos[0]->Average_Get(PerColumn[Col].Stats_PlotName).c_str());
                                            else
                                                Item->setText(Main->Files[Files_Pos]->Videos[0]->Average_Get(PerColumn[Col].Stats_PlotName, PerColumn[Col].Stats_PlotName2).c_str());
                                            break;
                case StatsType_Count : 
                                            Item->setText(Main->Files[Files_Pos]->Videos[0]->Count_Get(PerColumn[Col].Stats_PlotName).c_str());
                                            break;
                case StatsType_Count2 :
                                            Item->setText(Main->Files[Files_Pos]->Videos[0]->Count2_Get(PerColumn[Col].Stats_PlotName).c_str());
                                            break;
                case StatsType_Percent : 
                                            Item->setText(Main->Files[Files_Pos]->Videos[0]->Percent_Get(PerColumn[Col].Stats_PlotName).c_str());
                                            break;
                default:    ;
            }
            Item->setFlags(Item->flags()&((Qt::ItemFlags)-1-Qt::ItemIsEditable));
            setItem((int)Files_Pos, Col, Item);
        }
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void FilesList::contextMenuEvent (QContextMenuEvent* Event)
{
    //Retrieving data
    QTableWidgetItem* Item=itemAt(Event->pos());
    if (Item==NULL)
        return;

    //Creating menu
    QMenu menu(this);
    menu.addAction(new QAction("Show graphs", this)); //If you change this, change the test text too
    menu.addAction(new QAction("Show filters", this)); //If you change this, change the test text too
    menu.addSeparator();
    menu.addAction(new QAction("Close", this)); //If you change this, change the test text too

    //Displaying
    QAction* Action=menu.exec(Event->globalPos());
    if (Action==NULL)
        return;

    //Retrieving data
    QString Text=Action->text();

    //Special cases
    if (Text=="Show graphs") //If you change this, change the creation text too
    {
        Main->selectDisplayFile(Item->row());
        return;
    }
    if (Text=="Show filters") //If you change this, change the creation text too
    {
        Main->selectDisplayFiltersFile(Item->row());
        return;
    }
    if (Text=="Close") //If you change this, change the creation text too
    {
        Main->selectFile(Item->row());
        Main->closeFile();
        UpdateAll();
        return;
    }
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void FilesList::on_itemClicked(QTableWidgetItem * item)
{
    Main->selectFile(item->row());
}

//---------------------------------------------------------------------------
void FilesList::on_itemDoubleClicked(QTableWidgetItem * item)
{
    Main->selectDisplayFile(item->row());
}

//---------------------------------------------------------------------------
void FilesList::on_verticalHeaderClicked(int logicalIndex)
{
    Main->selectFile(logicalIndex);
}

//---------------------------------------------------------------------------
void FilesList::on_verticalHeaderDoubleClicked(int logicalIndex)
{
    Main->selectDisplayFile(logicalIndex);
}

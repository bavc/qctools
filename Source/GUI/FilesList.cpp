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
#include "Core/CommonStats.h"
#include "Core/VideoCore.h"
#include "Core/AudioCore.h"
#include "Core/FFmpeg_Glue.h"
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
    size_t          Stats_Item;
    size_t          Stats_Item2;
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
    Col_BitRate,
    Col_Duration,
    Col_FileSize,
  //Col_Encoder,
    Col_VideoFormat,
    Col_Width,
    Col_Height,
    Col_DAR,
    Col_SAR,
    Col_PixFormat,
    Col_ColorSpace,
    Col_ColorRange,
    Col_FramesDivDuration,
    Col_RFrameRate,
    Col_AvgFrameRate,
    Col_AudioFormat,
  //Col_SampleFormat,
    Col_SamplingRate,
  //Col_ChannelLayout,
  //Col_BitDepth,
    Col_Max
};

percolumn PerColumn[Col_Max]=
{
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Processed",        NULL, },
    { StatsType_Average,    Item_YAVG,              Item_VideoMax,          "Yav",              "average of Y values", },
    { StatsType_Average,    Item_YHIGH,             Item_YLOW,              "Yrang",            "average of ( YHIGH - YLOW ), indicative of contrast range", },
    { StatsType_Average,    Item_UAVG,              Item_VideoMax,          "Uav",              NULL, },
    { StatsType_Average,    Item_VAVG,              Item_VideoMax,          "Vav",              "average of V values", },
    { StatsType_Average,    Item_TOUT,              Item_VideoMax,          "TOUTav",           "average of TOUT values", },
    { StatsType_Count,      Item_TOUT,              Item_VideoMax,          "TOUTc",            "count of TOUT > 0.005", },
    { StatsType_Count,      Item_SATMAX,            Item_VideoMax,          "SATb",             "count of frames with MAXSAT > 88.7, outside of broadcast color levels", },
    { StatsType_Count2,     Item_SATMAX,            Item_VideoMax,          "SATi",             "count of frames with MAXSAT > 118.2, illegal YUV color", },
    { StatsType_Average,    Item_BRNG,              Item_VideoMax,          "BRNGav",           "average of BRNG", },
    { StatsType_Count,      Item_BRNG,              Item_VideoMax,          "BRNGc",            "count of frames with BRNG > 0.02", },
    { StatsType_Count,      Item_MSE_y,             Item_VideoMax,          "MSEfY",            "count of frames with MSEfY over 1000", },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Format",           NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Streams count",    NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Bit Rate",         NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Duration",         NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "File size",        NULL, },
  //{ StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Encoder",          NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Video Format",     NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Width",            NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Height",           NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "DAR",              NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "SAR",              NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Pix Format",       NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Color Space",      NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Color Range",      NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Frames/Dur",       NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "R Frame rate",     NULL, },
    { StatsType_None,       Item_VideoMax,          Item_VideoMax,          "Avg Frame rate",   NULL, },
    { StatsType_None,       Item_AudioMax,          Item_AudioMax,          "Audio Format",     NULL, },
  //{ StatsType_None,       Item_AudioMax,          Item_AudioMax,          "Sample format",    NULL, },
    { StatsType_None,       Item_AudioMax,          Item_AudioMax,          "Sampling rate",    "measured in Hz", },
  //{ StatsType_None,       Item_AudioMax,          Item_AudioMax,          "Channel layout",   NULL, },
  //{ StatsType_None,       Item_AudioMax,          Item_AudioMax,          "Bit depth",        NULL, },
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
        QString     BitRate;
        QString     FramesDivDuration;
        QString     RFrameRate;
        QString     AvgFrameRate;
        string      Duration;
        QString     ShortFileName;
        QString     FileSize;
        QString     VideoFormat;
        QString     Width;
        QString     Height;
        QString     DAR_String;
        QString     SAR;
        QString     PixFormat;
        QString     ColorSpace;
        QString     ColorRange;
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
            BitRate=QString::number(            Main->Files[Files_Pos]->Glue->BitRate_Get());
            int Milliseconds=(int)(             Main->Files[Files_Pos]->Glue->VideoDuration_Get()*1000);
            VideoFormat=                        Main->Files[Files_Pos]->Glue->VideoFormat_Get().c_str();
            Width=QString::number(              Main->Files[Files_Pos]->Glue->Width_Get());
            Height=QString::number(             Main->Files[Files_Pos]->Glue->Height_Get());
            double DAR=                         Main->Files[Files_Pos]->Glue->DAR_Get();
            SAR=                                Main->Files[Files_Pos]->Glue->SAR_Get().c_str();
            double FramesDivDurationd=          Main->Files[Files_Pos]->Glue->FramesDivDuration_Get();
            RFrameRate=                         Main->Files[Files_Pos]->Glue->RVideoFrameRate_Get().c_str();
            AvgFrameRate=                       Main->Files[Files_Pos]->Glue->AvgVideoFrameRate_Get().c_str();
            PixFormat=                          Main->Files[Files_Pos]->Glue->PixFormat_Get().c_str();
            ColorSpace=                         Main->Files[Files_Pos]->Glue->ColorSpace_Get().c_str();
            ColorRange=                         Main->Files[Files_Pos]->Glue->ColorRange_Get().c_str();
            AudioFormat=                        Main->Files[Files_Pos]->Glue->AudioFormat_Get().c_str();
            SampleFormat=                       Main->Files[Files_Pos]->Glue->SampleFormat_Get().c_str();
            double SamplingRate=                Main->Files[Files_Pos]->Glue->SamplingRate_Get();
            ChannelLayout=                      Main->Files[Files_Pos]->Glue->ChannelLayout_Get().c_str();
            double BitDepth=                    Main->Files[Files_Pos]->Glue->BitDepth_Get();

            // Parsing
            FramesDivDuration=QString::number(FramesDivDurationd, 'f', 3);

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

            DAR_String=QString::number(DAR, 'f', 4);
            if (SamplingRate)
                SamplingRate_String=QString::number(SamplingRate);
            if (BitDepth)
                BitDepth_String=QString::number(BitDepth)+"-bit";

            FileSize=QString::number(FileInfo.size());
        }


        QTableWidgetItem* VerticalHeaderItem=new QTableWidgetItem(FileInfo.fileName());
        VerticalHeaderItem->setToolTip(Main->Files[Files_Pos]->FileName);
        setVerticalHeaderItem((int)Files_Pos, VerticalHeaderItem);

        setItem((int)Files_Pos, Col_Format,         new QTableWidgetItem(Format));
        setItem((int)Files_Pos, Col_StreamCount,    new QTableWidgetItem(StreamCount));
        setItem((int)Files_Pos, Col_BitRate,        new QTableWidgetItem(BitRate));
        setItem((int)Files_Pos, Col_Duration,       new QTableWidgetItem(Duration.c_str()));
        setItem((int)Files_Pos, Col_FileSize,       new QTableWidgetItem(FileSize));
      //setItem((int)Files_Pos, Col_Encoder,        new QTableWidgetItem("(TODO)"));
        setItem((int)Files_Pos, Col_VideoFormat,    new QTableWidgetItem(VideoFormat));
        setItem((int)Files_Pos, Col_Width,          new QTableWidgetItem(Width));
        setItem((int)Files_Pos, Col_Height,         new QTableWidgetItem(Height));
        setItem((int)Files_Pos, Col_DAR,            new QTableWidgetItem(DAR_String));
        setItem((int)Files_Pos, Col_SAR,            new QTableWidgetItem(SAR));
        setItem((int)Files_Pos, Col_PixFormat,      new QTableWidgetItem(PixFormat));
        setItem((int)Files_Pos, Col_ColorSpace,     new QTableWidgetItem(ColorSpace));
        setItem((int)Files_Pos, Col_ColorRange,     new QTableWidgetItem(ColorRange));
        setItem((int)Files_Pos, Col_FramesDivDuration, new QTableWidgetItem(FramesDivDuration));
        setItem((int)Files_Pos, Col_RFrameRate,     new QTableWidgetItem(RFrameRate));
        setItem((int)Files_Pos, Col_AvgFrameRate,   new QTableWidgetItem(AvgFrameRate));
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
    Message<<(int)(Main->Files[Files_Pos]->ReferenceStat()->State_Get()*100)<<"%";
    setItem((int)Files_Pos, Col_Processed, new QTableWidgetItem(QString::fromStdString(Message.str())));
    
    // Stats
    for (size_t Col=0; Col<Col_Max; Col++)
        if (PerColumn[Col].Stats_Type!=StatsType_None)
        {
            QTableWidgetItem* Item=new QTableWidgetItem();
            switch (PerColumn[Col].Stats_Type)
            {
                case StatsType_Average : 
                                            if (PerColumn[Col].Stats_Item2==Item_VideoMax)
                                                Item->setText(Main->Files[Files_Pos]->ReferenceStat()->Average_Get(PerColumn[Col].Stats_Item).c_str());
                                            else
                                                Item->setText(Main->Files[Files_Pos]->ReferenceStat()->Average_Get(PerColumn[Col].Stats_Item, PerColumn[Col].Stats_Item2).c_str());
                                            break;
                case StatsType_Count : 
                                            Item->setText(Main->Files[Files_Pos]->ReferenceStat()->Count_Get(PerColumn[Col].Stats_Item).c_str());
                                            break;
                case StatsType_Count2 :
                                            Item->setText(Main->Files[Files_Pos]->ReferenceStat()->Count2_Get(PerColumn[Col].Stats_Item).c_str());
                                            break;
                case StatsType_Percent : 
                                            Item->setText(Main->Files[Files_Pos]->ReferenceStat()->Percent_Get(PerColumn[Col].Stats_Item).c_str());
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

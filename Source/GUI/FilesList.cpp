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

enum col_names
{
    Col_Processed,
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
    Col_SampleFormat,
    Col_SamplingRate,
    Col_ChannelLayout,
    Col_BitDepth,
    Col_Max
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
    setHorizontalHeaderItem(Col_Processed,      new QTableWidgetItem("Processed"));
    setHorizontalHeaderItem(Col_Format,         new QTableWidgetItem("Format"));
    setHorizontalHeaderItem(Col_StreamCount,    new QTableWidgetItem("Streams count"));
    setHorizontalHeaderItem(Col_Duration,       new QTableWidgetItem("Duration"));
    setHorizontalHeaderItem(Col_FileSize,       new QTableWidgetItem("File size"));
    //setHorizontalHeaderItem(Col_Encoder,        new QTableWidgetItem("Encoder"));
    setHorizontalHeaderItem(Col_VideoFormat,    new QTableWidgetItem("V. Format"));
    setHorizontalHeaderItem(Col_Width,          new QTableWidgetItem("Width"));
    setHorizontalHeaderItem(Col_Height,         new QTableWidgetItem("Height"));
    setHorizontalHeaderItem(Col_DAR,            new QTableWidgetItem("DAR"));
    setHorizontalHeaderItem(Col_PixFormat,      new QTableWidgetItem("Pix Format"));
    setHorizontalHeaderItem(Col_FrameRate,      new QTableWidgetItem("Frame rate"));
    setHorizontalHeaderItem(Col_AudioFormat,    new QTableWidgetItem("A. Format"));
    setHorizontalHeaderItem(Col_SampleFormat,   new QTableWidgetItem("Sample format"));
    setHorizontalHeaderItem(Col_SamplingRate,   new QTableWidgetItem("Sampling rate"));
    setHorizontalHeaderItem(Col_ChannelLayout,  new QTableWidgetItem("Channel layout"));
    setHorizontalHeaderItem(Col_BitDepth,       new QTableWidgetItem("Bit depth"));

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
        int Milliseconds;
        double FrameRate=Main->Files[Files_Pos]->Glue->VideoFrameCount/Main->Files[Files_Pos]->Glue->VideoDuration;
        Milliseconds=(int)((Main->Files[Files_Pos]->Glue->VideoFrameCount/FrameRate*1000)+0.5); //Rounding
        string Time;
        int H1=Milliseconds/36000000; Milliseconds%=36000000;
        int H2=Milliseconds/ 3600000; Milliseconds%= 3600000;
        int M1=Milliseconds/  600000; Milliseconds%=  600000;
        int M2=Milliseconds/   60000; Milliseconds%=   60000;
        int S1=Milliseconds/   10000; Milliseconds%=   10000;
        int S2=Milliseconds/    1000; Milliseconds%=    1000;
        int m1=Milliseconds/     100; Milliseconds%=     100;
        int m2=Milliseconds/      10; Milliseconds%=      10;
        int m3=Milliseconds         ;
        Time.append(1, '0'+H1);
        Time.append(1, '0'+H2);
        Time.append(1, ':');
        Time.append(1, '0'+M1);
        Time.append(1, '0'+M2);
        Time.append(1, ':');
        Time.append(1, '0'+S1);
        Time.append(1, '0'+S2);
        Time.append(1, '.');
        Time.append(1, '0'+m1);
        Time.append(1, '0'+m2);
        Time.append(1, '0'+m3);

        double DAR=Main->Files[Files_Pos]->Glue->DAR_Get();
        QString DAR_String;
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
        double SamplingRate=Main->Files[Files_Pos]->Glue->SamplingRate_Get();
        QString SamplingRate_String;
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
        double BitDepth=Main->Files[Files_Pos]->Glue->BitDepth_Get();
        QString BitDepth_String;
        if (BitDepth)
            BitDepth_String=QString::number(BitDepth)+"-bit";

        QFileInfo FileInfo(Main->Files[Files_Pos]->FileName);
        QTableWidgetItem* VerticalHeaderItem=new QTableWidgetItem(FileInfo.fileName());
        VerticalHeaderItem->setToolTip(Main->Files[Files_Pos]->FileName);
        setVerticalHeaderItem((int)Files_Pos, VerticalHeaderItem);
        setItem((int)Files_Pos, Col_Format,         new QTableWidgetItem(Main->Files[Files_Pos]->Glue->ContainerFormat_Get().c_str()));
        setItem((int)Files_Pos, Col_StreamCount,    new QTableWidgetItem(QString::number(Main->Files[Files_Pos]->Glue->StreamCount_Get())));
        setItem((int)Files_Pos, Col_Duration,       new QTableWidgetItem(Time.c_str()));
        setItem((int)Files_Pos, Col_FileSize,       new QTableWidgetItem(QString::number(FileInfo.size())));
        //setItem((int)Files_Pos, Col_Encoder,        new QTableWidgetItem("(TODO)"));
        setItem((int)Files_Pos, Col_VideoFormat,    new QTableWidgetItem(Main->Files[Files_Pos]->Glue->VideoFormat_Get().c_str()));
        setItem((int)Files_Pos, Col_Width,          new QTableWidgetItem(QString::number(Main->Files[Files_Pos]->Glue->Width_Get())));
        setItem((int)Files_Pos, Col_Height,         new QTableWidgetItem(QString::number(Main->Files[Files_Pos]->Glue->Height_Get())));
        setItem((int)Files_Pos, Col_DAR,            new QTableWidgetItem(DAR_String));
        setItem((int)Files_Pos, Col_PixFormat,      new QTableWidgetItem(Main->Files[Files_Pos]->Glue->PixFormat_Get().c_str()));
        setItem((int)Files_Pos, Col_FrameRate,      new QTableWidgetItem(QString::number(FrameRate, 'f', 3)));
        setItem((int)Files_Pos, Col_AudioFormat,    new QTableWidgetItem(Main->Files[Files_Pos]->Glue->AudioFormat_Get().c_str()));
        setItem((int)Files_Pos, Col_SampleFormat,   new QTableWidgetItem(Main->Files[Files_Pos]->Glue->SampleFormat_Get().c_str()));
        setItem((int)Files_Pos, Col_SamplingRate,   new QTableWidgetItem(SamplingRate_String));
        setItem((int)Files_Pos, Col_ChannelLayout,  new QTableWidgetItem(Main->Files[Files_Pos]->Glue->ChannelLayout_Get().c_str()));
        setItem((int)Files_Pos, Col_BitDepth,       new QTableWidgetItem(BitDepth_String));

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
        if (Main->Files[Files_Pos]->Glue->VideoFrameCount)
        {
            stringstream Message;
            Message<<(int)((double)Main->Files[Files_Pos]->Glue->VideoFramePos)*100/Main->Files[Files_Pos]->Glue->VideoFrameCount<<"%";
            setItem((int)Files_Pos, 0, new QTableWidgetItem(QString::fromStdString(Message.str())));
            QTableWidgetItem* Item=item((int)Files_Pos, 0);
            if (Item)
                Item->setFlags(Item->flags()&((Qt::ItemFlags)-1-Qt::ItemIsEditable));
        }

    resizeColumnsToContents();
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

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "GUI/Control.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/TinyDisplay.h"
#include "GUI/Info.h"
#include "Core/FileInformation.h"
#include "Core/CommonStats.h"
#include "Core/FFmpeg_Glue.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QToolButton>
#include <QPushButton>
#include <QTimer>
#include <QGridLayout>
#include <QLabel>
#include <QTime>
#include <QApplication>
#include <cfloat>
#include <QDebug>
#include <QTime>
#include <math.h> // for floor on mac
#include <QCheckBox>
#include <QClipboard>
#include <QShortcut>
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
Control::Control(QWidget *parent, FileInformation* FileInformationData_, bool IsSlave_):
    QWidget(parent),
    FileInfoData(FileInformationData_),
    IsSlave(IsSlave_),
    playAllFrames(true)
{
    // To update
    TinyDisplayArea=NULL;
    InfoArea=NULL;

    // Positioning info
    Frames_Pos=0;

    QFont Font=QFont();
    #ifdef _WIN32
        //Font.setPointSize(8);
    #else //_WIN32
        //Font.setPointSize(8);
    #endif //_WIN32
    Font.setPointSize(Font.pointSize()*3/2);

    // Control
    QGridLayout* Layout=new QGridLayout();
    Layout->setSpacing(0);
    Layout->setMargin(0);
    Layout->setContentsMargins(0,0,0,0);

    Info_Time=new QLabel(this);
    Info_Time->setFont(Font);
    Info_Time->setAlignment(Qt::AlignCenter);
    Layout->addWidget(Info_Time, 0, 0, 1, 1);

    M9=new QToolButton(this);
    M9->setText("<|");
    M9->setFont(Font);
    M9->setIcon(QIcon(":/icon/backward.png"));
    M9->setIconSize(QSize(23, 23));
    connect(M9, SIGNAL(clicked(bool)), this, SLOT(on_M9_clicked(bool)));
    Layout->addWidget(M9, 0, 1, 1, 1);

    M2=new QToolButton(this);
    M2->setText("-2x");
    M2->setFont(Font);
    connect(M2, SIGNAL(clicked(bool)), this, SLOT(on_M2_clicked(bool)));
    Layout->addWidget(M2, 0, 2, 1, 1);

    M1=new QToolButton(this);
    M1->setText("-1x");
    M1->setFont(Font);
    connect(M1, SIGNAL(clicked(bool)), this, SLOT(on_M1_clicked(bool)));
    Layout->addWidget(M1, 0, 3, 1, 1);

    M0=new QToolButton(this);
    M0->setText("-.5x");
    M0->setFont(Font);
    connect(M0, SIGNAL(clicked(bool)), this, SLOT(on_M0_clicked(bool)));
    Layout->addWidget(M0, 0, 4, 1, 1);

    Minus=new QToolButton(this);
    connect(Minus, SIGNAL(clicked(bool)), this, SLOT(on_Minus_clicked(bool)));
    Minus->setText("Prev");
    Minus->setFont(Font);
    Layout->addWidget(Minus, 0, 5, 1, 1);

    PlayPause=new QToolButton(this);
    PlayPause->setText(">");
    PlayPause->setFont(Font);
    PlayPause->setIcon(QIcon(":/icon/play.png"));
    PlayPause->setIconSize(QSize(23, 23));
    connect(PlayPause, SIGNAL(clicked(bool)), this, SLOT(on_PlayPause_clicked(bool)));

    Layout->addWidget(PlayPause, 0, 6, 1, 1);

    Pause=new QToolButton(this);
    Pause->setVisible(false);
    connect(Pause, SIGNAL(clicked(bool)), this, SLOT(on_Pause_clicked(bool)));

    Plus=new QToolButton(this);
    connect(Plus, SIGNAL(clicked(bool)), this, SLOT(on_Plus_clicked(bool)));
    Plus->setText("Next");
    Plus->setFont(Font);
    Layout->addWidget(Plus, 0, 7, 1, 1);

    P0=new QToolButton(this);
    P0->setText("+.5x");
    P0->setFont(Font);
    connect(P0, SIGNAL(clicked(bool)), this, SLOT(on_P0_clicked(bool)));
    Layout->addWidget(P0, 0, 8, 1, 1);

    P1=new QToolButton(this);
    P1->setText("+1x");
    P1->setFont(Font);
    connect(P1, SIGNAL(clicked(bool)), this, SLOT(on_P1_clicked(bool)));
    Layout->addWidget(P1, 0, 9, 1, 1);

    P2=new QToolButton(this);
    P2->setText("+2x");
    P2->setFont(Font);
    connect(P2, SIGNAL(clicked(bool)), this, SLOT(on_P2_clicked(bool)));
    Layout->addWidget(P2, 0, 10, 1, 1);

    P9=new QToolButton(this);
    P9->setText("|>");
    P9->setFont(Font);
    P9->setIcon(QIcon(":/icon/forward.png"));
    P9->setIconSize(QSize(23, 23));
    connect(P9, SIGNAL(clicked(bool)), this, SLOT(on_P9_clicked(bool)));
    Layout->addWidget(P9, 0, 11, 1, 1);

    Info_Frames=new QLabel(this);
    Info_Frames->setFont(Font);
    Info_Frames->setAlignment(Qt::AlignCenter);
    Layout->addWidget(Info_Frames, 0, 12, 1, 1);

    setLayout(Layout);

    Timer=NULL;
    Thread = NULL;
    SelectedSpeed=Speed_O;

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;

    // Update();

    QShortcut *shortcutCopy = new QShortcut(QKeySequence(QKeySequence::Copy), this);
    QObject::connect(shortcutCopy, SIGNAL(activated()), this, SLOT(copyTimeStamp()));
}

//---------------------------------------------------------------------------
void Control::stop()
{
    if(Thread)
    {
        qDebug() << "Thread->requestInterruption()";
        Thread->requestInterruption();

        while (Timer->isActive()) {
            qDebug() << "timer is still active...";
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        Thread->quit();
        Thread = nullptr;
        Timer = nullptr;
    }
}

Control::~Control()
{
    stop();
}

//***************************************************************************
// Actions
//***************************************************************************
static QTime zeroTime = QTime::fromString("00:00:00");

//---------------------------------------------------------------------------
void Control::Update()
{
    if(this->thread() != QThread::currentThread())
    {
        // qDebug() << "Control::Update: called from non-UI thread";
        QMetaObject::invokeMethod(this, "Update");
        return;

    } else {
        // qDebug() << "Control::Update: called from UI thread";
    }

    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;

    int Milliseconds=(int)-1;
    if (FileInfoData && !FileInfoData->Stats.empty()
     && ( Frames_Pos<FileInfoData->ReferenceStat()->x_Current
      || (Frames_Pos<FileInfoData->ReferenceStat()->x_Current_Max && FileInfoData->ReferenceStat()->x[1][Frames_Pos]))) //Also includes when stats are not ready but timestamp is available
        Milliseconds=(int)(FileInfoData->ReferenceStat()->x[1][Frames_Pos]*1000);
    else
    {
        double TimeStamp = FileInfoData->Glue->TimeStampOfCurrentFrame(0);
        if (TimeStamp!=DBL_MAX)
            Milliseconds=(int)(TimeStamp*1000);
    }

    if (Frames_Pos!=(int)-1)
        Info_Frames->setText(QString("Frame %1 [%2]").arg(Frames_Pos).arg(FileInfoData->Frame_Type_Get()));
    else
        Info_Frames->setText(QString());

    if (Milliseconds >= 0)
    {
        QTime time = zeroTime;
        time = time.addMSecs(Milliseconds);
        QString timeString = time.toString("hh:mm:ss.zzz");
        Info_Time->setText(timeString);
    }
    else
        Info_Time->setText("");

    // Controls
    if (Frames_Pos==0)
    {
        stop();

        M2->setEnabled(false);
        M1->setEnabled(false);
        M0->setEnabled(false);
        Minus->setEnabled(false);
        PlayPause->setText(">");
        PlayPause->setIcon(QIcon(":/icon/play.png"));
        PlayPause->setEnabled(true);
        Plus->setEnabled(true);
        P0->setEnabled(true);
        P1->setEnabled(true);
        P2->setEnabled(true);
    }
    else if (Frames_Pos+1==FileInfoData->ReferenceStat()->x_Current_Max)
    {
        stop();

        M2->setEnabled(true);
        M1->setEnabled(true);
        M0->setEnabled(true);
        Minus->setEnabled(true);
        PlayPause->setText(">");
        PlayPause->setIcon(QIcon(":/icon/play.png"));
        PlayPause->setEnabled(false);
        Plus->setEnabled(false);
        P0->setEnabled(false);
        P1->setEnabled(false);
        P2->setEnabled(false);
    }
    else
    {
        if (SelectedSpeed==Speed_O && Frames_Pos)
        {
            M2->setEnabled(true);
            M1->setEnabled(true);
            M0->setEnabled(true);
            Minus->setEnabled(true);
            PlayPause->setText(">");
            PlayPause->setIcon(QIcon(":/icon/play.png"));
            PlayPause->setEnabled(true);
        }
        if (SelectedSpeed==Speed_O && Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max)
        {
            PlayPause->setText(">");
            PlayPause->setIcon(QIcon(":/icon/play.png"));
            PlayPause->setEnabled(true);
            Plus->setEnabled(true);
            P0->setEnabled(true);
            P1->setEnabled(true);
            P2->setEnabled(true);
        }
    }
}

size_t Control::getCurrentFrame() const
{
    return FileInfoData->Frames_Pos_Get();
}

void Control::setPlayAllFrames(bool value)
{
    playAllFrames = value;
}

bool Control::getPlayAllFrames() const
{
    return playAllFrames;
}

//---------------------------------------------------------------------------
void Control::on_Minus_clicked(bool checked)
{
    if (IsSlave)
        return;

    FileInfoData->Frames_Pos_Minus();
    Q_EMIT currentFrameChanged();
}

//---------------------------------------------------------------------------
void Control::on_Plus_clicked(bool checked)
{
    if (IsSlave)
        return;

    FileInfoData->Frames_Pos_Plus();
    Q_EMIT currentFrameChanged();
}

//---------------------------------------------------------------------------
void Control::on_M9_clicked(bool checked)
{
    if (IsSlave)
        return;

    FileInfoData->Frames_Pos_Set(0);
    Q_EMIT currentFrameChanged();
}

//---------------------------------------------------------------------------
void Control::on_M2_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_M2;
    Timer_Duration=17;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(false);
    M1->setEnabled(true);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_M1_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_M1;
    Timer_Duration=33;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(false);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_M0_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_M0;
    Timer_Duration=67;
    Time_MinusPlus=false;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(true);

    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_PlayPause_clicked(bool checked)
{
    qDebug() << "total frames: " << FileInfoData->ReferenceStat()->x_Current_Max;

    if (SelectedSpeed==Control::Speed_O)
        on_P1_clicked(checked);
    else
        on_Pause_clicked(checked);
}

//---------------------------------------------------------------------------
void Control::on_Pause_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_O;
    Minus->setEnabled(Frames_Pos);
    Plus->setEnabled(Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max);
    M2->setEnabled(Frames_Pos);
    M1->setEnabled(Frames_Pos);
    M0->setEnabled(Frames_Pos);
    PlayPause->setText(">");
    PlayPause->setIcon(QIcon(":/icon/play.png"));
    PlayPause->setEnabled(Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max);
    P0->setEnabled(Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max);
    P1->setEnabled(Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max);
    P2->setEnabled(Frames_Pos+1!=FileInfoData->ReferenceStat()->x_Current_Max);

    Q_EMIT stopClicked();
    stop();
}

//---------------------------------------------------------------------------
void Control::on_P0_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_P0;
    Timer_Duration=67;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(false);
    P1->setEnabled(true);
    P2->setEnabled(true);

    Q_EMIT playClicked();
    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_P1_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_P1;
    Timer_Duration=33;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(false);
    P2->setEnabled(true);

    Q_EMIT playClicked();
    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_P2_clicked(bool checked)
{
    if (IsSlave)
        return;

    SelectedSpeed=Speed_P2;
    Timer_Duration=17;
    Time_MinusPlus=true;
    Minus->setEnabled(false);
    Plus->setEnabled(false);
    M2->setEnabled(true);
    M1->setEnabled(true);
    M0->setEnabled(true);
    PlayPause->setText("||");
    PlayPause->setIcon(QIcon(":/icon/pause.png"));
    PlayPause->setEnabled(true);
    P0->setEnabled(true);
    P1->setEnabled(true);
    P2->setEnabled(false);

    Q_EMIT playClicked();
    TimeOut_Init();
}

//---------------------------------------------------------------------------
void Control::on_P9_clicked(bool checked)
{
    if (IsSlave)
        return;

    if (FileInfoData->ReferenceStat()->x_Current_Max)
    {
        FileInfoData->Frames_Pos_Set(FileInfoData->ReferenceStat()->x_Current_Max-1);
        Q_EMIT currentFrameChanged();
    }
}

void Control::copyTimeStamp()
{
    qApp->clipboard()->setText(Info_Time->text());
}

void Control::setCurrentFrame(size_t frame)
{
    if (IsSlave)
        return;

    FileInfoData->Frames_Pos_Set(frame);
    Q_EMIT currentFrameChanged();
}

void Control::rewind(int frame)
{
    Time = QTime();

    startFrame = frame;
    startFrameTimeStamp = QTime::currentTime();
    lastRenderedFrame = -1;

    setCurrentFrame(frame);
}

//***************************************************************************
// Time
//***************************************************************************

//---------------------------------------------------------------------------
void Control::TimeOut_Init()
{
    /*
    stop();

    Time = QTime();

    startFrame = FileInfoData->Frames_Pos_Get();
    startFrameTimeStamp = QTime::currentTime();
    lastRenderedFrame = -1;

    auto averageFrameRate = FileInfoData->averageFrameRate();
    averageFrameDuration = averageFrameRate != 0.0 ? 1000.0 / averageFrameRate : 0.0;

    if (!Timer)
    {
        Thread = new QThread(this);
        connect(Thread, SIGNAL(finished()), Thread, SLOT(deleteLater()));

        Timer = new QTimer();
        connect(Thread, SIGNAL(started()), Timer, SLOT(start()));
        connect(Thread, SIGNAL(finished()), Timer, SLOT(stop()));
        connect(Thread, SIGNAL(finished()), Timer, SLOT(deleteLater()));

        connect(Timer, SIGNAL(timeout()), this, SLOT(TimeOut()), Qt::DirectConnection);

        if (playAllFrames)
        {
            if(SelectedSpeed == Speed_P0 || SelectedSpeed == Speed_M0)
                Timer_Duration = ceil(averageFrameRate * 2);
            else if(SelectedSpeed == Speed_P1 || SelectedSpeed == Speed_M1)
                Timer_Duration = ceil(averageFrameRate);
            else if(SelectedSpeed == Speed_P2 || SelectedSpeed == Speed_M2)
                Timer_Duration = ceil(averageFrameRate / 2);

            Timer->setInterval(Timer_Duration);
        } else
        {
            Timer->setInterval(averageFrameDuration / 3);
        }

        Timer->moveToThread(Thread);
        Thread->start();
    }
    */
}

//---------------------------------------------------------------------------
void Control::TimeOut ()
{
    /*
    // qDebug() << "threadId: " << QThread::currentThreadId();

    if (Time_MinusPlus)
    {
        if (Frames_Pos+1==FileInfoData->ReferenceStat()->x_Current_Max)
        {
            SelectedSpeed=Speed_O;
            if(TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;

            Timer->stop();
            return;
        }
    }
    else
    {
        if (Frames_Pos==0)
        {
			SelectedSpeed=Speed_O;
            if(TinyDisplayArea && TinyDisplayArea->BigDisplayArea)
                TinyDisplayArea->BigDisplayArea->ControlArea->SelectedSpeed=SelectedSpeed;

            Timer->stop();
            return;
        }
    }

    if(lastRenderedFrame == -1)
    {
        lastRenderedFrame = startFrame;
        lastRenderedFrameTimeStamp = QTime::currentTime();
        setCurrentFrame(startFrame);
    }
    else
    {
        if(playAllFrames)
        {
			if (SelectedSpeed == Speed_M2 || SelectedSpeed == Speed_M1 || SelectedSpeed == Speed_M0)
				setCurrentFrame(--lastRenderedFrame);
			else if (SelectedSpeed == Speed_P2 || SelectedSpeed == Speed_P1|| SelectedSpeed == Speed_P0)
				setCurrentFrame(++lastRenderedFrame);
        } else {

            auto currentTime = QTime::currentTime();
            auto timeFromStartFrame = startFrameTimeStamp.msecsTo(currentTime);

            auto framesFromStartFrame = floor(qreal(timeFromStartFrame) / averageFrameDuration);
            if(SelectedSpeed == Speed_M0 || SelectedSpeed == Speed_P0) {
                framesFromStartFrame /= 2;
            } else if(SelectedSpeed == Speed_M2 || SelectedSpeed == Speed_P2) {
                framesFromStartFrame *= 2;
            }

            size_t expectedFrame = startFrame + (Time_MinusPlus ? framesFromStartFrame : -framesFromStartFrame);

            if(expectedFrame != getCurrentFrame())
                setCurrentFrame(expectedFrame);

            lastRenderedFrame = expectedFrame;
            lastRenderedFrameTimeStamp = currentTime;

        }
    }

    if (QThread::currentThread()->isInterruptionRequested())
    {
        qDebug() << "stopping timer...";
        Timer->stop();
    }
    */
}

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Control_H
#define GUI_Control_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QWidget>
#include <QTime>

class FileInformation;
class PerPicture;
class TinyDisplay;
class Info;

class QLabel;
class QToolButton;
class QPushButton;
class QTimer;
class QTime;
class QCheckBox;
//---------------------------------------------------------------------------

//***************************************************************************
// Class
//***************************************************************************

class Control : public QWidget
{
    Q_OBJECT

public:
    explicit Control(QWidget *parent, FileInformation* FileInfoData, bool IsSlave=false);

    virtual ~Control();

    size_t getCurrentFrame() const;

    void setPlayAllFrames(bool value);
    bool getPlayAllFrames() const;

Q_SIGNALS:
    void currentFrameChanged();

    void playClicked();
    void stopClicked();

public Q_SLOTS:
    // Commands
    void                        Update();

    void TimeOut();
    void on_M9_clicked(bool checked);
    void on_M2_clicked(bool checked);
    void on_M1_clicked(bool checked);
    void on_M0_clicked(bool checked);
    void on_Minus_clicked(bool checked=false);
    void on_PlayPause_clicked(bool checked);
    void on_Pause_clicked(bool checked);
    void on_Plus_clicked(bool checked=false);
    void on_P0_clicked(bool checked);
    void on_P1_clicked(bool checked);
    void on_P2_clicked(bool checked);
    void on_P9_clicked(bool checked);

    void copyTimeStamp();
    void setCurrentFrame(size_t frame);
    void rewind(int frame);

public:
    // To update
    TinyDisplay*                TinyDisplayArea;
    Info*                       InfoArea;

    // Widgets
    QToolButton*                M9;
    QToolButton*                M2;
    QToolButton*                M1;
    QToolButton*                M0;
    QToolButton*                Minus;
    QLabel*                     Info_Time;
    QToolButton*                PlayPause;
    QToolButton*                Pause;
    QLabel*                     Info_Frames;
    QToolButton*                Plus;
    QToolButton*                P0;
    QToolButton*                P1;
    QToolButton*                P2;
    QToolButton*                P9;

    void stop();

private:
    bool playAllFrames;

    void TimeOut_Init();

    enum selectedspeed
    {
        Speed_M2,
        Speed_M1,
        Speed_M0,
        Speed_O,
        Speed_P0,
        Speed_P1,
        Speed_P2,
    };
    selectedspeed               SelectedSpeed;
    bool                        ShouldUpate;
    bool                        IsSlave;

    // File information
    FileInformation*            FileInfoData;
    int                         Frames_Pos;

    // Time
    QTimer* Timer;
    QThread* Thread;
    QTime   Time;

    size_t  startFrame;
    QTime   startFrameTimeStamp;

    size_t  lastRenderedFrame;
    QTime   lastRenderedFrameTimeStamp;

    double  averageFrameDuration;

    bool    Time_MinusPlus;
    int     Timer_Duration;
};

#endif // GUI_Control_H

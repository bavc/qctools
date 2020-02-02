#include "playercontrol.h"
#include "ui_playercontrol.h"

PlayerControl::PlayerControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerControl)
{
    ui->setupUi(this);
}

PlayerControl::~PlayerControl()
{
    delete ui;
}

const QPushButton *PlayerControl::goToStartButton() const
{
    return ui->goToStart_pushButton;
}

const QPushButton *PlayerControl::goToEndButton() const
{
    return ui->goToEnd_pushButton;
}

const QPushButton *PlayerControl::prevFrameButton() const
{
    return ui->prev_pushButton;
}

const QPushButton *PlayerControl::nextFrameButton() const
{
    return ui->next_pushButton;
}

const QPushButton *PlayerControl::playPauseButton() const
{
    return ui->playPause_pushButton;
}

QLabel *PlayerControl::timeLabel() const
{
    return ui->time_label;
}

QLabel *PlayerControl::frameLabel() const
{
    return ui->frame_label;
}

QLabel *PlayerControl::sliderLabel() const
{
    return ui->slider_label;
}

QLineEdit *PlayerControl::lineEdit() const
{
    return ui->goToTime_lineEdit;
}

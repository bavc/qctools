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

QPushButton *PlayerControl::goToStartButton()
{
    return ui->goToStart_pushButton;
}

QPushButton *PlayerControl::goToEndButton()
{
    return ui->goToEnd_pushButton;
}

QPushButton *PlayerControl::prevFrameButton()
{
    return ui->prev_pushButton;
}

QPushButton *PlayerControl::nextFrameButton()
{
    return ui->next_pushButton;
}

QPushButton *PlayerControl::playPauseButton()
{
    return ui->playPause_pushButton;
}

QPushButton *PlayerControl::exportButton()
{
    return ui->export_pushButton;
}

QLabel *PlayerControl::timeLabel() const
{
    return ui->time_label;
}

QLabel *PlayerControl::frameLabel() const
{
    return ui->frame_label;
}

QLineEdit *PlayerControl::lineEdit() const
{
    return ui->goToTime_lineEdit;
}

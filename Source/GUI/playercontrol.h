#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QLineEdit>

namespace Ui {
class PlayerControl;
}

class PlayerControl : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControl(QWidget *parent = nullptr);
    ~PlayerControl();

    const QPushButton* goToStartButton() const;
    const QPushButton* goToEndButton() const;
    const QPushButton* prevFrameButton() const;
    const QPushButton* nextFrameButton() const;
    const QPushButton* playPauseButton() const;

    QLabel* timeLabel() const;
    QLabel* frameLabel() const;
    QLabel* sliderLabel() const;

    QLineEdit* lineEdit() const;

private:
    Ui::PlayerControl *ui;
};

#endif // PLAYERCONTROL_H

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

    QPushButton* goToStartButton();
    QPushButton* goToEndButton();
    QPushButton* prevFrameButton();
    QPushButton* nextFrameButton();
    QPushButton* playPauseButton();
    QPushButton* exportButton();

    QLabel* timeLabel() const;
    QLabel* frameLabel() const;

    QLineEdit* lineEdit() const;

private:
    Ui::PlayerControl *ui;
};

#endif // PLAYERCONTROL_H

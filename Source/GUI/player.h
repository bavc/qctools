#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QtAV>

class FileInformation;

namespace Ui {
class Player;
}

class Player : public QWidget
{
    Q_OBJECT

public:
    explicit Player(QWidget *parent = nullptr);
    ~Player();

public Q_SLOTS:
    void setFile(FileInformation* filePath);
    void playPause();
    void seekBySlider(int value);
    void seekBySlider();

protected:
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void updateSlider(qint64 value);
    void updateSlider();
    void updateSliderUnit();
    void updateVideoOutputSize();

private Q_SLOTS:
    void on_playPause_pushButton_clicked();
    void on_fitToScreen_radioButton_toggled(bool value);
    void on_normalScale_radioButton_toggled(bool value);
    void on_scalePercentage_spinBox_valueChanged(int value);
    void on_scalePercentage_horizontalSlider_valueChanged(int value);

private:
    void setScaleSliderPercentage(int percents);
    void setScaleSpinboxPercentage(int percents);

private:
    Ui::Player *ui;

    int m_unit;

    QtAV::VideoOutput *m_vo;
    QtAV::AVPlayer *m_player;

    QtAV::LibAVFilterVideo* m_videoFilter;
    QtAV::LibAVFilterAudio* m_audioFilter;

    FileInformation* m_fileInformation;
};

#endif // PLAYER_H

#ifndef PLAYER_H
#define PLAYER_H

#include <QMainWindow>
#include <QTimer>
#include <QWidget>
#include <QtAV>

class FileInformation;
class FilterSelector;

namespace Ui {
class Player;
}

class Player : public QMainWindow
{
    Q_OBJECT

public:
    explicit Player(QWidget *parent = nullptr);
    ~Player();

    FileInformation* file() const;

    void playPaused(qint64 ms);

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
    void applyFilter();

private Q_SLOTS:
    void on_playPause_pushButton_clicked();

    void on_fitToScreen_radioButton_toggled(bool value);

    void on_normalScale_radioButton_toggled(bool value);

    void on_scalePercentage_spinBox_valueChanged(int value);

    void on_scalePercentage_horizontalSlider_valueChanged(int value);

    void on_graphmonitor_checkBox_clicked(bool checked);

    void on_goToStart_pushButton_clicked();

    void on_goToEnd_pushButton_clicked();

    void on_prev_pushButton_clicked();

    void on_next_pushButton_clicked();

    void on_lineEdit_returnPressed();

private:
    void setScaleSliderPercentage(int percents);
    void setScaleSpinboxPercentage(int percents);
    void handleFilterChange(FilterSelector *filterSelector, int filterIndex);
    void setFilter(const QString& filter);

private:
    Ui::Player *ui;

    QtAV::VideoOutput *m_vo;
    QtAV::AVPlayer *m_player;

    qreal m_unit;
    int m_framesCount;

    QtAV::LibAVFilterVideo* m_videoFilter;
    QtAV::LibAVFilterAudio* m_audioFilter;

    FileInformation* m_fileInformation;
    FilterSelector* m_filterSelectors[6];
    QString m_filters[6];
    QTimer m_filterUpdateTimer;
};

#endif // PLAYER_H

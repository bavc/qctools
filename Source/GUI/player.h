#ifndef PLAYER_H
#define PLAYER_H

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <QUrl>
#include <QVideoWidget>
#include <QVideoFrame>
#ifdef QT_AVPLAYER_MULTIMEDIA
#include <QtAVPlayer/qavaudiooutput.h>
#else
#include <QAudioSink>
#endif // QT_AVPLAYER_MULTIMEDIA
#include <QtAVPlayer/qavplayer.h>
#include "Core/FileInformation.h"

class QGraphicsVideoItem;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QMediaService>
#include <QVideoRendererControl>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class VideoRenderer : public QVideoRendererControl
{
public:
    QAbstractVideoSurface *surface() const override
    {
        return m_surface;
    }

    void setSurface(QAbstractVideoSurface *surface) override
    {
        m_surface = surface;
    }

    QAbstractVideoSurface *m_surface = nullptr;
};

class MediaObject;
class MediaService : public QMediaService
{
public:
    MediaService(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaService(parent)
        , m_renderer(vr)
    {
    }

    QMediaControl* requestControl(const char *name) override
    {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            return m_renderer;

        return nullptr;
    }

    void releaseControl(QMediaControl *) override
    {
    }

    VideoRenderer *m_renderer = nullptr;
};

class VideoWidget : public QVideoWidget
{
public:
    VideoWidget(QWidget* parent = nullptr) : QVideoWidget(parent) {
    }

    bool setMediaObject(QMediaObject *object) override
    {
        return QVideoWidget::setMediaObject(object);
    }
};

class MediaObject : public QMediaObject
{
public:
    explicit MediaObject(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaObject(parent, new MediaService(vr, parent))
    {
    }
};

#else
#include <QVideoSink>
#endif //

class FileInformation;
class FilterSelector;
class DraggableChildrenBehaviour;
class CommentsPlot;

namespace Ui {
class Player;
}

class MediaPlayer : public QAVPlayer {
    Q_OBJECT
public:
    MediaPlayer() {
        t.setInterval(100);
        connect(&t, &QTimer::timeout, [this]() {
            if(position() != prevPos) {
                prevPos = position();
                Q_EMIT positionChanged(prevPos);
            }
        });
        connect(this, &QAVPlayer::stateChanged, this, [this](QAVPlayer::State state) {
            if(state == QAVPlayer::PlayingState) {
                t.start();
            } else if(state == QAVPlayer::StoppedState || state == QAVPlayer::PausedState) {
                t.stop();
            }
        }, Qt::QueuedConnection);
        connect(this, &QAVPlayer::mediaStatusChanged, this, [this](QAVPlayer::MediaStatus status) {
            if(status == QAVPlayer::EndOfMedia) {
                prevPos = position();
                Q_EMIT positionChanged(prevPos);
            }
        }, Qt::QueuedConnection);
    }

    bool isPlaying () const {
        return state() == QAVPlayer::PlayingState;
    }

    bool isPaused() const {
        return state() == QAVPlayer::PausedState;
    }

    void setFile(const QString& file) {
        m_file = file;
        auto sourceFile = m_file;

        if (isDpx(sourceFile)) {
            int dpxOffset = 0;
            sourceFile = adjustDpxFileName(sourceFile, dpxOffset);
            setInputOptions({ {"start_number", QString::number(dpxOffset) }, {"f", "image2"} });
        } else {
            if(!inputOptions().empty()) {
                setInputOptions({});
            }
        }

        setSource(sourceFile);
    }

    QString file() const {
        return m_file;
    }

    QSize videoFrameSize() const;

    void specifyPosition(qint64 pos) {
        if(prevPos != pos) {
            prevPos = pos;
            Q_EMIT positionChanged(prevPos);
        }
    }

Q_SIGNALS:
    void positionChanged(qint64);

private:
    QString m_file;
    qint64 prevPos { 0 };
    QTimer t;
};

class SelectionAreaGraphicsObject;
class Player : public QMainWindow
{
    Q_OBJECT

public:
    explicit Player(QWidget *parent = nullptr);
    ~Player();

    FileInformation* file() const;

    QPushButton* playPauseButton() const;
    void playPaused(qint64 ms);

    void updateInfoLabels();

    static qint64 timeStringToMs(const QString& timeValue);

public Q_SLOTS:
    void setFile(FileInformation* filePath);
    void playPause();
    void seekBySlider(int value);
    void seekBySlider();
    void grabFrame();
    void showHideDebug();
    void showHideFilters();

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter( QObject *object, QEvent *event ) override;

private Q_SLOTS:
    void updateSlider(qint64 value);
    void updateSlider();
    void updateVideoOutputSize();
    void applyFilter();
    void handleFileInformationPositionChanges();
    void onCursorMoved(int index);

private Q_SLOTS:
    void on_playPause_pushButton_clicked();

    void on_fitToScreen_radioButton_toggled(bool value);

    void on_normalScale_radioButton_toggled(bool value);

    void on_freeScale_radioButton_toggled(bool value);

    void on_scalePercentage_spinBox_valueChanged(int value);

    void on_scalePercentage_horizontalSlider_valueChanged(int value);

    void on_graphmonitor_checkBox_clicked(bool checked);

    void on_goToStart_pushButton_clicked();

    void on_goToEnd_pushButton_clicked();

    void on_prev_pushButton_clicked();

    void on_next_pushButton_clicked();

    void on_fitToGrid_checkBox_toggled(bool checked);

    void on_speedp_horizontalSlider_valueChanged(int value);

    void on_goToTime_lineEdit_returnPressed();

    void on_export_pushButton_clicked();

    void on_xDoubleSpinBox_valueChanged(double arg1);

    void on_yDoubleSpinBox_valueChanged(double arg1);

    void on_wDoubleSpinBox_valueChanged(double arg1);

    void on_hDoubleSpinBox_valueChanged(double arg1);

private:
    void setScaleSliderPercentage(int percents);
    void setScaleSpinboxPercentage(int percents);
    void handleFilterChange(FilterSelector *filterSelector, int filterIndex);
    void setFilter(const QString& filter);
    QString replaceFilterTokens(const QString& filterString);

private:
    qint64 frameToMs(int frame);
    int msToFrame(qint64 ms);

private:
    Ui::Player *ui;

    QGraphicsVideoItem* m_w;

    MediaPlayer* m_player;
    bool m_mute { false };

    #ifdef QT_AVPLAYER_MULTIMEDIA
    QScopedPointer<QAVAudioOutput> m_audioOutput;
    #else
    QScopedPointer<QAudioSink> m_audioOutput;
    QIODevice* m_audioDevice;
    #endif // QT_AVPLAYER_MULTIMEDIA
    QVideoFrame videoFrame;

    bool m_handlePlayPauseClick;

    int m_framesCount;

    FileInformation* m_fileInformation;
    FilterSelector* m_filterSelectors[6];
    FilterSelector* m_adjustmentSelector;
    DraggableChildrenBehaviour* m_draggableBehaviour;
    CommentsPlot* m_commentsPlot;
    bool m_seekOnFileInformationPositionChange;
    bool m_ignorePositionChanges;

    SelectionAreaGraphicsObject* m_selectionArea;
    QRect m_selectionAreaGeometry;
    qreal m_scaleFactor;
    QSize m_videoFrameSize;
    QTimer m_filterUpdateTimer;
};

#endif // PLAYER_H

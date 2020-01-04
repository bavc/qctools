#include "player.h"
#include "ui_player.h"
#include "Core/FileInformation.h"
#include "Core/FFmpeg_Glue.h"
#include "GUI/filterselector.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

const int MaxFilters = 6;
const int DefaultFilterIndex = 1;

Player::Player(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Player)
{
    ui->setupUi(this);
    m_unit = 1;

    m_player = new QtAV::AVPlayer(ui->scrollArea);
    m_vo = new QtAV::VideoOutput(ui->scrollArea);
    connect(m_vo, SIGNAL(videoFrameSizeChanged()), this, SLOT(updateVideoOutputSize()));

    ui->scrollArea->setWidget(m_vo->widget());
    ui->scrollArea->widget()->setGeometry(0, 0, 100, 100);

    m_player->setRenderer(m_vo);
    m_player->setSeekType(QtAV::AnyFrameSeek);
    m_player->setMediaEndAction(QtAV::MediaEndAction_Pause);
    m_player->setAsyncLoad(false);
    m_player->setNotifyInterval(1);
    m_player->setBufferMode(QtAV::BufferPackets);
    m_player->setBufferValue(10000);

    m_videoFilter = new QtAV::LibAVFilterVideo(this);
    m_audioFilter = new QtAV::LibAVFilterAudio(this);

    m_player->installFilter(m_videoFilter);
    m_player->installFilter(m_audioFilter);

    connect(m_player, &QtAV::AVPlayer::stateChanged, [this](QtAV::AVPlayer::State state) {
        if(state == QtAV::AVPlayer::PlayingState) {
            ui->playPause_pushButton->setIcon(QIcon(":/icon/pause.png"));
        } else {
            ui->playPause_pushButton->setIcon(QIcon(":/icon/play.png"));
        }
    });

    connect(ui->playerSlider, SIGNAL(sliderMoved(int)), SLOT(seekBySlider(int)));
    connect(ui->playerSlider, SIGNAL(sliderPressed()), SLOT(seekBySlider()));

    connect(m_player, SIGNAL(positionChanged(qint64)), SLOT(updateSlider(qint64)));
    connect(m_player, SIGNAL(started()), SLOT(updateSlider()));
    // connect(m_player, SIGNAL(notifyIntervalChanged()), SLOT(updateSliderUnit()));

    connect(ui->arrangementButtonGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(applyFilter()));

    ui->filterGroupBox->setLayout(new QVBoxLayout);
    ui->filterGroupBox->setMinimumHeight(60 * MaxFilters);

    for(int i = 0; i < 6; ++i) {
        m_filterSelectors[i] = new FilterSelector(this);
        handleFilterChange(m_filterSelectors[i], i);
        ui->filterGroupBox->layout()->addWidget(m_filterSelectors[i]);
    }

    // select 'normal' by default
    m_filterSelectors[0]->selectCurrentFilter(DefaultFilterIndex);

    m_filterUpdateTimer.setSingleShot(true);
    connect(&m_filterUpdateTimer, &QTimer::timeout, this, &Player::applyFilter);
}

Player::~Player()
{
    m_player->stop();
    m_player->removeEventFilter(m_videoFilter);
    m_player->removeEventFilter(m_audioFilter);
    delete ui;
}

FileInformation *Player::file() const
{
    return m_fileInformation;
}

void Player::setFile(FileInformation *fileInfo)
{
    if(m_player->file() != fileInfo->fileName()) {
        m_fileInformation = fileInfo;
        for(int i = 0; i < MaxFilters; ++i)
        {
            m_filterSelectors[i]->setFileInformation(m_fileInformation);
        }

        m_player->stop();
        m_player->setFile(fileInfo->fileName());
        m_player->audio()->setMute(true);

        std::shared_ptr<QMetaObject::Connection> pConnection = std::make_shared<QMetaObject::Connection>();

        *pConnection = connect(m_player, &QtAV::AVPlayer::stateChanged, [this, pConnection](QtAV::AVPlayer::State state) {
            if(state == QtAV::AVPlayer::PlayingState) {
                auto position = m_player->position();
                QTimer::singleShot(0, this, [this, position] {
                    m_player->audio()->setMute(false);

                    m_player->seek((qint64)0);
                    m_player->pause(true);

                    QTimer::singleShot(0, this, [this] {
                        // select 'normal' by default
                        m_filterSelectors[0]->setCurrentFilter(DefaultFilterIndex);
                    });
                });

                QObject::disconnect(*pConnection);

            }
        });

        QTimer::singleShot(0, this, SLOT(updateVideoOutputSize()));
        m_player->load();

        m_framesCount = m_fileInformation->Glue->VideoFrameCount_Get();
        ui->playerSlider->setMaximum(m_player->duration());

        m_unit = 1; // qreal(m_player->duration()) / m_framesCount;

        m_player->play();
    }
}

void Player::playPause()
{
    if (!m_player->isPlaying()) {
        m_player->play();
        return;
    }
    m_player->pause(!m_player->isPaused());
}

void Player::seekBySlider(int value)
{
    if (!m_player->isPlaying())
        return;

    auto newValue = qint64(value*m_unit);
    qDebug() << "seek to: " << value;

    m_player->seek(newValue);
}

void Player::seekBySlider()
{
    seekBySlider(ui->playerSlider->value());
}

void Player::resizeEvent(QResizeEvent *event)
{
    updateVideoOutputSize();
}

void Player::updateSlider(qint64 value)
{
    auto newValue = int(qreal(value)/m_unit);
    if(ui->playerSlider->value() == newValue)
        return;

    qDebug() << "update slider: " << newValue;

    // ui->playerSlider->setRange(0, int(m_player->duration()/m_unit));
    ui->playerSlider->setValue(newValue);

    ui->slider_label->setText(QString::number(m_player->duration()) + "/" + QString::number(int(newValue/m_unit)));

    auto position = m_player->position();
    auto duration = m_player->duration();
    auto framePos = (int) (qreal(position) / duration  * m_framesCount);
    ui->frame_label->setText(QString("Frame %1 [%2]").arg(framePos).arg(m_fileInformation->Frame_Type_Get()));
}

void Player::updateSlider()
{
    updateSlider(m_player->position());
}

void Player::updateSliderUnit()
{
    // m_unit = m_player->notifyInterval();
    // updateSlider();
}

void Player::updateVideoOutputSize()
{
    QSize newSize;

    auto filteredFrameWidth = m_vo->videoFrameSize().width();
    auto filteredFrameHeight = m_vo->videoFrameSize().height();

    if(!ui->fitToScreen_radioButton->isChecked()) {
        double multiplier = ((double) ui->scalePercentage_spinBox->value()) / 100;
        newSize = QSize(filteredFrameWidth, filteredFrameHeight) * multiplier;
    } else {
        auto availableSize = ui->scrollArea->viewport()->size() - QSize(1, 1);

        auto scaleFactor = double(availableSize.width()) / filteredFrameWidth;
        newSize = QSize(availableSize.width(), scaleFactor * filteredFrameHeight);
        if(newSize.height() > availableSize.height()) {
            scaleFactor = double(availableSize.height()) / filteredFrameHeight;
            newSize = QSize(scaleFactor * filteredFrameWidth, availableSize.height());
        }
    }

    auto geometry = ui->scrollArea->widget()->geometry();
    geometry.setSize(newSize);

    ui->scrollArea->widget()->setGeometry(geometry);
}

void Player::applyFilter()
{
    ui->plainTextEdit->clear();

    QStringList definedFilters;
    for(auto i = 0; i < MaxFilters; ++i) {
        auto empty = m_filters[i].isEmpty();
        if(!empty)
            definedFilters.append(m_filters[i]);
    }

    ui->plainTextEdit->appendPlainText(QString("*** defined filters ***: \n\n%1").arg(definedFilters.join("\n")));

    if(definedFilters.empty()) {
        setFilter(QString());
        return;
    }

    auto layout = QString();
    if(ui->vertical_checkBox->isChecked()) {
        layout = "0_0|0_h0|0_h0+h1|0_h0+h1+h2|0_h0+h1+h2+h3|0_h0+h1+h2+h3+h4";
    } else if(ui->horizontal_checkBox->isChecked()) {
        layout = "0_0|w0_0|w0+w1_0|w0+w1+w2_0|w0+w1+w2+w3_0|w0+w1+w2+w3+w4_0";
    } else if(ui->grid_checkBox->isChecked()) {
        layout = "0_0|w0_0|0_h0|w0_h0|0_h0+h1|w0_h0+h1";
    }

    ui->plainTextEdit->appendPlainText(QString("*** layout ***: \n\n%1").arg(layout));

    QString splits[] = {
        "",
        "sws_flags=neighbor;split=2[x1][x2];",
        "sws_flags=neighbor;split=3[x1][x2][x3];",
        "sws_flags=neighbor;split=4[x1][x2][x3][x4];",
        "sws_flags=neighbor;split=5[x1][x2][x3][x4][x5];",
        "sws_flags=neighbor;split=6[x1][x2][x3][x4][x5][x6];"
    };

    auto split = splits[definedFilters.length() - 1];

    ui->plainTextEdit->appendPlainText(QString("*** split ***: \n\n%1").arg(split));

    QString filterString;

    if(definedFilters.length() == 1) {
        filterString = definedFilters[0];
    } else {
        for(int i = 0; i < definedFilters.length(); ++i) {
            filterString += QString("[x%1]%2[y%1];").arg(i + 1).arg(definedFilters[i]);
        }
    }

    ui->plainTextEdit->appendPlainText(QString("*** filterString ***: \n\n%1").arg(filterString));

    QString xstack_inputs[] = {
        "",
        "[y1][y2]",
        "[y1][y2][y3]",
        "[y1][y2][y3][y4]",
        "[y1][y2][y3][y4][y5]",
        "[y1][y2][y3][y4][y5][y6]"
    };

    auto xstack_input = xstack_inputs[definedFilters.length() - 1];

    ui->plainTextEdit->appendPlainText(QString("*** xstack_input ***: \n\n%1").arg(xstack_input));

    QString xstack_option;

    if(definedFilters.length() != 1) {
        xstack_option = QString("%1xstack=inputs=%2:layout=%3").arg(xstack_input).arg(definedFilters.length()).arg(layout);
    }

    QString combinedFilter = split + filterString + xstack_option;

    if(ui->graphmonitor_checkBox->isChecked())
    {
        combinedFilter.append(QString(",graphmonitor=flags=queue+pts+time+timebase+format+size+rate:m=full"));
    }

    ui->plainTextEdit->appendPlainText(QString("*** result ***: \n\n%1").arg(combinedFilter));

    setFilter(combinedFilter);
}

void Player::on_playPause_pushButton_clicked()
{
    playPause();
}

void Player::on_fitToScreen_radioButton_toggled(bool value)
{
    if(value)
    {
        updateVideoOutputSize();
    }
}

void Player::on_normalScale_radioButton_toggled(bool value)
{
    if(value)
    {
        setScaleSliderPercentage(100);
        setScaleSpinboxPercentage(100);
        on_scalePercentage_spinBox_valueChanged(100);
    }
}

void Player::on_scalePercentage_spinBox_valueChanged(int value)
{
    double multiplier = ((double) value) / 100;

    QSize newSize = QSize(m_vo->videoFrameSize().width(), m_vo->videoFrameSize().height()) * multiplier;
    QSize currentSize = ui->scrollArea->widget()->size();

    if(newSize != currentSize)
    {
        if(value != 100 && !ui->freeScale_radioButton->isChecked())
        {
            ui->freeScale_radioButton->blockSignals(true);
            ui->freeScale_radioButton->setChecked(true);
            ui->freeScale_radioButton->blockSignals(false);
        }

        updateVideoOutputSize();
    }

    setScaleSpinboxPercentage(value);
    setScaleSliderPercentage(value);
}

const int MinSliderPercents = 50;
const int MaxSliderPercents = 200;
const int AvgSliderPercents = 100;

void Player::on_scalePercentage_horizontalSlider_valueChanged(int value)
{
    int range = ui->scalePercentage_horizontalSlider->maximum() - ui->scalePercentage_horizontalSlider->minimum();
    int halfRange = range / 2;
    int valueInPercents = 0;

    if(value <= halfRange)
    {
        valueInPercents = (AvgSliderPercents - MinSliderPercents) * (value - ui->scalePercentage_horizontalSlider->minimum()) / halfRange + MinSliderPercents;
    }
    else
    {
        valueInPercents = (MaxSliderPercents -  AvgSliderPercents) * (value - halfRange) / halfRange + AvgSliderPercents;
    }

    on_scalePercentage_spinBox_valueChanged(valueInPercents);
}

void Player::setScaleSliderPercentage(int percents)
{
    ui->scalePercentage_horizontalSlider->blockSignals(true);

    int range = ui->scalePercentage_horizontalSlider->maximum() - ui->scalePercentage_horizontalSlider->minimum();
    int halfRange = range / 2;

    if(percents < MinSliderPercents)
        percents = MinSliderPercents;
    if(percents > MaxSliderPercents)
        percents = MaxSliderPercents;

    if(percents <= AvgSliderPercents) {
        int percentRange = AvgSliderPercents - MinSliderPercents;
        ui->scalePercentage_horizontalSlider->setValue(halfRange * (percents - MinSliderPercents) / percentRange);
    } else {
        int percentRange = MaxSliderPercents - AvgSliderPercents;
        ui->scalePercentage_horizontalSlider->setValue(halfRange + halfRange * (percents - AvgSliderPercents) / percentRange);
    }
    ui->scalePercentage_horizontalSlider->blockSignals(false);
}

void Player::setScaleSpinboxPercentage(int percents)
{
    ui->scalePercentage_spinBox->blockSignals(true);
    ui->scalePercentage_spinBox->setValue(percents);
    ui->scalePercentage_spinBox->blockSignals(false);
}

void Player::handleFilterChange(FilterSelector *filterSelector, int filterIndex)
{
    connect(filterSelector, &FilterSelector::filterChanged, [this, filterIndex](const QString& filterString) {

        QString str = filterString;

        str.replace(QString("${width}"), QString::number(m_fileInformation->Glue->Width_Get()));
        str.replace(QString("${height}"), QString::number(m_fileInformation->Glue->Height_Get()));
        str.replace(QString("${dar}"), QString::number(m_fileInformation->Glue->DAR_Get()));

        //    QSize windowSize = imageLabels[Pos]->pixmapSize();
        QSize windowSize = ui->scrollArea->widget()->size();

        str.replace(QString("${window_width}"), QString::number(windowSize.width()));
        str.replace(QString("${window_height}"), QString::number(windowSize.height()));

        QString tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QDir tempDir(tempLocation);

        QString qctoolsTmpSubDir = "qctools";
        QString fontFileName = "Anonymous_Pro_B.ttf";

        if(tempDir.exists())
        {
            QDir qctoolsTmpDir(tempLocation + "/" + qctoolsTmpSubDir);
            if(!qctoolsTmpDir.exists())
                tempDir.mkdir(qctoolsTmpSubDir);

            QFile fontFile(qctoolsTmpDir.path() + "/" + fontFileName);
            if(!fontFile.exists())
            {
                QFile::copy(":/" + fontFileName, fontFile.fileName());
            }

            if(fontFile.exists())
            {
                QString fontFileName(fontFile.fileName());
                fontFileName = fontFileName.replace(":", "\\\\:"); // ":" is a reserved character, it must be escaped
                str.replace(QString("${fontfile}"), fontFileName);
            }
        }

        m_filters[filterIndex] = str;

        m_filterUpdateTimer.stop();
        m_filterUpdateTimer.start(100);
    });
}

void Player::setFilter(const QString &filter)
{
    m_videoFilter->setOptions(filter);
    m_audioFilter->setOptions(filter);

    if(m_player->isPaused()) {
        QTimer::singleShot(0, this, [&] {
            auto sliderValue = (qint64)ui->playerSlider->value();
            // m_player->seek(sliderValue * m_unit);
            m_player->pause(true);
        });
    }
}

void Player::on_graphmonitor_checkBox_clicked(bool checked)
{
    applyFilter();
}

void Player::on_goToStart_pushButton_clicked()
{
    m_player->seek(qint64(0));
}

void Player::on_goToEnd_pushButton_clicked()
{
    m_player->seek(m_player->startPosition() + m_player->duration());
}

void Player::on_prev_pushButton_clicked()
{
    auto newPosition = m_player->position() - 1;
    qDebug() << "new position: " << newPosition;
    // m_player->seek(newPosition);

    auto frameDuration = (qreal) m_player->duration() / m_framesCount;
    m_player->seek(m_player->position() - (qint64) frameDuration);

    /*
    m_player->stepForward();
    m_player->stepBackward();
    m_player->stepBackward();
    */
}

void Player::on_next_pushButton_clicked()
{
    auto newPosition = m_player->position() + 1;
    qDebug() << "new position: " << newPosition;
    // m_player->seek(newPosition);
    m_player->stepForward();
}

void Player::on_lineEdit_returnPressed()
{
    m_player->seek((qint64) (m_unit * ui->lineEdit->text().toInt()));
}

#include "player.h"
#include "ui_player.h"
#include "Core/FileInformation.h"
#include "Core/FFmpeg_Glue.h"
#include "GUI/filterselector.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

Player::Player(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Player)
{
    ui->setupUi(this);
    m_unit = 1000;

    m_player = new QtAV::AVPlayer(ui->scrollArea);
    m_vo = new QtAV::VideoOutput(ui->scrollArea);

    ui->scrollArea->setWidget(m_vo->widget());
    ui->scrollArea->widget()->setGeometry(0, 0, 100, 100);

    m_player->setRenderer(m_vo);
    m_videoFilter = new QtAV::LibAVFilterVideo();
    m_audioFilter = new QtAV::LibAVFilterAudio();

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
    connect(m_player, SIGNAL(notifyIntervalChanged()), SLOT(updateSliderUnit()));

    m_filterSelector = new FilterSelector(this);
    handleFilterChange(m_filterSelector, 0);

    ui->filterFrame->setLayout(new QHBoxLayout);
    ui->filterFrame->setMinimumHeight(100);
    ui->filterFrame->layout()->addWidget(m_filterSelector);
}

Player::~Player()
{
    delete ui;
}

void Player::setFile(FileInformation *fileInfo)
{
    if(m_player->file() != fileInfo->fileName()) {
        m_fileInformation = fileInfo;
        m_filterSelector->setFileInformation(m_fileInformation);
        m_player->setFile(fileInfo->fileName());

        std::shared_ptr<QMetaObject::Connection> pConnection = std::make_shared<QMetaObject::Connection>();
        *pConnection = connect(m_player, &QtAV::AVPlayer::stateChanged, [this, pConnection](QtAV::AVPlayer::State state) {
            if(state == QtAV::AVPlayer::PlayingState) {
                m_player->pause();
                m_player->seek(m_player->position());
            }

            QObject::disconnect(*pConnection);
        });

        QTimer::singleShot(0, this, SLOT(updateVideoOutputSize()));
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
    m_player->seek(qint64(value*m_unit));
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
    ui->playerSlider->setRange(0, int(m_player->duration()/m_unit));
    ui->playerSlider->setValue(int(value/m_unit));
}

void Player::updateSlider()
{
    updateSlider(m_player->position());
}

void Player::updateSliderUnit()
{
    m_unit = m_player->notifyInterval();
    updateSlider();
}

void Player::updateVideoOutputSize()
{
    QSize newSize;

    if(!ui->fitToScreen_radioButton->isChecked()) {
        double multiplier = ((double) ui->scalePercentage_spinBox->value()) / 100;
        newSize = QSize(m_fileInformation->width(), m_fileInformation->height()) * multiplier;
    } else {
        auto availableSize = ui->scrollArea->viewport()->size() - QSize(1, 1);

        auto scaleFactor = double(availableSize.width()) / m_fileInformation->width();
        newSize = QSize(availableSize.width(), scaleFactor * m_fileInformation->height());
        if(newSize.height() > availableSize.height()) {
            scaleFactor = double(availableSize.height()) / m_fileInformation->height();
            newSize = QSize(scaleFactor * m_fileInformation->width(), availableSize.height());
        }
    }

    // qDebug() << "newSize: " << newSize;
    auto geometry = ui->scrollArea->widget()->geometry();
    // qDebug() << "old geometry: " << geometry;

    geometry.setSize(newSize);
    // qDebug() << "new geometry: " << geometry;

    ui->scrollArea->widget()->setGeometry(geometry);
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

    QSize newSize = QSize(m_fileInformation->width(), m_fileInformation->height()) * multiplier;
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

        setFilter(str);
    });
}

void Player::setFilter(const QString &filter)
{
    m_videoFilter->setOptions(filter);
    m_audioFilter->setOptions(filter);

    if(m_player->isPaused()) {
        m_player->seek(m_player->position());
    }
}

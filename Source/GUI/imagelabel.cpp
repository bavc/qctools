#include "imagelabel.h"
#include "ui_imagelabel.h"
#include "SelectionArea.h"
#include "Core/FFmpeg_Glue.h"
#include <QDebug>
#include <QPainter>
#include <cmath>

ImageLabel::ImageLabel(FFmpeg_Glue** Picture_, size_t Pos_, QWidget *parent) :
    ui(new Ui::ImageLabel),
    QFrame(parent),
    Picture(Picture_),
    Pos(Pos_),
    selectionPos(0, 0),
    selectionSize(0, 0),
    maxSelectionAreaSize(0, 0),
    minSelectionAreaSize(0, 0),
    debugOverlay(false)
{
    ui->setupUi(this);

    uilabel = new QLabel();
    ui->scrollArea->setWidget(uilabel);

    selectionArea = new SelectionArea(uilabel);

    connect(selectionArea, &SelectionArea::geometryChangeFinished, this, [&]() {
        Q_EMIT selectionChangeFinished(QRectF(selectionPos, selectionSize));
    });

    connect(selectionArea, &SelectionArea::geometryChanged, this, [&](const QRect& geometry) {
        int scaledWidth = Pixmap.width();
        int scaledHeight = Pixmap.height();

        QRectF originalGeometry(geometry);

        auto originalWidth = (*Picture)->Width_Get();
        auto originalHeight = (*Picture)->Height_Get();

        if(originalWidth > originalHeight)
            originalHeight = originalWidth / (*Picture)->OutputDAR_Get(Pos - 1);
        else
            originalWidth = originalHeight * (*Picture)->OutputDAR_Get(Pos - 1);

        QSizeF size(originalGeometry.width() * originalWidth / scaledWidth,
                    originalGeometry.height() * originalHeight / scaledHeight);

        QPointF topLeft(originalGeometry.topLeft().x() * originalWidth / scaledWidth,
                        originalGeometry.topLeft().y() * originalHeight / scaledHeight);

        originalGeometry.setSize(size);
        originalGeometry.moveTopLeft(topLeft);

        selectionPos = originalGeometry.topLeft();
        selectionSize = originalGeometry.size();

        Q_EMIT selectionChanged(originalGeometry);
    });

    uilabel->installEventFilter(this);
}

ImageLabel::~ImageLabel()
{
    delete ui;
}

void ImageLabel::Remove ()
{
    Pixmap=QPixmap();
    resize(0, 0);
    repaint();
    setVisible(false);
}


void ImageLabel::setImage(const QImage& image)
{
    updatePixmap(image);
}

void ImageLabel::updatePixmap(const QImage& image /*= nullptr*/)
{
    if(*Picture == nullptr)
    {
        return;
    }

    if (needRescale())
    {
        qDebug() << (Pos == 1 ? "left" : "right") << " needs rescale, rescaling..";
        rescale();
    }
    else
    {
        auto picture = *Picture;

        QImage Image = image;
        if (Image.isNull())
        {
            auto frameImage = picture->Image_Get(Pos - 1);
            if(!frameImage.isNull())
            {
                Image = QImage(frameImage.data(), frameImage.width(), frameImage.height(), frameImage.linesize(), QImage::Format_RGB888);
            }
        }

        if (Image.isNull())
        {
            Pixmap = QPixmap(Pixmap.width(), Pixmap.height());
            uilabel->setPixmap(Pixmap);
            return;
        }

        Pixmap.convertFromImage(Image);
        uilabel->setPixmap(Pixmap);
    }
}

void ImageLabel::setPixmap(const QPixmap &pixmap)
{
    Pixmap = pixmap;

    if (needRescale())
    {
        rescale();
    }
    else
    {
        uilabel->setPixmap(pixmap);
    }
}

size_t ImageLabel::GetPos() const
{
    return Pos;
}

void ImageLabel::moveSelectionX(double value)
{
    selectionPos.setX(value);

    int originalWidth = (*Picture)->Width_Get();
    int scaledWidth = Pixmap.width();
    int scaledX = (value * scaledWidth / originalWidth);

    QRect geometry = selectionArea->geometry();
    int geometryX = geometry.topLeft().x();

    if(geometryX == scaledX)
    {
        qDebug() << "geometry X equals to box value";
        return;
    }

    QSignalBlocker blocker(selectionArea);
    geometry.moveTopLeft(QPoint(scaledX, geometry.topLeft().y()));
    selectionArea->setGeometry(geometry);
}

void ImageLabel::moveSelectionY(double value)
{
    selectionPos.setY(value);

    int originalHeight = (*Picture)->Height_Get();
    int scaledHeight = Pixmap.height();
    int scaledY = (value * scaledHeight / originalHeight);

    QRect geometry = selectionArea->geometry();
    int geometryY = geometry.topLeft().y();

    if(geometryY == scaledY)
    {
        qDebug() << "geometry center Y equals to box value";
        return;
    }

    QSignalBlocker blocker(selectionArea);
    geometry.moveTopLeft(QPoint(geometry.topLeft().x(), scaledY));
    selectionArea->setGeometry(geometry);
}

void ImageLabel::changeSelectionWidth(double value)
{
    selectionSize.setWidth(value);

    auto originalWidth = (*Picture)->Width_Get();
    auto scaledWidth = Pixmap.width();

    int width = (value * scaledWidth / originalWidth);

    QRect geometry = selectionArea->geometry();
    int geometryWidth = geometry.width();

    if(geometryWidth == width)
    {
        qDebug() << "geometryWidth equals to box value";
        return;
    }

    QSignalBlocker blocker(selectionArea);

    qDebug() << "width changed: value = " << value << ", old width = " << geometry.width() << ", new width = " << width;

    geometry.setWidth(width);
    selectionArea->setGeometry(geometry);
}

void ImageLabel::changeSelectionHeight(double value)
{
    selectionSize.setHeight(value);

    auto originalHeight = (*Picture)->Height_Get();
    auto scaledHeight = Pixmap.height();

    int height = (value * scaledHeight / originalHeight);

    QRect geometry = selectionArea->geometry();
    int geometryHeight = geometry.height();

    if(geometryHeight == height)
    {
        qDebug() << "geometryHeight equals to box value";
        return;
    }

    QSignalBlocker blocker(selectionArea);

    qDebug() << "height changed: value = " << value << ", old height = " << geometry.height() << ", new height = " << height;

    geometry.setHeight(height);
    selectionArea->setGeometry(geometry);
}

void ImageLabel::setMaxSelectionWidth(double w)
{
    maxSelectionAreaSize = QSizeF(w, w * (*Picture)->Height_Get() / (*Picture)->Width_Get());
}

void ImageLabel::setMinSelectionWidth(double w)
{
    minSelectionAreaSize = QSizeF(w, w * (*Picture)->Height_Get() / (*Picture)->Width_Get());
}

void ImageLabel::setMinSelectionSize(QSizeF size)
{
    minSelectionAreaSize = size;
}

void ImageLabel::setMaxSelectionSize(QSizeF size)
{
    maxSelectionAreaSize = size;
}

void ImageLabel::setSelectionArea(double x, double y, double w, double h)
{
    selectionPos.setX(x); selectionPos.setY(y); selectionSize.setWidth(w); selectionSize.setHeight(h);

    // x, y - top left of box

    auto originalWidth = (*Picture)->Width_Get();
    auto originalHeight = (*Picture)->Height_Get();

    if(originalWidth > originalHeight)
        originalHeight = originalWidth / (*Picture)->OutputDAR_Get(Pos - 1);
    else
        originalWidth = originalHeight * (*Picture)->OutputDAR_Get(Pos - 1);

    auto scaledWidth = Pixmap.width();
    auto scaledHeight = Pixmap.height();

    auto scaledX = x * scaledWidth / originalWidth;
    auto scaledY = y * scaledHeight / originalHeight;

    int width = (w * scaledWidth / originalWidth);
    int height = (h * scaledHeight / originalHeight);

    QSignalBlocker blocker(selectionArea);

    int maxScaledSelectionWidth = maxSelectionAreaSize.width() * scaledWidth / originalWidth;
    int maxScaledSelectionHeight = maxSelectionAreaSize.height() * scaledHeight / originalHeight;

    selectionArea->setMaxSize(maxScaledSelectionWidth, maxScaledSelectionHeight);

    int minScaledSelectionWidth = minSelectionAreaSize.width() * scaledWidth / originalWidth;
    int minScaledSelectionHeight = minSelectionAreaSize.height() * scaledHeight / originalHeight;

    selectionArea->setMinSize(minScaledSelectionWidth, minScaledSelectionHeight);

    QRect geometry(scaledX, scaledY, width, height);
    selectionArea->setGeometry(geometry);
}

void ImageLabel::clearSelectionArea()
{
    selectionPos.setX(0); selectionPos.setY(0); selectionSize.setWidth(0); selectionSize.setHeight(0);

    QSignalBlocker blocker(selectionArea);

    selectionArea->setMaxSize(0, 0);
    selectionArea->setGeometry(QRect(0, 0, 0, 0));
}

void ImageLabel::showDebugOverlay(bool enable)
{
    debugOverlay = enable;
    selectionArea->showDebugOverlay(enable);
}

void ImageLabel::adjustScale()
{
    QSize newSize;
    if(!ui->fitToScreen_radioButton->isChecked())
    {
        double multiplier = ((double) ui->scalePercentage_spinBox->value()) / 100;
        newSize = QSize((*Picture)->Width_Get(), (*Picture)->Height_Get()) * multiplier;
    }
    rescale(newSize);
}
void ImageLabel::on_fitToScreen_radioButton_toggled(bool value)
{
    if(value)
    {
        rescale();
    }
}

void ImageLabel::on_normalScale_radioButton_toggled(bool value)
{
    if(value)
    {
        on_scalePercentage_spinBox_valueChanged(100);
    }
}

void ImageLabel::on_scalePercentage_spinBox_valueChanged(int value)
{
    if(*Picture)
    {
        double multiplier = ((double) value) / 100;

        QSize newSize = QSize((*Picture)->Width_Get(), (*Picture)->Height_Get()) * multiplier;
        QSize currentSize = Pixmap.size();

        if(newSize != currentSize)
        {
            if(value != 100 && !ui->freeScale_radioButton->isChecked())
            {
                ui->freeScale_radioButton->blockSignals(true);
                ui->freeScale_radioButton->setChecked(true);
                ui->freeScale_radioButton->blockSignals(false);
            }

            rescale(newSize);
        }
    }
}

const int MinSliderPercents = 50;
const int MaxSliderPercents = 200;
const int AvgSliderPercents = 100;

void ImageLabel::on_scalePercentage_horizontalSlider_valueChanged(int value)
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

void ImageLabel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updatePixmap();
}

bool ImageLabel::eventFilter(QObject *object, QEvent *event)
{
    if(!debugOverlay)
        return false;

    if(event->type() == QEvent::Paint)
    {
        if(*Picture == nullptr)
        {
            return false;
        }

        auto picture = *Picture;

        QWidget* widget = static_cast<QWidget*> (object);

        uilabel->removeEventFilter(this);
        QApplication::sendEvent(object, event);
        uilabel->installEventFilter(this);

        QPainter p(widget);
        p.setPen(Qt::green);

        auto originalWidth = (*Picture)->Width_Get();
        auto originalHeight = (*Picture)->Height_Get();

        auto scaledWidth = selectionArea->width();
        auto scaledHeight = selectionArea->height();

        auto scaledX = selectionPos.x() * scaledWidth / originalWidth;
        auto scaledY = selectionPos.y() * scaledHeight / originalHeight;

        p.fillRect(QRect(0, 0, size().width(), 50), QColor(128, 128, 128, 128));
        p.drawText(20, 20, QString("frameWidth: %1, frameHeight: %2, fw/fh: %3, imageWidth: %4, imageHeigh: %5, iw/ih: %6")
                   .arg(originalWidth)
                   .arg(originalHeight)
                   .arg(qreal(originalWidth) / originalHeight)
                   .arg(uilabel->width())
                   .arg(uilabel->height())
                   .arg(qreal(uilabel->width()) / uilabel->height())
                );

        p.drawText(20, 30, QString("imageLabelWidth: %1, imageLabelHeight: %2, dar: %3, sar: %4, output dar: %5")
                   .arg(width())
                   .arg(height())
                   .arg(QString::number(picture->DAR_Get()))
                   .arg(QString::fromStdString(picture->SAR_Get()))
                   .arg(QString::number(picture->OutputDAR_Get(Pos - 1))));

        p.drawText(20, 40, QString("selectionWidth: %1, selectionHeight: %2, aspect ratio: %3")
                   .arg(selectionArea->geometry().width())
                   .arg(selectionArea->geometry().height())
                   .arg(qreal(selectionArea->geometry().width()) / selectionArea->geometry().height()));

        return true;
    }

    return false;
}

bool ImageLabel::needRescale()
{
    if(*Picture == nullptr || !ui->fitToScreen_radioButton->isChecked())
    {
        return false;
    }

    auto picture = *Picture;
    auto availableSize = ui->scrollArea->viewport()->size() - QSize(1, 1);

    QSize Size = availableSize;
    QSize pixmapSize = Pixmap.size();

    auto dar = picture->OutputDAR_Get(Pos - 1);
    auto iar = qreal(Size.width()) / Size.height();

    int expectedWidth = 0;
    int expectedHeight = 0;

    if (std::isnan(dar))
        return false;

    if(dar > iar) {
        expectedWidth = Size.width();
        expectedHeight = Size.width() / dar;
    } else {
        expectedHeight = Size.height();
        expectedWidth = Size.height() * dar;
    }

    int dw = abs(expectedWidth - pixmapSize.width());
    int dh = abs(expectedHeight - pixmapSize.height());

    bool needRescale = dw > 1 && dh > 1;

    return needRescale;
}

void ImageLabel::rescale(const QSize& newSize /*= QSize()*/ )
{
    if(*Picture == nullptr)
    {
        return;
    }

    auto picture = *Picture;
    auto availableSize = !newSize.isEmpty() ? newSize : ui->scrollArea->viewport()->size() - QSize(1, 1);

    if(availableSize.width() < 0 || availableSize.height() < 0)
        return;

    picture->Scale_Change(availableSize.width(), availableSize.height(), Pos - 1);
    auto scaleFactor = (qreal) availableSize.width() / picture->Width_Get();

    auto image = picture->Image_Get(Pos - 1);

    if (image.isNull())
    {

        Pixmap = QPixmap(picture->OutputWidth_Get(Pos - 1), picture->OutputHeight_Get(Pos - 1));
        uilabel->setGeometry(0, 0, Pixmap.width(), Pixmap.height());
        uilabel->setPixmap(Pixmap);
        return;
    }

    Pixmap.convertFromImage(QImage(image.data(), image.width(), image.height(), image.linesize(), QImage::Format_RGB888));
    uilabel->setGeometry(0, 0, Pixmap.width(), Pixmap.height());
    uilabel->setPixmap(Pixmap);

    int percents = scaleFactor * 100 + 0.5;

    ui->scalePercentage_spinBox->blockSignals(true);
    ui->scalePercentage_spinBox->setValue(percents);
    ui->scalePercentage_spinBox->blockSignals(false);

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

    setSelectionArea(selectionPos.x(), selectionPos.y(), selectionSize.width(), selectionSize.height());
}

QSize ImageLabel::pixmapSize() const
{
    return Pixmap.size();
}

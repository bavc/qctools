#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QResizeEvent>
#include <QFrame>
#include <QLabel>

namespace Ui {
class ImageLabel;
}

class FFmpeg_Glue;
class SelectionArea;
class ImageLabel : public QFrame
{
    Q_OBJECT

public:
    explicit ImageLabel(FFmpeg_Glue** Picture, size_t Pos, QWidget *parent=NULL);
    ~ImageLabel();

    void                        Remove ();
    void                        updatePixmap(const QImage& image = QImage());
    void                        setPixmap(const QPixmap& pixmap);

    size_t                      GetPos() const;

public Q_SLOTS:

    void setImage(const QImage& image);
    void moveSelectionX(double value);
    void moveSelectionY(double value);
    void changeSelectionWidth(double value);
    void changeSelectionHeight(double value);

    void setMinSelectionWidth(double w);
    void setMaxSelectionWidth(double w);

    void setMinSelectionSize(QSizeF size);
    void setMaxSelectionSize(QSizeF size);

    void setSelectionArea(double x, double y, double w, double h);
    void clearSelectionArea();

    void showDebugOverlay(bool enable);

private Q_SLOTS:
    void on_fitToScreen_radioButton_toggled(bool value);
    void on_normalScale_radioButton_toggled(bool value);
    void on_scalePercentage_doubleSpinBox_valueChanged(double value);

Q_SIGNALS:

    void selectionChanged(const QRectF& geometry);
    void selectionChangeFinished(const QRectF& geometry);

protected:
    void resizeEvent(QResizeEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

    bool needRescale();
    void rescale(const QSize& newSize = QSize());
private:
    Ui::ImageLabel *ui;
    SelectionArea* selectionArea;

    QPointF selectionPos;
    QSizeF selectionSize;

    QSizeF maxSelectionAreaSize;
    QSizeF minSelectionAreaSize;

private:
    QPixmap                     Pixmap;
    FFmpeg_Glue**               Picture;
    size_t                      Pos;
    bool                        debugOverlay;
    QLabel* uilabel;
};

#endif // IMAGELABEL_H

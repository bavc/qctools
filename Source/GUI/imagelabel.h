#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QResizeEvent>
#include <QWidget>

namespace Ui {
class ImageLabel;
}

class FFmpeg_Glue;
class SelectionArea;
class ImageLabel : public QWidget
{
    Q_OBJECT

public:
    explicit ImageLabel(FFmpeg_Glue** Picture, size_t Pos, QWidget *parent=NULL);
    ~ImageLabel();

    void                        Remove ();
    bool                        UpdatePixmap();
    size_t                      GetPos() const;

public Q_SLOTS:

    void moveSelectionX(double value);
    void moveSelectionY(double value);
    void changeSelectionWidth(double value);
    void changeSelectionHeight(double value);

    void setMinSelectionWidth(double w);
    void setMaxSelectionWidth(double w);

    void setMinSelectionSize(QSizeF size);
    void setMaxSelectionSize(QSizeF size);

    void setSelectionArea(double x, double y, double w, double h);
    void showDebugOverlay(bool enable);

Q_SIGNALS:

    void selectionChanged(const QRectF& geometry);
    void selectionChangeFinished(const QRectF& geometry);

protected:
    void resizeEvent(QResizeEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

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
};

#endif // IMAGELABEL_H

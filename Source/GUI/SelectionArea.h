#ifndef AIRUBBERBAND_H
#define AIRUBBERBAND_H

#include <QRubberBand>

class SelectionArea : public QWidget
{
    Q_OBJECT
public:
    SelectionArea(QWidget * p = 0);
    ~SelectionArea();

    enum TrackerHit {
		hitNothing = -1,
		hitTopLeft = 0, 
		hitTopRight = 1, 
		hitBottomRight = 2, 
		hitBottomLeft = 3,
		hitTop = 4, 
		hitRight = 5, 
		hitBottom = 6, 
		hitLeft = 7, 
		hitMiddle = 8
	};
    Q_ENUM(TrackerHit);

public Q_SLOTS:
    void setMaxSize(int width, int height);
    void setMinSize(int width, int height);
    void showDebugOverlay(bool enable);

Q_SIGNALS:
    void geometryChanged(const QRect& rect);
    void geometryChangeFinished();

protected:
	bool eventFilter(QObject * watched, QEvent * event);
    void paintEvent(QPaintEvent* event);

    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);

private:
	TrackerHit hitTest(const QPoint& point) const;
	QCursor* cursorForHitArea(TrackerHit) const;

	TrackerHit hitArea;
	QPoint position;
	QRect startGeometry;

    int maximumWidth;
    int maximumHeight;

    int minimumWidth;
    int minimumHeight;

	QCursor defaultCursor;
	QCursor* diagResizeCursor1;
	QCursor* diagResizeCursor2;
	QCursor* horResizeCursor;
	QCursor* verResizeCursor;
	QCursor* moveCursor;

    bool debugOverlay;
};

#endif // AIRUBBERBAND_H

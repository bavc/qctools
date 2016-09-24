#ifndef AIRUBBERBAND_H
#define AIRUBBERBAND_H

#include <QRubberBand>
#include <QFrame>

class AIRubberband : public QRubberBand
{
    Q_OBJECT
public:
    AIRubberband(Shape s, QWidget * p = 0);
    ~AIRubberband();

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

    void reset();
Q_SIGNALS:
    void geometryChanged(const QRect& rect);

protected:
	bool eventFilter(QObject * watched, QEvent * event);

private:
	TrackerHit hitTest(const QPoint& point) const;
	QCursor* cursorForHitArea(TrackerHit) const;

	TrackerHit hitArea;
	QPoint position;
	QRect startGeometry;

	QWidget* parentWidget;

	QCursor defaultCursor;
	QCursor* diagResizeCursor1;
	QCursor* diagResizeCursor2;
	QCursor* horResizeCursor;
	QCursor* verResizeCursor;
	QCursor* moveCursor;
};

#endif // AIRUBBERBAND_H

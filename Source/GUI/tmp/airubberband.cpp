#include "airubberband.h"
#include <QtGui>

AIRubberband::AIRubberband(Shape s, QWidget * p)
    : QRubberBand(s, p), parentWidget(p), hitArea(hitNothing)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);

	p->installEventFilter(this);
	p->setMouseTracking(true);
	setMouseTracking(true);

	defaultCursor = p->cursor();

	diagResizeCursor1 = new QCursor(Qt::SizeFDiagCursor);
	diagResizeCursor2 = new QCursor(Qt::SizeBDiagCursor);
	horResizeCursor = new QCursor(Qt::SizeHorCursor);
	verResizeCursor = new QCursor(Qt::SizeVerCursor);
	moveCursor = new QCursor(Qt::SizeAllCursor);
}

AIRubberband::~AIRubberband()
{
	delete diagResizeCursor1;
	delete diagResizeCursor2;
	delete horResizeCursor;
	delete verResizeCursor;
	delete moveCursor;

    parentWidget->removeEventFilter(this);
}

void AIRubberband::reset()
{
    parentWidget->setCursor(defaultCursor);
    hitArea = hitNothing;
}

bool AIRubberband::eventFilter(QObject * watched, QEvent * event)
{
	if(watched == parent()) {
		if(event->type() == QEvent::MouseButtonPress) {

			QMouseEvent* e = (QMouseEvent*) event;
			position = e->globalPos();
			startGeometry = geometry();
			hitArea = hitTest(e->pos());

		} else if(event->type() == QEvent::MouseButtonRelease) {

			hitArea = hitNothing;

		} else if(event->type() == QEvent::MouseMove) {

			QMouseEvent* e = (QMouseEvent*) event;

			if(hitArea == hitNothing) {
				QCursor* cursor = cursorForHitArea(hitTest(e->pos()));
				
				if(cursor->shape() != parentWidget->cursor().shape())
					parentWidget->setCursor(*cursor);
				return false;
			}

			QRect currentGeometry = geometry();

			if(hitArea == hitTopLeft) {
				currentGeometry.setTopLeft(e->pos());
			} else if(hitArea == hitTop) {
				currentGeometry.setTop(e->y());
			} else if(hitArea == hitTopRight) {
				currentGeometry.setTopRight(e->pos());
			} else if(hitArea == hitRight) {
				currentGeometry.setRight(e->x());
			} else if(hitArea == hitBottomRight) {
				currentGeometry.setBottomRight(e->pos());
			} else if(hitArea == hitBottom) {
				currentGeometry.setBottom(e->y());
			} else if(hitArea == hitBottomLeft) {
				currentGeometry.setBottomLeft(e->pos());
			} else if(hitArea == hitLeft) {
				currentGeometry.setLeft(e->x());
			} else if(hitArea == hitMiddle) {
				currentGeometry = startGeometry.translated(e->globalPos() - position);
			}

			setGeometry(currentGeometry);

            Q_EMIT geometryChanged(currentGeometry);
            parentWidget->update();
		}
	}

	return false;
}

QCursor* AIRubberband::cursorForHitArea(AIRubberband::TrackerHit hitArea) const 
{
	switch(hitArea) {
		case hitNothing:
			return const_cast<QCursor*> (&defaultCursor);
		case hitTopLeft:
			return diagResizeCursor1;
		case hitTopRight:
			return diagResizeCursor2;
		case hitBottomRight: 
			return diagResizeCursor1;
		case hitBottomLeft:
			return diagResizeCursor2;
		case hitTop:
			return verResizeCursor;
		case hitRight: 
			return horResizeCursor;
		case hitBottom: 
			return verResizeCursor;
		case hitLeft: 
			return horResizeCursor;
		case hitMiddle:
			return moveCursor;
		default:
			return moveCursor;
	}
}

AIRubberband::TrackerHit AIRubberband::hitTest(const QPoint& point) const
{
	int w = width();
	int h = height();
	int b = 4;

	// position inside rubberband
	QPoint rp = mapFrom((QWidget*) parent(), point);
	int x = rp.x();
	int y = rp.y();

	if(x < 0 - b || x > w + b || y < 0 - b || y > h + b) { 
		return hitNothing;
	} else if(x < b && y < b) {
		return hitTopLeft;
	} else if(x > w - b && y < b) {
		return hitTopRight;
	} else if(y < b) {
		return hitTop;
	} else if(y > h - b && x > w - b) {
		return hitBottomRight;
	} else if(x > w - b) {
		return hitRight;
	} else if(x < b && y > h - b) {
		return hitBottomLeft;
	} else if(y > h - b) {
		return hitBottom;
	} else if(x < b) {
		return hitLeft;
	} else {
		return hitMiddle;
	}
}

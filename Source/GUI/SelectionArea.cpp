#include "SelectionArea.h"
#include <QtGui>

SelectionArea::SelectionArea(QWidget * p)
    : QWidget(p), hitArea(hitNothing), maximumWidth(0), maximumHeight(0), minimumWidth(0), minimumHeight(0), debugOverlay(false)
{
    setAttribute(Qt::WA_TranslucentBackground);

	p->installEventFilter(this);

	defaultCursor = p->cursor();

	diagResizeCursor1 = new QCursor(Qt::SizeFDiagCursor);
	diagResizeCursor2 = new QCursor(Qt::SizeBDiagCursor);
	horResizeCursor = new QCursor(Qt::SizeHorCursor);
	verResizeCursor = new QCursor(Qt::SizeVerCursor);
	moveCursor = new QCursor(Qt::SizeAllCursor);

}

SelectionArea::~SelectionArea()
{
	delete diagResizeCursor1;
	delete diagResizeCursor2;
	delete horResizeCursor;
	delete verResizeCursor;
	delete moveCursor;

    parentWidget()->removeEventFilter(this);
}

void SelectionArea::setMaxSize(int width, int height)
{
    maximumWidth = width;
    maximumHeight = height;
}

void SelectionArea::setMinSize(int width, int height)
{
    minimumWidth = width;
    minimumHeight = height;
}

void SelectionArea::showDebugOverlay(bool enable)
{
    debugOverlay = enable;
}

bool SelectionArea::eventFilter(QObject * watched, QEvent * event)
{
	if(watched == parent()) {
		if(event->type() == QEvent::MouseButtonPress) {

			QMouseEvent* e = (QMouseEvent*) event;
			position = e->globalPos();
			startGeometry = geometry();
			hitArea = hitTest(e->pos());

		} else if(event->type() == QEvent::MouseButtonRelease) {

			hitArea = hitNothing;
            Q_EMIT geometryChangeFinished();

		} else if(event->type() == QEvent::MouseMove) {

			QMouseEvent* e = (QMouseEvent*) event;

			if(hitArea == hitNothing) {
				QCursor* cursor = cursorForHitArea(hitTest(e->pos()));
				
                if(cursor->shape() != parentWidget()->cursor().shape())
                    parentWidget()->setCursor(*cursor);
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

            if(hitArea != hitMiddle)
            {
                if(maximumWidth != 0 && currentGeometry.width() > maximumWidth)
                    currentGeometry.setWidth(maximumWidth);
                if(maximumHeight != 0 && currentGeometry.height() > maximumHeight)
                    currentGeometry.setHeight(maximumHeight);
                if(minimumWidth != 0 && currentGeometry.width() < minimumWidth)
                    currentGeometry.setWidth(minimumWidth);
                if(minimumHeight != 0 && currentGeometry.height() < minimumHeight)
                    currentGeometry.setHeight(minimumHeight);

            } else
            {
                int w = currentGeometry.width();
                int h = currentGeometry.height();

                if(currentGeometry.left() < 0)
                {
                    currentGeometry.setLeft(0);
                    currentGeometry.setWidth(w);
                }

                if(currentGeometry.right() > parentWidget()->width())
                {
                    currentGeometry.setRight(parentWidget()->width());
                    currentGeometry.setLeft(currentGeometry.right() - w);
                }

                if(currentGeometry.top() < 0)
                {
                    currentGeometry.setTop(0);
                    currentGeometry.setHeight(h);
                }

                if(currentGeometry.bottom() > parentWidget()->height())
                {
                    currentGeometry.setBottom(parentWidget()->height());
                    currentGeometry.setTop(currentGeometry.bottom() - h);
                }
            }

            setGeometry(currentGeometry);
            Q_EMIT geometryChanged(currentGeometry);
		}
	}

    return false;
}

void SelectionArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    {
        QPen pen(Qt::black, 2);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawRect(event->rect());
    }

    {
        QPen pen(Qt::green, 2);
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawRect(event->rect());
    }


    if(debugOverlay)
    {
        QPen pen(Qt::green, 2, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawText(2, height() - 10,
                        QString("x: %1, y: %2, %3x%4")
                         .arg(x())
                         .arg(y())
                         .arg(width())
                         .arg(height()));
    }
}

void SelectionArea::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    setMouseTracking(false);
    parentWidget()->setMouseTracking(false);
}

void SelectionArea::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    setMouseTracking(true);
    parentWidget()->setMouseTracking(true);
}

QCursor* SelectionArea::cursorForHitArea(SelectionArea::TrackerHit hitArea) const
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

SelectionArea::TrackerHit SelectionArea::hitTest(const QPoint& point) const
{
	int w = width();
	int h = height();
	int b = 4;

    // position inside selection area
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

#include "SelectionArea.h"
#include <QGraphicsSceneMouseEvent>
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

/////////////////

SelectionAreaGraphicsObject::SelectionAreaGraphicsObject(QGraphicsObject* p)
    : QGraphicsObject(p), hitArea(hitNothing), debugOverlay(false), parentItem(p)
{
    defaultCursor = p->cursor();

    diagResizeCursor1 = new QCursor(Qt::SizeFDiagCursor);
    diagResizeCursor2 = new QCursor(Qt::SizeBDiagCursor);
    horResizeCursor = new QCursor(Qt::SizeHorCursor);
    verResizeCursor = new QCursor(Qt::SizeVerCursor);
    moveCursor = new QCursor(Qt::SizeAllCursor);
}

SelectionAreaGraphicsObject::~SelectionAreaGraphicsObject()
{
    delete diagResizeCursor1;
    delete diagResizeCursor2;
    delete horResizeCursor;
    delete verResizeCursor;
    delete moveCursor;
}

QRectF SelectionAreaGraphicsObject::geometry() const
{
    return m_boundingRect;
}

void SelectionAreaGraphicsObject::showDebugOverlay(bool enable)
{
    debugOverlay = enable;
}

void SelectionAreaGraphicsObject::setGeometry(const QRectF &geometry)
{
    if(m_boundingRect != geometry) {
        prepareGeometryChange();
        m_boundingRect = QRectF(0, 0, geometry.width(), geometry.height());
        update();

        setX(geometry.x());
        setY(geometry.y());
    }
}

void SelectionAreaGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    auto e = event;

    mouseOffset = this->pos() - mapToScene(event->pos());
    startGeometry = boundingRect().toRect();
    hitArea = hitTest(e->pos());
}

void SelectionAreaGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto e = event;

    if(hitArea == hitNothing) {
        QCursor* cursor = cursorForHitArea(hitTest(e->pos()));

        if(cursor->shape() != this->cursor().shape())
            this->setCursor(*cursor);
        return;
    }

    auto currentGeometry = boundingRect();
    int w = currentGeometry.width();
    int h = currentGeometry.height();

    currentGeometry.moveTo(x(), y());
    auto scenePos = mapToScene(e->pos());

    if(hitArea == hitTopLeft) {
        if(scenePos.x() < 0)
            scenePos.setX(0);
        if(scenePos.y() < 0)
            scenePos.setY(0);

        currentGeometry.setTopLeft(scenePos);
    } else if(hitArea == hitTop) {
        if(scenePos.y() < 0)
            scenePos.setY(0);

        currentGeometry.setTop(scenePos.y());
    } else if(hitArea == hitTopRight) {
        if(scenePos.y() < 0)
            scenePos.setY(0);
        if(scenePos.x() > parentItem->boundingRect().width())
            scenePos.setX(parentItem->boundingRect().width());

        currentGeometry.setTopRight(scenePos);
    } else if(hitArea == hitRight) {
        if(scenePos.x() > parentItem->boundingRect().width())
            scenePos.setX(parentItem->boundingRect().width());

        currentGeometry.setRight(scenePos.x());
    } else if(hitArea == hitBottomRight) {
        if(scenePos.y() > parentItem->boundingRect().height())
            scenePos.setY(parentItem->boundingRect().height());
        if(scenePos.x() > parentItem->boundingRect().width())
            scenePos.setX(parentItem->boundingRect().width());

        currentGeometry.setBottomRight(scenePos);
    } else if(hitArea == hitBottom) {
        if(scenePos.y() > parentItem->boundingRect().height())
            scenePos.setY(parentItem->boundingRect().height());

        currentGeometry.setBottom(scenePos.y());
    } else if(hitArea == hitBottomLeft) {
        if(scenePos.y() > parentItem->boundingRect().height())
            scenePos.setY(parentItem->boundingRect().height());
        if(scenePos.x() < 0)
            scenePos.setX(0);

        currentGeometry.setBottomLeft(scenePos);
    } else if(hitArea == hitLeft) {
        if(scenePos.x() < 0)
            scenePos.setX(0);

        currentGeometry.setLeft(scenePos.x());
    } else if(hitArea == hitMiddle) {
        currentGeometry = startGeometry.translated(scenePos + mouseOffset);

        if(currentGeometry.left() < 0)
        {
            currentGeometry.setLeft(0);
            currentGeometry.setWidth(w);
        }

        if(currentGeometry.right() > parentItem->boundingRect().width())
        {
            currentGeometry.setRight(parentItem->boundingRect().width());
            currentGeometry.setLeft(currentGeometry.right() - w);
        }

        if(currentGeometry.top() < 0)
        {
            currentGeometry.setTop(0);
            currentGeometry.setHeight(h);
        }

        if(currentGeometry.bottom() > parentItem->boundingRect().height())
        {
            currentGeometry.setBottom(parentItem->boundingRect().height());
            currentGeometry.setTop(currentGeometry.bottom() - h);
        }
    }

    setGeometry(currentGeometry);
    Q_EMIT geometryChanged(currentGeometry.toRect());
}

void SelectionAreaGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    auto e = event;
    qDebug() << "mouseReleaseEvent: " << boundingRect() << e->pos();

    hitArea = hitNothing;
    Q_EMIT geometryChangeFinished();
}

void SelectionAreaGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    auto e = event;
    qDebug() << "hoverEnterEvent: " << boundingRect() << e->pos();

    if(hitArea == hitNothing) {
        QCursor* cursor = cursorForHitArea(hitTest(e->pos()));

        if(cursor->shape() != this->cursor().shape())
            this->setCursor(*cursor);
        return;
    }
}

void SelectionAreaGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    auto e = event;
    qDebug() << "hoverLeaveEvent: " << boundingRect() << e->pos();

    hitArea = hitNothing;
}

void SelectionAreaGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    auto e = event;

    if(hitArea == hitNothing) {
        QCursor* cursor = cursorForHitArea(hitTest(e->pos()));

        if(cursor->shape() != this->cursor().shape())
            this->setCursor(*cursor);
        return;
    }
}

void SelectionAreaGraphicsObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    {
        QPen pen(Qt::black, 2);
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        painter->drawRect(boundingRect());
    }

    {
        QPen pen(Qt::green, 2);
        pen.setStyle(Qt::DotLine);
        painter->setPen(pen);
        painter->drawRect(boundingRect());
    }


    if(debugOverlay)
    {
        QPen pen(Qt::green, 2, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawText(2, boundingRect().height() - 10,
                         QString("x: %1, y: %2, %3x%4")
                             .arg(x())
                             .arg(y())
                             .arg(boundingRect().width())
                             .arg(boundingRect().height()));
    }

}

QRectF SelectionAreaGraphicsObject::boundingRect() const
{
    return m_boundingRect;
}

QCursor* SelectionAreaGraphicsObject::cursorForHitArea(SelectionAreaGraphicsObject::TrackerHit hitArea) const
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

SelectionAreaGraphicsObject::TrackerHit SelectionAreaGraphicsObject::hitTest(const QPointF& point) const
{
    int w = m_boundingRect.width();
    int h = m_boundingRect.height();
    int b = 8;

    // position inside selection area
    auto rp = point;
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

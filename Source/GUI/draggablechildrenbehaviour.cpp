#include "draggablechildrenbehaviour.h"
#include <QDrag>
#include <QEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QWidget>
#include <QDebug>
#include <QApplication>

const static QString draggableMimeType = "qapplication/dragable";
const static QString draggableOldIndexMimeType = "qapplication/dragable-old-index";

DraggableChildrenBehaviour::DraggableChildrenBehaviour(QBoxLayout *layout) : QObject(layout->parentWidget()), layout(layout), dropIndicator(new QFrame(layout->parentWidget()))
{
    parent = layout->parentWidget();
    parent->setAcceptDrops(true);
    parent->installEventFilter(this);

    dropIndicator->setFrameShape(isHorizontalLayout() ? QFrame::VLine : QFrame::HLine);
    dropIndicator->hide();

    for(auto child : parent->children())
    {
        child->installEventFilter(this);
    }
}

void DraggableChildrenBehaviour::newDrag(QWidget *watched)
{
    QDrag *drag = new QDrag(this);

    auto mimeData = new QMimeData();
    const QByteArray data = QByteArray::number((quintptr)watched);

    mimeData->setData(draggableMimeType, data);
    mimeData->setData(draggableOldIndexMimeType, QByteArray::number(layout->indexOf(watched)));

    drag->setMimeData(mimeData);
    drag->start();
}

bool DraggableChildrenBehaviour::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == parent)
    {
        if(event->type() == QEvent::ChildAdded)
        {
            auto addedChild = static_cast<QChildEvent*> (event)->child();
            auto addedChildWidget = qobject_cast<QWidget*>(addedChild);

            if(addedChildWidget)
                addedChildWidget->installEventFilter(this);
        }
        else if(event->type() == QEvent::DragEnter)
        {
            auto dragEnterEvent = static_cast<QDragEnterEvent*> (event);
            if(dragEnterEvent->mimeData()->hasFormat(draggableMimeType))
                dragEnterEvent->accept();
        }
        else if(event->type() == QEvent::DragMove)
        {
            auto dragMoveEvent = static_cast<QDragMoveEvent*> (event);

            QWidget *child = parent->childAt(dragMoveEvent->pos());
            while(child && layout->indexOf(child) == -1)
                child = child->parentWidget();

            const int currentDropIndex = layout->indexOf(dropIndicator);

            if(child)
            {
                int newDropIndex = layout->indexOf(child);
                auto centerOfChild = child->geometry().center();

                qDebug() << "pos: " << dragMoveEvent->pos() << ", child.center: " << centerOfChild << ", newDropIndex: " << newDropIndex;

                bool afterChild = false;
                if(isHorizontalLayout())
                {
                    afterChild = (dragMoveEvent->pos().x() > centerOfChild.x());
                } else
                {
                    afterChild = (dragMoveEvent->pos().y() > centerOfChild.y());
                }

                if(afterChild)
                    ++newDropIndex;

                if(currentDropIndex != -1 && currentDropIndex < newDropIndex)
                    newDropIndex--;

                if(newDropIndex != currentDropIndex)
                {
                    qDebug() << "DragMove: " << "newDropIndex = " << newDropIndex << ", currentDropIndex = " << currentDropIndex;

                    if(currentDropIndex != -1)
                        layout->removeWidget(dropIndicator);

                    layout->insertWidget(newDropIndex, dropIndicator);
                    dropIndicator->show();
                }
            } else {
                qDebug() << "No child";
            }
        } else if(event->type() == QEvent::DragLeave)
        {
            auto dragLeaveEvent = static_cast<QDragLeaveEvent*> (event);

            layout->removeWidget(dropIndicator);
            dropIndicator->hide();
            event->accept();

        } else if(event->type() == QEvent::Drop)
        {
            auto dropEvent = static_cast<QDropEvent*> (event);

            const QByteArray data = dropEvent->mimeData()->data(draggableMimeType);
            QWidget* draggingWidget = reinterpret_cast<QWidget*> (data.toULongLong());
            int newIndex = layout->indexOf(dropIndicator);
            if(newIndex != -1)
            {
                const int currentIndex = layout->indexOf(draggingWidget);
                bool positionChanged = false;

                if(currentIndex != -1 && currentIndex != newIndex)
                {
                    if(currentIndex < newIndex)
                        newIndex--;

                    qDebug() << "Drop: " << "newIndex = " << newIndex;

                    layout->removeWidget(draggingWidget);
                    layout->insertWidget(newIndex, draggingWidget);
                    positionChanged = true;
                }

                layout->removeWidget(dropIndicator);
                dropIndicator->hide();

                dropEvent->accept();

                const int oldIndex = dropEvent->mimeData()->data(draggableOldIndexMimeType).toInt();

                if(positionChanged && oldIndex != newIndex)
                    Q_EMIT childPositionChanged(draggingWidget, oldIndex, newIndex);

            } else {

                qDebug() << "Attempt to drop without drop indicator in layout, discarding drop event.. ";
                dropEvent->ignore();
            }
        }

    } else if(watched != parent)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton)
            {
                mouseClickedPos = mouseEvent->pos();
            }
        }
        else if(event->type() == QEvent::MouseMove)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->buttons() & Qt::LeftButton)
            {
                int distance = (mouseEvent->pos() - mouseClickedPos).manhattanLength();
                if(distance >= 5)
                {
                    newDrag(static_cast<QWidget*> (watched));
                }
            }
        }
    }

    return false;
}

bool DraggableChildrenBehaviour::isHorizontalLayout() const
{
    return layout->direction() == QBoxLayout::LeftToRight || layout->direction() == QBoxLayout::RightToLeft;
}

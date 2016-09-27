#ifndef DRAGGABLECHILDRENBEHAVIOUR_H
#define DRAGGABLECHILDRENBEHAVIOUR_H

#include <QBoxLayout>
#include <QFrame>
#include <QObject>

class DraggableChildrenBehaviour : public QObject
{
    Q_OBJECT
public:
    explicit DraggableChildrenBehaviour(QBoxLayout *layout = 0);

protected:
    bool eventFilter(QObject *watched, QEvent *event);

Q_SIGNALS:
    void childPositionChanged(QWidget* child, int oldPos, int newPos);

private:
    void newDrag(QWidget *watched);
    bool isHorizontalLayout() const;

private:
    QWidget* parent;
    QBoxLayout* layout;
    QFrame* dropIndicator;
    QPoint mouseClickedPos;
};

#endif // DRAGGABLECHILDRENBEHAVIOUR_H

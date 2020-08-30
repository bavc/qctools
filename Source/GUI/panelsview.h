#ifndef PANELSVIEW_H
#define PANELSVIEW_H

#include <QFrame>
#include <QSize>
#include <functional>

class PanelsView : public QFrame
{
    Q_OBJECT
public:
    explicit PanelsView(QWidget *parent = nullptr);
    void setProvider(const std::function<int()>& getPanelsCount,
                     const std::function<QImage(int)>& getPanelImage);

    int panelIndexByFrame(int frameIndex) const;
    void getPanelsBounds(int& startPanelIndex, int& startPanelOffset, int& endPanelIndex, int& endPanelLength);
    void refresh();

public Q_SLOTS:
    void setVisibleFrames(int from, int to);
    void setActualWidth(int width);

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *event);

Q_SIGNALS:

private:
    int m_startFrame;
    int m_endFrame;
    int m_actualWidth;
    std::function<int()> getPanelsCount;
    std::function<QImage(int)> getPanelImage;
};

#endif // PANELSVIEW_H

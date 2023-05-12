#include "panelsview.h"
#include "Comments.h"
#include "Plot.h"
#include <QDebug>
#include <QPainter>
#include <QPushButton>
#include <QWheelEvent>

class PanelCursor : public PlotCursor {
public:
    PanelCursor( QWidget *canvas, QwtPlot* plot ): PlotCursor(canvas), m_plot(plot)
    {
    }

    virtual int translatedPos( double pos ) const
    {
        // translate from plot into widget coordinate

        const QwtPlot* plot = m_plot;
        if ( plot )
        {
            const QwtScaleMap scaleMap = plot->canvasMap( QwtPlot::xBottom );
            auto translated = qRound( scaleMap.transform( pos ) ) + parentWidget()->contentsMargins().left();

            // qDebug() << "translated: " << translated;
            return translated;
        }

        return -1;
    }

    QwtPlot* m_plot;
};

PanelsView::PanelsView(QWidget *parent, const QString& panelTitle, const QString& yaxis, const QString& legend, CommentsPlot* plot) : QFrame(parent), m_actualWidth(0), m_plot(plot)
{
    qDebug() << "creating PanelsView: " << panelTitle;

    m_PlotCursor = new PanelCursor(this, plot);
    m_PlotCursor->setPosition( 0 );
    m_panelTitle = panelTitle;
    m_panelGroup = qHash(m_panelTitle);

    auto picker = new QwtPanelPicker(this, plot);
    connect(picker, SIGNAL(moved(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));
    connect(picker, SIGNAL(selected(const QPointF&)), SLOT(onPickerMoved(const QPointF&)));

    auto plotHeight = 30;

    auto splitted = yaxis.split(":");
    if(splitted.length() == 1)
    {
        m_middleYLabel = splitted[0];
    }
    if(splitted.length() == 2)
    {
        m_bottomYLabel = splitted[0];
        m_topYLabel = splitted[1];
    } else if(splitted.length() == 3)
    {
        m_bottomYLabel = splitted[0];
        m_middleYLabel = splitted[1];
        m_topYLabel = splitted[2];
    }

    m_plotLegend = new PlotLegend();

    /*
    connect( this, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
         m_legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );
    */

    QwtLegendData titleData;
    titleData.setValue(QwtLegendData::TitleRole, legend);

    m_plotLegend->updateLegend(QVariant("title"), QList<QwtLegendData> { titleData });
}

PanelsView::~PanelsView()
{
    delete m_legend;
}

void PanelsView::setProvider(const std::function<int ()> &getPanelsCount, const std::function<QImage (int)> &getPanelImage)
{
    this->getPanelsCount = getPanelsCount;
    this->getPanelImage = getPanelImage;
}

void PanelsView::getPanelsBounds(int &startPanelIndex, int &startPanelOffset, int &endPanelIndex, int &endPanelLength)
{
    auto panelSize = getPanelImage(0).size();
    if(panelSize.isEmpty()) {
        startPanelIndex = 0;
        startPanelOffset = 0;
        endPanelIndex = 0;
        endPanelLength = 0;
        return;
    }

    auto panelWidth = panelSize.width();

    startPanelOffset = m_startFrame % panelWidth;
    startPanelIndex = (m_startFrame - startPanelOffset) / panelWidth;

    endPanelLength = m_endFrame % panelWidth;
    endPanelIndex = (m_endFrame - endPanelLength) / panelWidth;
}

void PanelsView::refresh()
{
    m_panelPixmap = QPixmap();
    repaint();
    m_PlotCursor->updateOverlay();
}

void PanelsView::setVisibleFrames(int from, int to)
{
    m_startFrame = from;
    m_endFrame = to;
    qDebug() << "from: " << from << "to: " << to;

    refresh();
}

void PanelsView::setActualWidth(int actualWidth)
{
    if(m_actualWidth != actualWidth)
    {
        m_actualWidth = actualWidth;
        refresh();
    }
}

void PanelsView::setCursorPos(double x)
{
    m_PlotCursor->setPosition( x );
}

void PanelsView::setLeftOffset(int leftOffset)
{
    m_leftOffset = leftOffset;
}

void PanelsView::onPickerMoved(const QPointF &pos)
{
    const int idx = m_plot->frameAt( pos.x() );
    if ( idx >= 0 )
        Q_EMIT cursorMoved( idx );
}

QColor fillColor("Cornsilk");

void PanelsView::paintEvent(QPaintEvent *e)
{
    const auto leftMargin = contentsMargins().left();
    const auto rightMargin = contentsMargins().right();
    const auto w = width();
    const auto h = height();

    QPainter p(this);

    p.drawRect(QRect(leftMargin,
                     lineWidth() - 1,
                     w - (leftMargin + rightMargin + 1),
                     h - (lineWidth() - 1) * 2));

    p.save();
    auto font = this->font();
    p.setFont(font);
    QFontMetrics metrics(font);

    if(!m_topYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, leftMargin, h),
                                                   Qt::TextWordWrap, m_topYLabel);

        auto topYLabelWidth = rect.width();

        p.setPen(Qt::black);
        p.drawText(QRect(leftMargin - m_leftOffset / 2 - topYLabelWidth,
                          lineWidth(), rect.width(), rect.height()), m_topYLabel);
    }

    if(!m_middleYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, leftMargin, h),
                                                   Qt::TextWordWrap, m_middleYLabel);

        auto middleYLabelWidth = rect.width();

        p.setPen(Qt::black);
        p.drawText(QRect(leftMargin - m_leftOffset / 2 - middleYLabelWidth,
                          h / 2 - (lineWidth() -1) - rect.height() / 2, rect.width(), rect.height()), m_middleYLabel);
    }

    if(!m_bottomYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, leftMargin, h),
                                                   Qt::TextWordWrap, m_bottomYLabel);

        auto bottomYLabelWidth = rect.width();
        auto bottomYLabelHeight = rect.height();

        p.drawText(QRect(leftMargin - m_leftOffset / 2 - bottomYLabelWidth,
                          h - (lineWidth() - 1) - rect.height(), rect.width(), rect.height()), m_bottomYLabel);
    }

    p.restore();

    if(m_panelPixmap.isNull())
    {
        auto availableWidth = w - (leftMargin + m_leftOffset + 1) - (rightMargin + m_leftOffset - 1);
        auto availableHeight = h - lineWidth();

        auto panelsCount = getPanelsCount();
        if(panelsCount == 0) {
            m_panelPixmap = QPixmap(availableWidth, availableHeight);
            m_panelPixmap.fill(fillColor);

            p.drawPixmap(leftMargin + m_leftOffset + 1, lineWidth(), m_panelPixmap);
            return;
        }

        auto sx = m_actualWidth == 0 ? 1 : ((qreal) availableWidth / m_actualWidth);
        auto sy = (qreal)availableHeight / getPanelImage(0).height();

        m_panelPixmap = QPixmap(availableWidth, availableHeight);
        m_panelPixmap.fill(fillColor);
        QPainter p(&m_panelPixmap);

        auto totalFrames = m_endFrame - m_startFrame;
        auto zx = (qreal) m_actualWidth / totalFrames;

        QTransform scalingViewMatrix(sx, 0, 0, sy, 0, 0);
        QTransform scalingZoomMatrix(zx, 0, 0, 1, 0, 0);
        p.setTransform(scalingViewMatrix * scalingZoomMatrix);

        int startPanelOffset, startPanelIndex, endPanelLength, endPanelIndex;
        getPanelsBounds(startPanelIndex, startPanelOffset, endPanelIndex, endPanelLength);

        int x = 0;
        int y = 0;

        for(auto i = startPanelIndex; i <= endPanelIndex; ++i) {

            qDebug() << "getPanelsCount(): " << getPanelsCount();
            if(i < getPanelsCount())
            {
                auto image = getPanelImage(i);
                qDebug() << "getPanelImage: " << i << image.width() << image.height();

                auto imageXOffset = ((i == startPanelIndex) ? startPanelOffset : 0);
                auto imageWidth = ((i == endPanelIndex) ? endPanelLength : image.width());
                auto imageHeight = image.height();

                QRect sr(imageXOffset, 0, imageWidth, imageHeight);
                p.drawImage(QPointF(x, y), image, sr);
                x += (sr.width() - imageXOffset);
            }
        }
    }

    // qDebug() << "painting... ";
    p.drawPixmap(leftMargin + m_leftOffset + 1, lineWidth(), m_panelPixmap);
}

void PanelsView::wheelEvent(QWheelEvent *event)
{
    if(getPanelsCount() == 0)
        return;

    auto panelImageHeight = getPanelImage(0).height();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto delta = event->delta();
#else
    auto delta = event->pixelDelta().y();
#endif //

    if(delta > 0) {
        auto newHeight = height() + 10;
        if(newHeight > panelImageHeight)
            newHeight = panelImageHeight;

        m_panelPixmap = QPixmap();
        this->setMinimumHeight(newHeight);
    }
    else if(delta < 0)
    {
        auto newHeight = height() - 10;
        if(newHeight < 100)
            newHeight = 100;

        m_panelPixmap = QPixmap();
        this->setMinimumHeight(newHeight);
    }
}

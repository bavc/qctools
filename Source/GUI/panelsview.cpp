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

PanelsView::PanelsView(QWidget *parent, const QString& panelTitle, const QString& yaxis, CommentsPlot* plot) : QFrame(parent), m_actualWidth(0), m_plot(plot)
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

    m_legend = new PlotLegend();
    m_legend->setMaximumHeight(plotHeight);

    /*
    connect( this, SIGNAL( legendDataChanged( const QVariant &, const QList<QwtLegendData> & ) ),
         m_legend, SLOT( updateLegend( const QVariant &, const QList<QwtLegendData> & ) ) );
    */

    QwtLegendData titleData;
    titleData.setValue(QwtLegendData::TitleRole, m_panelTitle);

    m_legend->updateLegend(QVariant("title"), QList<QwtLegendData> { titleData });
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
    repaint();
    m_PlotCursor->updateOverlay();
}

void PanelsView::setVisibleFrames(int from, int to)
{
    m_startFrame = from;
    m_endFrame = to;
    qDebug() << "from: " << from << "to: " << to;

    repaint();
}

void PanelsView::setActualWidth(int actualWidth)
{
    m_actualWidth = actualWidth;
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

void PanelsView::paintEvent(QPaintEvent *e)
{
    QPainter p;
    p.begin(this);

    p.drawRect(QRect(contentsMargins().left(),
                     lineWidth() - 1,
                     width() - (contentsMargins().left() + contentsMargins().right() + 1),
                     height() - (lineWidth() - 1) * 2));


    p.save();
    auto font = this->font();
    p.setFont(font);
    QFontMetrics metrics(font);

    if(!m_topYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, contentsMargins().left(), height()),
                                                   Qt::TextWordWrap, m_topYLabel);

        auto topYLabelWidth = rect.width();
        auto topYLabelHeight = rect.height();

        p.setPen(Qt::black);
        p.drawText(QRect(contentsMargins().left() - m_leftOffset / 2 - topYLabelWidth,
                          lineWidth(), rect.width(), rect.height()), m_topYLabel);
    }

    if(!m_middleYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, contentsMargins().left(), height()),
                                                   Qt::TextWordWrap, m_middleYLabel);

        auto middleYLabelWidth = rect.width();
        auto middleYLabelHeight = rect.height();

        p.setPen(Qt::black);
        p.drawText(QRect(contentsMargins().left() - m_leftOffset / 2 - middleYLabelWidth,
                          height() / 2 - (lineWidth() -1) - rect.height() / 2, rect.width(), rect.height()), m_middleYLabel);
    }

    if(!m_bottomYLabel.isEmpty())
    {
        auto rect = metrics.boundingRect(QRect(0, 0, contentsMargins().left(), height()),
                                                   Qt::TextWordWrap, m_bottomYLabel);

        auto bottomYLabelWidth = rect.width();
        auto bottomYLabelHeight = rect.height();

        p.drawText(QRect(contentsMargins().left() - m_leftOffset / 2 - bottomYLabelWidth,
                          height() - (lineWidth() - 1) - rect.height(), rect.width(), rect.height()), m_bottomYLabel);
    }

    p.restore();

    auto panelsCount = getPanelsCount();
    if(panelsCount == 0)
        return;

    auto availableWidth = width() - (contentsMargins().left() + m_leftOffset + 1) - (contentsMargins().right() + m_leftOffset - 1);
    auto sx = m_actualWidth == 0 ? 1 : ((qreal) availableWidth / m_actualWidth);
    auto availableHeight = height() - lineWidth();
    auto sy = (qreal)availableHeight / getPanelImage(0).height();

    auto dx = availableWidth - m_actualWidth;

    QRect viewport(contentsMargins().left() + m_leftOffset + 1,
                   lineWidth(),
                   width() - (contentsMargins().right() + m_leftOffset - 1),
                   availableHeight);

    p.setViewport(viewport);

    auto totalFrames = m_endFrame - m_startFrame;
    auto zx = (qreal) m_actualWidth / totalFrames;

    QMatrix scalingViewMatrix(sx, 0, 0, sy, 0, 0);
    QMatrix scalingZoomMatrix(zx, 0, 0, 1, 0, 0);
    p.setMatrix(scalingViewMatrix * scalingZoomMatrix);

    int startPanelOffset, startPanelIndex, endPanelLength, endPanelIndex;
    getPanelsBounds(startPanelIndex, startPanelOffset, endPanelIndex, endPanelLength);

    // qDebug() << "contentsMargins: " << contentsMargins() << "totalFrames: " << totalFrames;

    /*
    qDebug() << "startPanelIndex: " << startPanelIndex << "startPanelOffset: " << startPanelOffset
             << "endPanelIndex: " << endPanelIndex << "endPanelLength: " << endPanelLength << "availableWidth: " << availableWidth << "actual: " << m_actualWidth
             << "height: " << availableHeight;
    */

    // p.fillRect(QRect(0, 0, width(), height()), Qt::green);
    int x = 0;
    int y = 0;

    for(auto i = startPanelIndex; i <= endPanelIndex; ++i) {

        if(i < getPanelsCount())
        {
            auto image = getPanelImage(i);
            auto imageXOffset = ((i == startPanelIndex) ? startPanelOffset : 0);
            auto imageWidth = ((i == endPanelIndex) ? endPanelLength : image.width());
            auto imageHeight = image.height();

            QRect sr(imageXOffset, 0, imageWidth, imageHeight);
            p.drawImage(QPointF(x, y), image, sr);
            //p.fillRect(x, 0, imageWidth, image.height(), Qt::red);

            //qDebug() << "x: " << x << "sr: " << sr;
            x += (sr.width() - imageXOffset);
        }
    }

    p.end();
}

void PanelsView::wheelEvent(QWheelEvent *event)
{
    if(event->delta() > 0) {
        auto newHeight = height() + 10;
        if(newHeight > getPanelImage(0).height())
            newHeight = getPanelImage(0).height();

        this->setMinimumHeight(newHeight);
    }
    else if(event->delta() < 0)
    {
        auto newHeight = height() - 10;
        if(newHeight < 100)
            newHeight = 100;

        this->setMinimumHeight(newHeight);
    }
}

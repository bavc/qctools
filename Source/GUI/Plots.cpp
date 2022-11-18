/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "Core/FFmpeg_Glue.h"
#include "GUI/Plots.h"
#include "GUI/Plot.h"
#include "GUI/PlotLegend.h"
#include "GUI/PlotScaleWidget.h"
#include "GUI/Comments.h"
#include "GUI/CommentsEditor.h"
#include "GUI/barchartconditioneditor.h"
#include "GUI/barchartconditioninput.h"
#include "Core/Core.h"
#include "Core/VideoCore.h"
#include "QAVVideoFrame.h"
#include "playercontrol.h"
#include <QComboBox>
#include <QGridLayout>
#include <QEvent>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <cmath>
#include <clocale>
#include <unordered_set>
#include <QMouseEvent>
#include <QInputDialog>
#include <QTextDocument>
#include <QPushButton>
#include <QCheckBox>
#include <QToolButton>
#include <qwt_plot_curve.h>

//---------------------------------------------------------------------------

class XAxisFormatBox: public QComboBox
{
public:
    XAxisFormatBox( QWidget* parent = NULL ):
        QComboBox( parent )
    {
        setContentsMargins( 0, 0, 0, 0 );

        addItem( "Frames" );
        addItem( "Seconds" );
        addItem( "Minutes" );
        addItem( "Hours" );
        addItem( "Time" );
    }
};

//---------------------------------------------------------------------------
void Plots::showEditBarchartProfileDialog(const size_t plotGroup, Plot* plot, const stream_info& streamInfo)
{
    QDialog dialog;
    dialog.setWindowTitle("Edit barchart conditions");
    QVBoxLayout* grid = new QVBoxLayout;
    dialog.setLayout(grid);

    QList<QPair<PlotSeriesData*, BarchartConditionEditor*>> pairs;

    int j = streamInfo.PerGroup[plotGroup].Count;
    while(j-- > 0)
    {
        auto conditionEditor = new BarchartConditionEditor(nullptr);
        PlotSeriesData* data = plot->getData(j);

        data->mutableConditions().updateAll(m_fileInfoData->BitsPerRawSample());
        auto curve = dynamic_cast<const QwtPlotCurve*>( plot->getCurve(j) );

        QString title = curve->title().text();

        conditionEditor->setLabel(title);
        conditionEditor->setDefaultColor(curve->pen().color());
        conditionEditor->setConditions(data->conditions());
        connect(data, SIGNAL(conditionsUpdated()), conditionEditor, SLOT(onConditionsUpdated()));

        grid->addWidget(conditionEditor, streamInfo.PerGroup[plotGroup].Count - 1 - j);

        pairs.append(QPair<PlotSeriesData*, BarchartConditionEditor*>(data, conditionEditor));
    }

    auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(dialogButtonBox->button(QDialogButtonBox::Save), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(dialogButtonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), &dialog, SLOT(reject()));

    grid->addWidget(dialogButtonBox, streamInfo.PerGroup[plotGroup].Count);
    if(QDialog::Accepted == dialog.exec())
    {
        for(auto pair : pairs) {
            auto data = pair.first;
            auto editor = pair.second;

            if(data->conditions().m_items.size() != editor->conditionsCount())
            {
                if(data->conditions().m_items.size() > editor->conditionsCount())
                {
                    while(data->conditions().m_items.size() > editor->conditionsCount())
                        data->mutableConditions().remove();
                }
                else
                {
                    while(data->conditions().m_items.size() < editor->conditionsCount())
                        data->mutableConditions().add();
                }
            }

            for(auto i = 0; i < editor->conditionsCount(); ++i)
            {
                auto conditionInput = editor->getCondition(i);
                data->mutableConditions().update(i, conditionInput->getCondition(), conditionInput->getColor(),
                                                 conditionInput->getName(), conditionInput->getEliminateSpikes());
            }
        }

        plot->updateSymbols();
        plot->replot();

        Q_EMIT barchartProfileChanged();
    }
}

Plots::Plots( QWidget *parent, FileInformation* fileInformation ) :
    QWidget( parent ),
    m_zoomFactor ( 0 ),
    m_fileInfoData( fileInformation ),
    m_dataTypeIndex( Plots::AxisSeconds ),
    m_commentsPlot(nullptr)
{
    setlocale(LC_NUMERIC, "C");
    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing( 1 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    // bottom scale
    m_scaleWidget = new PlotScaleWidget();
    m_scaleWidget->setFormat( Plots::AxisTime );
    setVisibleFrames( 0, numFrames() - 1 );

    // plots and legends
    m_plots = new Plot**[m_fileInfoData->Stats.size()];
    m_plotsCount = 0;
    
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        if (m_fileInfoData->Stats[streamPos])
        {
            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
            size_t countOfGroups = PerStreamType[type].CountOfGroups;
        
            m_plots[streamPos] = new Plot*[countOfGroups + 1]; //+1 for axix
    
            for ( size_t group = 0; group < countOfGroups; group++ )
            {
                if (m_fileInfoData->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup])
                {
                    Plot* plot = new Plot( streamPos, type, group, m_fileInfoData, this );
                    plot->setObjectName(QString("Plot for stream: %1 of type %2, group %3").arg(streamPos).arg(type).arg(group));

                    connect(plot, &Plot::visibilityChanged, [plot](bool visible) {
                        qDebug() << "Plot::visibilityChanged for " << plot << "visible: " << visible;
                    });

                    const size_t plotType = plot->type();
                    const size_t plotGroup = plot->group();
                    const CommonStats* stat = stats( plot->streamPos() );

                    auto streamInfo = PerStreamType[plotType];

                    plot->addGuidelines(m_fileInfoData->BitsPerRawSample());

                    // we allow to shrink the plot below height of the size hint
                    plot->plotLayout()->setAlignCanvasToScales(false);
                    plot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
                    plot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
                    plot->initYAxis();

                    updateSamples( plot );

                    connect(plot, &Plot::cursorMoved, [this, plot](const QPointF& point, int framePos) {

                        // search for video plot
                        Plot* videoPlot = nullptr;
                        if(plot->type() == Type_Audio) {
                            for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
                            {
                                if (m_fileInfoData->Stats[streamPos] && m_fileInfoData->Stats[streamPos]->Type_Get() == Type_Video);
                                {
                                    size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
                                    size_t countOfGroups = PerStreamType[type].CountOfGroups;
                                    for ( size_t group = 0; group < countOfGroups; group++ )
                                    {
                                        if (m_fileInfoData->ActiveFilters[PerStreamType[type].PerGroup[group].ActiveFilterGroup]) {
                                            videoPlot = m_plots[streamPos][group];
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }

                            if(videoPlot)
                                framePos = videoPlot->frameAt(point.x());
                        }

                        onCursorMoved(framePos);

                    });

                    plot->canvas()->installEventFilter( this );

                    layout->addWidget( plot, m_plotsCount, 0 );
                    QVBoxLayout* legendLayout = new QVBoxLayout();
                    legendLayout->setContentsMargins(5, 0, 5, 0);
                    legendLayout->setSpacing(10);
                    legendLayout->setAlignment(Qt::AlignVCenter);

                    QToolButton* barchartConfigButton = new QToolButton();
                    connect(plot, SIGNAL(visibilityChanged(bool)), barchartConfigButton, SLOT(setVisible(bool)));
                    connect(barchartConfigButton, &QToolButton::clicked, [=]() {
                        showEditBarchartProfileDialog(plotGroup, plot, streamInfo);
                    });

                    QToolButton* barchartPlotSwitch = new QToolButton();
                    barchartPlotSwitch->setIcon(QIcon(":/icon/bar_chart.png"));
                    barchartPlotSwitch->setCheckable(true);

                    connect(plot, SIGNAL(visibilityChanged(bool)), barchartPlotSwitch, SLOT(setVisible(bool)));

                    QVector<PlotSeriesData*> series;
                    series.reserve(streamInfo.PerGroup[plotGroup].Count);

                    for(size_t j = 0; j < streamInfo.PerGroup[plotGroup].Count; ++j)
                    {
                        size_t yIndex = streamInfo.PerGroup[plotGroup].Start + j;

                        auto seriesData = new PlotSeriesData(stats(plot->streamPos()), plot->getCurve(j)->title().text(), m_fileInfoData->BitsPerRawSample(),
                                                             m_dataTypeIndex, yIndex, plotGroup, j, streamInfo.PerGroup[plotGroup].Count);
                        series.append(seriesData);
                        plot->setData(j, seriesData);
                    }

                    connect(barchartPlotSwitch, &QToolButton::toggled, [=](bool toggled) {

                        bool switchToBarcharts = toggled;
                        if(switchToBarcharts) {
                            bool empty = true;
                            for(PlotSeriesData* seriesData : series) {
                                if(!seriesData->conditions().isEmpty()) {
                                    empty = false;
                                    break;
                                }
                            }

                            if(empty) {
                                switchToBarcharts = false;
                                showEditBarchartProfileDialog(plotGroup, plot, streamInfo);

                                for(PlotSeriesData* seriesData : series) {
                                    if(!seriesData->conditions().isEmpty()) {
                                        switchToBarcharts = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if(switchToBarcharts != toggled) {
                            barchartPlotSwitch->blockSignals(true);
                            barchartPlotSwitch->setChecked(switchToBarcharts);
                            barchartPlotSwitch->blockSignals(false);
                        }

                        for(auto& seriesData : series) {
                            seriesData->setBarchart(switchToBarcharts);
                        }

                        plot->setBarchart(switchToBarcharts);
                        barchartPlotSwitch->setIcon(switchToBarcharts ? QIcon(":/icon/chart_chart.png") : QIcon(":/icon/bar_chart.png"));
                    });

                    QHBoxLayout* barchartAndConfigurationLayout = new QHBoxLayout();
                    barchartAndConfigurationLayout->setAlignment(Qt::AlignLeft);
                    barchartAndConfigurationLayout->setSpacing(5);
                    barchartAndConfigurationLayout->addWidget(barchartPlotSwitch);
                    barchartAndConfigurationLayout->addWidget(barchartConfigButton);

                    legendLayout->addItem(barchartAndConfigurationLayout);
                    legendLayout->addWidget(plot->plotLegend());

                    QWidget* legendContainer = new QFrame();
                    plot->plotLegend()->setParent(legendContainer);
                    legendContainer->setContentsMargins(0, 0, 0, 0);
                    legendContainer->setLayout(legendLayout);

                    layout->addWidget(legendContainer, m_plotsCount, 1);

                    int height = barchartPlotSwitch->sizeHint().height();
                    barchartConfigButton->setIcon(QIcon(":/icon/settings.png"));
                    barchartConfigButton->setMaximumSize(QSize(height, height));

                    plot->setLegend(legendContainer);
                    plot->legend()->setObjectName(QString("Legend for %1").arg(plot->objectName()));

                    m_plots[streamPos][group] = plot;

                    m_plotsCount++;

                    qDebug() << "created plot with group: " << plot->group() << ", type: " << plot->type() << ", m_plotsCount: " << m_plotsCount;
                }
                else
                {
                    m_plots[streamPos][group] = NULL;
                }
            }
        }
        else
        {
            m_plots[streamPos]=NULL;
        }
    }

    m_commentsPlot = createCommentsPlot(fileInformation, &m_dataTypeIndex);
    m_commentsPlot->setObjectName(QString("Comments Plot of type %1, group %2").arg(Type_Comments).arg(0));

    m_commentsPlot->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
    m_commentsPlot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );

    connect( m_commentsPlot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );
    m_commentsPlot->canvas()->installEventFilter( this );

    if(m_commentsPlot)
    {
        layout->addWidget(m_commentsPlot, m_plotsCount, 0);

        QVBoxLayout* legendLayout = new QVBoxLayout();
        legendLayout->setContentsMargins(5, 0, 5, 0);
        legendLayout->setSpacing(10);
        legendLayout->setAlignment(Qt::AlignVCenter);
        legendLayout->addWidget(m_commentsPlot->plotLegend());

        QWidget* legendContainer = new QFrame(this);
        m_commentsPlot->plotLegend()->setParent(legendContainer);
        legendContainer->setContentsMargins(0, 0, 0, 0);
        legendContainer->setLayout(legendLayout);
        legendContainer->setMaximumHeight(m_commentsPlot->maximumHeight());
        legendContainer->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

        layout->addWidget(legendContainer, m_plotsCount, 1, Qt::AlignCenter);

        m_commentsPlot->setLegend(legendContainer);
        m_commentsPlot->legend()->setObjectName(QString("Legend for %1").arg(m_commentsPlot->objectName()));
    }

    setCommentsVisible(false);

    auto mappedTopLeft = m_commentsPlot->canvas()->mapToParent(m_commentsPlot->plotLayout()->canvasRect().topLeft().toPoint());
    auto mappedBottomRight = m_commentsPlot->canvas()->mapToParent(m_commentsPlot->plotLayout()->canvasRect().bottomRight().toPoint());

    // qDebug() << "mappedTopLeft: " << mappedTopLeft;
    // qDebug() << "mappedBottomRight: " << mappedBottomRight;

    auto panelsCount = 0;
    auto& panelOutputsByTitle = m_fileInfoData->panelOutputsByTitle();
    for(auto & item : panelOutputsByTitle.keys())
    {
        auto panelOutputIndexs = panelOutputsByTitle[item];
        for(auto panelOutputIndex : panelOutputIndexs)
        {
            auto metadata = m_fileInfoData->getPanelOutputMetadata(panelOutputIndex);
            auto yaxisIt = metadata.find("yaxis");
            auto yaxis = yaxisIt != metadata.end() ? yaxisIt->second : "";
            auto legendIt = metadata.find("legend");
            auto legend = legendIt != metadata.end() ? legendIt->second : "";
            auto panel_typeIt = metadata.find("panel_type");
            auto isAudioPanel = panel_typeIt != metadata.end() ? (panel_typeIt->second == "audio") : false;
            auto input_streamIt = metadata.find("input_stream");
            auto input_stream = input_streamIt != metadata.end() ? input_streamIt->second : "unknown";

            qDebug() << "panelTitle: " << QString::fromStdString(item);

            ++panelsCount;

            auto m_PanelsView = new PanelsView(this, QString::fromStdString(item), QString::fromStdString(yaxis), QString::fromStdString(legend), m_commentsPlot);
            m_PanelsView->setObjectName(QString("Panels Plot for stream %1 of type %2, group %3, title: %4").arg(QString::fromStdString(input_stream)).arg(Type_Panels).arg(qHash(m_PanelsView->panelTitle())).arg(m_PanelsView->panelTitle()));

            qDebug() << "added panelView: " << m_PanelsView->objectName();

            m_PanelsView->setFrameShape(QFrame::Panel);
            m_PanelsView->setLineWidth(2);
            m_PanelsView->setFont(m_commentsPlot->axisWidget(QwtPlot::yLeft)->font());

            auto leftMargin = m_commentsPlot->axisWidget(QwtPlot::yLeft)->margin();
            auto leftSpacing = m_commentsPlot->axisWidget(QwtPlot::yLeft)->spacing();

            mappedTopLeft.setX(mappedTopLeft.x() + leftMargin + leftSpacing);
            mappedBottomRight.setX(mappedBottomRight.x() - leftMargin - leftSpacing);

            auto right = m_PanelsView->width() - m_commentsPlot->width();

            m_PanelsView->setContentsMargins(mappedTopLeft.x(), 0,
                                             /*right*/
                                             mappedBottomRight.x(), 0);

            m_PanelsView->setMinimumHeight(100);

            layout->addWidget(m_PanelsView, m_plotsCount + m_PanelsViews.size() + 1, 0);

            QVBoxLayout* legendLayout = new QVBoxLayout();
            legendLayout->setContentsMargins(5, 0, 5, 0);
            legendLayout->setSpacing(10);
            legendLayout->setAlignment(Qt::AlignVCenter);
            legendLayout->addWidget(m_PanelsView->legend());

            QWidget* legendContainer = new QFrame(this);
            m_PanelsView->plotLegend()->setParent(legendContainer);
            legendContainer->setContentsMargins(0, 0, 0, 0);
            legendContainer->setLayout(legendLayout);

            layout->addWidget(legendContainer, m_plotsCount + m_PanelsViews.size() + 1, 1 );

            m_PanelsView->setLegend(legendContainer);
            m_PanelsView->legend()->setObjectName(QString("Legend for %1").arg(m_commentsPlot->objectName()));

            m_PanelsView->setVisible(false);
            m_PanelsView->legend()->setVisible(false);

            connect(m_PanelsView, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );
            connect(this, &Plots::visibleFramesChanged, m_PanelsView, &PanelsView::setVisibleFrames);

            if(isAudioPanel) {
                m_PanelsView->setProvider([&, panelOutputIndex] {
                    auto panelsCount = m_fileInfoData->getPanelFramesCount(panelOutputIndex);
                    return panelsCount;
                }, [&, panelOutputIndex](int index) -> QImage {
                    auto frame = m_fileInfoData->getPanelFrame(panelOutputIndex, index);
                    auto panelImage = QImage(*frame.frame()->data, frame.frame()->width, frame.frame()->height,
                                             *frame.frame()->linesize, QImage::Format_RGB888);

                    auto frameRate = m_fileInfoData->Glue->getAvgVideoFrameRate();
                    if(frameRate.isValid()) {
                        return panelImage.scaled(panelImage.width() * frameRate.value() / 32, panelImage.height());
                    }
                    else return panelImage;
                });

            } else {
                m_PanelsView->setProvider([&, panelOutputIndex] {
                    auto panelsCount = m_fileInfoData->getPanelFramesCount(panelOutputIndex);
                    return panelsCount;
                }, [&, panelOutputIndex](int index) -> QImage {
                    auto frame = m_fileInfoData->getPanelFrame(panelOutputIndex, index);
                    auto panelImage = QImage(*frame.frame()->data, frame.frame()->width, frame.frame()->height,
                                             *frame.frame()->linesize, QImage::Format_RGB888);

                    return panelImage;
                });
            }
            m_PanelsView->setVisibleFrames(0, numFrames() - 1);

            m_PanelsViews.push_back(m_PanelsView);
        }
    }

    layout->addWidget( m_scaleWidget, m_plotsCount + panelsCount + 1, 0, 1, 2 );

    // combo box for the axis format
    XAxisFormatBox* xAxisBox = new XAxisFormatBox();
    xAxisBox->setCurrentIndex( Plots::AxisTime );
    connect( xAxisBox, SIGNAL( currentIndexChanged( int ) ),
        this, SLOT( onXAxisFormatChanged( int ) ) );

    int axisBoxRow = layout->rowCount() - 1;
    // one row below to have space enough for bottom scale tick labels
    layout->addWidget( xAxisBox, m_plotsCount + panelsCount + 1, 1 );

    m_playerControl = new PlayerControl();
    layout->addWidget(m_playerControl, m_plotsCount + panelsCount + 2, 0);

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    setCursorPos( framePos() );
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
}

PlayerControl *Plots::playerControl()
{
    return m_playerControl;
}

//---------------------------------------------------------------------------
const QwtPlot* Plots::plot( size_t streamPos, size_t Group ) const
{
    return m_plots[streamPos]?m_plots[streamPos][Group]:NULL;
}



//---------------------------------------------------------------------------
void Plots::refresh()
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
            for ( int i = 0; i < PerStreamType[type].CountOfGroups; i++ )
                if (m_plots[streamPos][i] && m_plots[streamPos][i]->isVisible())
            {
                m_plots[streamPos][i]->initYAxis();
                updateSamples( m_plots[streamPos][i] );
            }
        }

    for(auto m_PanelsView : m_PanelsViews)
        m_PanelsView->refresh();

    setCursorPos( framePos() );
    replotAll();

}

//---------------------------------------------------------------------------
void Plots::setVisibleFrames( int from, int to , bool force)
{
    if ( from != m_frameInterval.from || to != m_frameInterval.to || force)
    {
        m_frameInterval.from = from;
        m_frameInterval.to = to;

        const double* x = stats()->x[m_dataTypeIndex];
        m_timeInterval.from = x[from];
        m_timeInterval.to = x[to];

        // Handling unfinished parsing with estimated x
        if ( m_timeInterval.from == 0  && from)
            m_timeInterval.from = stats()->x_Max[m_dataTypeIndex] / stats()->x_Max[0] * from;
        if ( m_timeInterval.to == 0 )
            m_timeInterval.to = stats()->x_Max[m_dataTypeIndex] / stats()->x_Max[0] * to;
    }

    Q_EMIT visibleFramesChanged(from, to);
}

//---------------------------------------------------------------------------
FrameInterval Plots::visibleFrames() const
{
    return m_frameInterval;
}

//---------------------------------------------------------------------------
void Plots::onCurrentFrameChanged()
{
    // position of the current frame has changed 

    if ( isZoomed() )
    {
        const int n = m_frameInterval.count();

        const int from = qBound( 0, framePos() - n / 2, numFrames() - n );
        const int to = from + n - 1;

        if ( from != m_frameInterval.from )
        {
            setVisibleFrames( from, to );
            replotAll();
        }
    }

    setCursorPos( framePos() );
}

//---------------------------------------------------------------------------
void Plots::setCursorPos( int newFramePos )
{
    setFramePos( newFramePos );

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            const double x = m_fileInfoData->Stats[streamPos]->x[m_dataTypeIndex][framePos(streamPos)];

            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( int i = 0; i < PerStreamType[type].CountOfGroups; ++i )
                if (m_plots[streamPos][i])
                m_plots[streamPos][i]->setCursorPos( x );

            if(type == Type_Video) {
                m_commentsPlot->setCursorPos(x);

                for(auto& panel : m_PanelsViews) {
                    panel->setCursorPos(x);
                }
            }
        }

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);
    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::updateSamples( Plot* plot )
{
    const size_t plotType = plot->type();
    const size_t plotGroup = plot->group();
    const CommonStats* stat = stats( plot->streamPos() );

    auto streamInfo = PerStreamType[plotType];

    for(auto j = 0; j < streamInfo.PerGroup[plotGroup].Count; ++j)
    {
        plot->replot();
    }
}

//---------------------------------------------------------------------------
void Plots::Zoom_Move( int Begin )
{
    const int n = m_frameInterval.count();

    const int from = qMax( Begin, 0 );
    const int to = qMin( numFrames(), from + n ) - 1;

    setVisibleFrames( to - n + 1, to );

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
		    size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group])
                m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
        }

    if(m_commentsPlot)
        m_commentsPlot->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    refresh();

    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::alignYAxes()
{
    double maxExtent = 0;

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group] && m_plots[streamPos] )
            {
                QwtScaleWidget *scaleWidget = m_plots[streamPos][group]->axisWidget( QwtPlot::yLeft );

                QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
                scaleDraw->setMinimumExtent( 0.0 );

                if ( m_plots[streamPos][group] && m_plots[streamPos][group]->isVisibleTo( this ) )
                {
                    const double extent = scaleDraw->extent( scaleWidget->font() );
                    maxExtent = qMax( extent, maxExtent );
                }
            }
            maxExtent += 3; // margin
        }

    if(m_commentsPlot) {
        QwtScaleWidget *scaleWidget = m_commentsPlot->axisWidget(QwtPlot::yLeft);
        QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();

        const double extent = scaleDraw->extent( scaleWidget->font() );
        maxExtent = qMax( extent, maxExtent );
    }

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group])
            {
                QwtScaleWidget *scaleWidget = m_plots[streamPos][group]->axisWidget( QwtPlot::yLeft );
                scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
            }
        }

    if(m_commentsPlot)
        m_commentsPlot->axisWidget(QwtPlot::yLeft)->scaleDraw()->setMinimumExtent(maxExtent);
}

void showEditFrameCommentsDialog(QWidget* parentWidget, FileInformation* info, CommonStats* stats, size_t frameIndex)
{
    CommentsEditor dialog;
    dialog.setWindowTitle(QString("Edit comment"));
    dialog.setLabelText(QString("Comment for frame %1:").arg(frameIndex));

    QString textValue;
    if(stats->comments[frameIndex])
    {
        QTextDocument doc;
        textValue = QString::fromUtf8(stats->comments[frameIndex]);
        doc.setHtml(textValue.replace("\n", "<br>"));
        textValue = doc.toPlainText();
    }

    if(stats->comments[frameIndex])
    {
        dialog.buttons()->button(QDialogButtonBox::Discard)->setVisible(true);
    }

    dialog.setTextValue(textValue);
    int result = dialog.exec();

    if(result == QDialog::Rejected)
        return;

    static QString replacePattern = "<br ***>";
    static QString htmlEscapedPattern = replacePattern.toHtmlEscaped();

    textValue = dialog.textValue().replace("\n", replacePattern);
    textValue = textValue.toHtmlEscaped();
    textValue = textValue.replace(htmlEscapedPattern, "\n");

    if(result == QDialogButtonBox::DestructiveRole || textValue.isEmpty())
    {
        if(stats->comments[frameIndex] != nullptr)
        {
            delete [] stats->comments[frameIndex];
            stats->comments[frameIndex] = nullptr;
            info->setCommentsUpdated(stats);
        }
    } else // result == QDialog::Accepted
    {
        if(!stats->comments[frameIndex] || strcmp(stats->comments[frameIndex], textValue.toUtf8().constData()) != 0)
        {
            delete [] stats->comments[frameIndex];
            stats->comments[frameIndex] = strdup(textValue.toUtf8().constData());
            info->setCommentsUpdated(stats);
        }
    }
}

bool Plots::eventFilter( QObject *object, QEvent *event )
{
    if ( event->type() == QEvent::Move || event->type() == QEvent::Resize )
    {
        for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
            if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
            {
                size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();
        
                for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                {
                    if ( m_plots[streamPos][group] && m_plots[streamPos][group]->isVisibleTo( this ) )
                    {
                        if ( object == m_plots[streamPos][group]->canvas() ) {
                            alignXAxis( m_plots[streamPos][group] );
                        }

                        break;
                    }
                }
            }

        if(object == m_commentsPlot->canvas())
            alignXAxis(m_commentsPlot);

        alignYAxes();

        QTimer::singleShot(0, [&]() {
            auto canvasRect = m_commentsPlot->plotLayout()->canvasRect();
            auto mappedTopLeft = m_commentsPlot->canvas()->mapToParent(QPoint(0, 0));
            auto mappedBottomRight = m_commentsPlot->canvas()->mapToParent(QPoint(canvasRect.width(), canvasRect.height()));

            auto leftMargin = m_commentsPlot->axisWidget(QwtPlot::yLeft)->margin();
            auto leftSpacing = m_commentsPlot->axisWidget(QwtPlot::yLeft)->spacing();

            for(auto m_PanelsView : m_PanelsViews) {
                m_PanelsView->setLeftOffset(leftMargin + leftSpacing);
                m_PanelsView->setContentsMargins(mappedTopLeft.x(), 0, m_PanelsView->width() - mappedBottomRight.x(), 0);
                m_PanelsView->setActualWidth(m_commentsPlot->canvas()->contentsRect().width());
            }
        });
    }
    else if(event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::LeftButton)
        {
            showEditFrameCommentsDialog(parentWidget(), m_fileInfoData, m_fileInfoData->ReferenceStat(), framePos());
        }
    } else if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_M)
        {
            showEditFrameCommentsDialog(parentWidget(), m_fileInfoData, m_fileInfoData->ReferenceStat(), framePos());
        }
    }

    return QWidget::eventFilter( object, event );
}

void swapWidgets(QGridLayout* yourGridLayout, QWidget *widgetA, QWidget *widgetB)
{
    int indexA = yourGridLayout->indexOf(widgetA);

    int row1, column1, rowSpan1, columnSpan1;
    yourGridLayout->getItemPosition(indexA, &row1, &column1, &rowSpan1, &columnSpan1);

    auto oldA = yourGridLayout->takeAt(indexA);

    int indexB = yourGridLayout->indexOf(widgetB);
    int row2, column2, rowSpan2, columnSpan2;

    yourGridLayout->getItemPosition(indexB, &row2, &column2, &rowSpan2, &columnSpan2);
    auto oldB = yourGridLayout->takeAt(indexB);

    yourGridLayout->addWidget(widgetB, row1, column1, rowSpan1, columnSpan1);
    yourGridLayout->addWidget(widgetA, row2, column2, rowSpan2, columnSpan2);

/*
    if(row1 > row2) {
        yourGridLayout->addWidget(widgetB, row1, column1, rowSpan1, columnSpan1);
        yourGridLayout->addWidget(widgetA, row2, column2, rowSpan2, columnSpan2);
    } else {
        yourGridLayout->addWidget(widgetA, row2, column2, rowSpan2, columnSpan2);
        yourGridLayout->addWidget(widgetB, row1, column1, rowSpan1, columnSpan1);
    }
    */

    delete oldA;
    delete oldB;
}

void Plots::changeOrder(QList<std::tuple<quint64, quint64>> newOrder)
{
    if(newOrder.empty())
    {
        qDebug() << "orderedFilterInfo is empty, do not reorder..";
        return;
    }

    qDebug() << "changeOrder: items = " << newOrder.count();

    auto gridLayout = static_cast<QGridLayout*> (layout());
    auto rowsCount = gridLayout->rowCount();

    Q_ASSERT(m_plotsCount <= rowsCount);

    qDebug() << "plotsCount: " << m_plotsCount << "commentsCount: " << 1 << "panelsCount: " << m_PanelsViews.size();

    QList <std::tuple<quint64, quint64, quint64>> currentOrderedPlotsInfo;
    QList <std::tuple<quint64, quint64, quint64>> newOrderedPlotsInfo;

    for(auto row = 0; row < (m_plotsCount + 1 + m_PanelsViews.size()); ++row)
    {
        auto plotItem = gridLayout->itemAtPosition(row, 0);
        // qDebug() << "plotItem: " << plotItem;

        auto legendItem = gridLayout->itemAtPosition(row, 1);

        Q_ASSERT(plotItem);
        Q_ASSERT(legendItem);

        auto plot = qobject_cast<Plot*> (plotItem->widget());
        if(plot)
            currentOrderedPlotsInfo.push_back(std::make_tuple(plot->group(), plot->type(), plot->streamPos()));

        auto commentsPlot = qobject_cast<CommentsPlot*> (plotItem->widget());
        if(commentsPlot)
            currentOrderedPlotsInfo.push_back(std::make_tuple(0, Type_Comments, 0));

        auto panelsView = qobject_cast<PanelsView*> (plotItem->widget());
        if(panelsView)
            currentOrderedPlotsInfo.push_back(std::make_tuple(qHash(panelsView->panelTitle()), Type_Panels, 0));
    }

    // currentOrderedPlotsInfo.push_back(std::make_tuple(0, Type_Max, 0));

    qDebug() << "\tnewOrder: " << newOrder.length();
    for(auto filterInfo : newOrder)
    {
        qDebug() << "\t\t" << QString("group: %1/type: %2")
                    .arg(std::get<0>(filterInfo))
                    .arg(std::get<1>(filterInfo));
    }

    qDebug() << "\tcurrentOrderedPlotsInfo: " << currentOrderedPlotsInfo.length();
    for(auto filterInfo : currentOrderedPlotsInfo)
    {
        qDebug() << "\t\t" << QString("group: %1/type: %2")
                    .arg(std::get<0>(filterInfo))
                    .arg(std::get<1>(filterInfo));
    }

    for(auto filterInfo : newOrder)
    {
        for(auto plotInfo : currentOrderedPlotsInfo)
        {
            auto plotGroup = std::get<0>(plotInfo);
            auto filterGroup = std::get<0>(filterInfo);

            auto plotType = std::get<1>(plotInfo);
            auto filterType = std::get<1>(filterInfo);

            if(plotGroup == filterGroup &&  plotType == filterType)
            {
                newOrderedPlotsInfo.push_back(plotInfo);
            }
        }
    }

    // Q_ASSERT(currentOrderedPlotsInfo.length() == newOrderedPlotsInfo.length());
    if(currentOrderedPlotsInfo.length() != newOrderedPlotsInfo.length())
    {
        auto currentSet = QSet<QString>();
        auto newSet = QSet<QString>();

        qDebug() << "\tcurrent: " << currentOrderedPlotsInfo.size();

        for(auto i = 0; i < currentOrderedPlotsInfo.length(); ++i) {
            qDebug() << "\t\t" << QString("group: %1/type: %2")
                        .arg(std::get<0>(currentOrderedPlotsInfo[i]))
                        .arg(std::get<1>(currentOrderedPlotsInfo[i]));

            currentSet.insert(
                        QString("group: %1/type: %2")
                        .arg(std::get<0>(currentOrderedPlotsInfo[i]))
                        .arg(std::get<1>(currentOrderedPlotsInfo[i]))
                        );
        }

        qDebug() << "\tnew: " << newOrderedPlotsInfo.size();

        for(auto i = 0; i < newOrderedPlotsInfo.length(); ++i) {
            qDebug() << "\t\t" << QString("group: %1/type: %2")
                        .arg(std::get<0>(newOrderedPlotsInfo[i]))
                        .arg(std::get<1>(newOrderedPlotsInfo[i]));

            newSet.insert(
                        QString("group: %1/type: %2")
                        .arg(std::get<0>(newOrderedPlotsInfo[i]))
                        .arg(std::get<1>(newOrderedPlotsInfo[i]))
                        );
        }

        auto newMinusCurrent = QSet<QString>(newSet).subtract(currentSet);
        auto currentMinusNew= QSet<QString>(currentSet).subtract(newSet);

        qDebug() << "newMinusCurrent: " << newMinusCurrent;
        qDebug() << "currentMinusNew: " << currentMinusNew;

        if(currentOrderedPlotsInfo.length() > newOrderedPlotsInfo.length())
        {
            for(auto i = 0; i < currentOrderedPlotsInfo.size(); ++i)
            {
                bool existsInNew = false;
                for(auto j = 0; j < newOrderedPlotsInfo.length(); ++j) {
                    if(newOrderedPlotsInfo[j] == currentOrderedPlotsInfo[i]) {
                        existsInNew = true;
                        break;
                    }
                }
                if(!existsInNew) {
                    newOrderedPlotsInfo.append(currentOrderedPlotsInfo[i]);
                }
            }
        }
        else
        {
            return;
        }
    }

    assert(newOrderedPlotsInfo.length() == currentOrderedPlotsInfo.length());

    qDebug() << "\teffective new order: " << newOrderedPlotsInfo.size();

    for(auto i = 0; i < newOrderedPlotsInfo.length(); ++i) {
        qDebug() << "\t\t" << QString("group: %1/type: %2")
                    .arg(std::get<0>(newOrderedPlotsInfo[i]))
                    .arg(std::get<1>(newOrderedPlotsInfo[i]));
    }

    auto offset = 0;
    for(auto i = 0; i < newOrderedPlotsInfo.length(); ++i)
    {
        auto newOrderedPlotInto = newOrderedPlotsInfo[i];
        auto currentOrderedPlotInfo = currentOrderedPlotsInfo[i];

        qDebug() << "newOrderedPlotInfo: " << std::get<0>(newOrderedPlotInto) << "/" << std::get<1>(newOrderedPlotInto);

        if(newOrderedPlotInto != currentOrderedPlotInfo)
        {
            // if previously ordered plot is the same as new one (multi-track case)
            // then start searching for related plot right after position where we found previous one
            if(i > 0 && newOrderedPlotsInfo[i - 1] == newOrderedPlotInto)
            {
                ++offset;
            } else {
                offset = 0;
            }

            // search current item which we should put at new position
            for(auto j = offset; j < newOrderedPlotsInfo.length(); ++j)
            {
                currentOrderedPlotInfo = currentOrderedPlotsInfo[j];

                if(newOrderedPlotInto == currentOrderedPlotInfo)
                {
                    qDebug() << "i: " << i << ", j: " << j;

                    auto plotWidget = gridLayout->itemAtPosition(j, 0)->widget();
                    qDebug() << "plot widget: " << plotWidget->objectName();

                    auto legendWidget = gridLayout->itemAtPosition(j, 1)->widget();
                    qDebug() << "legend widget: " << legendWidget->objectName();

                    auto swapPlotWidget = gridLayout->itemAtPosition(i, 0)->widget();
                    qDebug() << "swap plot widget: " << swapPlotWidget->objectName();

                    auto swapLegendWidget = gridLayout->itemAtPosition(i, 1)->widget();
                    qDebug() << "swap legend widget: " << swapLegendWidget->objectName();

                    swapWidgets(gridLayout, plotWidget, swapPlotWidget);
                    swapWidgets(gridLayout, legendWidget, swapLegendWidget);

                    currentOrderedPlotsInfo[j] = currentOrderedPlotsInfo[i];
                    currentOrderedPlotsInfo[i] = newOrderedPlotsInfo[i];

                    offset = j;
                    break;
                }
            }
        } else {
            qDebug() << "newOrderedPlotInto == currentOrderedPlotInfo";
        }
    }

    Q_ASSERT(rowsCount == gridLayout->rowCount());
}

QJsonObject Plots::saveBarchartsProfile()
{
    QJsonObject conditionsObject;
    QJsonArray conditionsArray;

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            auto type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( size_t group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group]) {
                    auto plot = m_plots[streamPos][group];
                    QJsonObject plotObject;

                    plotObject.insert("streamPos", (int) plot->streamPos());
                    plotObject.insert("plotType", (int) plot->type());
                    plotObject.insert("plotGroup", (int) plot->group());
                    plotObject.insert("plotTitle", PerStreamType[plot->type()].PerGroup[plot->group()].Name);

                    QJsonArray plotFormulas;
                    auto streamInfo = PerStreamType[plot->type()];
                    for(size_t j = 0; j < streamInfo.PerGroup[plot->group()].Count; ++j)
                    {
                        auto curveData = static_cast<const PlotSeriesData*>(plot->getData(j));
                        plotFormulas.append(curveData->conditions().toJson());
                    }

                    plotObject.insert("plotFormulas", plotFormulas);
                    conditionsArray.append(plotObject);
                }
        }
    }

    conditionsObject.insert("profileFormulas", conditionsArray);
    return conditionsObject;
}

void Plots::loadBarchartsProfile(const QJsonObject& profile)
{
    QJsonArray conditions = profile.value("profileFormulas").toArray();

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] ) {
            auto type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( size_t group = 0; group < PerStreamType[type].CountOfGroups; group++ ) {
                if (m_plots[streamPos][group]) {
                    auto plot = m_plots[streamPos][group];

                    auto streamInfo = PerStreamType[plot->type()];
                    for(size_t j = 0; j < streamInfo.PerGroup[plot->group()].Count; ++j)
                    {
                        auto curveData = plot->getData(j);
                        curveData->mutableConditions().clear();
                    }

                    if(conditions.empty() && plot->isBarchart())
                        plot->replot();
                }
            }
        }
    }

    for(auto condition : conditions) {
        auto conditionObject = condition.toObject();
        auto plotGroup = conditionObject.value("plotGroup").toInt();
        auto plotType = conditionObject.value("plotType").toInt();
        auto streamPos = conditionObject.value("streamPos").toInt();
        auto plotTitle = conditionObject.value("plotTitle").toString();

        auto plotFormulas = conditionObject.value("plotFormulas").toArray();

        if(streamPos < m_fileInfoData->Stats.size() && m_fileInfoData->Stats[streamPos] && m_plots[streamPos]) {
            if(plotType == m_fileInfoData->Stats[streamPos]->Type_Get() && plotGroup < PerStreamType[plotType].CountOfGroups) {
                if (m_plots[streamPos][plotGroup]) {
                    auto plot = m_plots[streamPos][plotGroup];

                    for(auto plotCondition : plotFormulas) {
                        auto plotConditionObject = plotCondition.toObject();

                        auto curveIndex = plotConditionObject.value("chartIndex").toInt();
                        if(curveIndex < plot->curvesCount()) {
                            auto curveData = plot->getData(curveIndex);
                            auto formulas = plotConditionObject.value("formulas").toArray();

                            for(auto formula : formulas) {
                                auto formulaObject = formula.toObject();
                                auto value = formulaObject.value("value").toString();
                                auto color = QColor(formulaObject.value("color").toString());
                                auto label = formulaObject.value("label").toString();
                                auto eliminateSpikes = formulaObject.value("eliminateSpikes").toBool();

                                curveData->mutableConditions().add(value, color, label, eliminateSpikes);
                            }
                        }
                    }

                    if(plot->isBarchart())
                        plot->replot();
                }
            }
        }
    }
}

void Plots::alignXAxis( const QwtPlot* plot )
{
    const QWidget* canvas = plot->canvas();

    QRect r = canvas->geometry(); 
    r.moveTopLeft( mapFromGlobal( plot->mapToGlobal( r.topLeft() ) ) );

    int left = r.left();
    left += plot->plotLayout()->canvasMargin( QwtPlot::yLeft );

    int right = width() - ( r.right() - 1 );
    right += plot->plotLayout()->canvasMargin( QwtPlot::yRight );

    m_scaleWidget->setBorderDist( left, right );
}

//---------------------------------------------------------------------------
void Plots::onCursorMoved( int framePos )
{
    m_fileInfoData->Frames_Pos_Set( framePos );
    setCursorPos( framePos );
}

//---------------------------------------------------------------------------
void Plots::onXAxisFormatChanged( int format )
{
    if ( format == m_scaleWidget->format() )
        return;

    int dataTypeIndex = format;
    if ( format == AxisTime )
        dataTypeIndex = AxisSeconds;

    if ( m_dataTypeIndex != dataTypeIndex )
    {
        m_dataTypeIndex = dataTypeIndex;

        setVisibleFrames( m_frameInterval.from, m_frameInterval.to, true );

        for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
            if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos])
            {
		        size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

                for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                    if (m_plots[streamPos][group])
                    m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
            }

        if(m_commentsPlot)
            m_commentsPlot->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );

        m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

        refresh();
    }

    m_scaleWidget->setFormat( format );
    m_scaleWidget->update();

    replotAll();
}

//---------------------------------------------------------------------------
void Plots::setPlotVisible( size_t type, size_t group, bool on )
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            if ( type == m_fileInfoData->Stats[streamPos]->Type_Get() && m_plots[streamPos] && m_plots[streamPos][group] && on != m_plots[streamPos][group]->isVisibleTo( this ))
            {
                m_plots[streamPos][group]->setVisible( on );
                m_plots[streamPos][group]->legend()->setVisible( on );
            }
        }
}

void Plots::setCommentsVisible(bool visible)
{
    if(visible) {
        m_commentsPlot->restorePlotHeight();
    } else {
        m_commentsPlot->setMinimumHeight(0);
        m_commentsPlot->setMaximumHeight(0);
    }
    m_commentsPlot->legend()->setVisible(visible);
}

void Plots::updatePlotsVisibility(const QMap<QString, std::tuple<quint64, quint64> > &visiblePlots)
{
    qDebug() << "updatePlotsVisibility";
    for(auto plotName : visiblePlots.keys()) {
        qDebug() << "\tplotName: " << plotName;
    }

    class GroupAndTypeTupleHashFunction {
    public:
        size_t operator()(const std::tuple<quint64, quint64>& p) const
        {
            auto group = std::get<0>(p);
            auto type = std::get<1>(p);

            return (std::hash<unsigned long long>()(group)) ^ (std::hash<unsigned long long>()(type));
        }
    };

    std::unordered_set<std::tuple<quint64, quint64>, GroupAndTypeTupleHashFunction> groupAndTypeSet;
    for(auto groupAndType : visiblePlots.values())
    {
        groupAndTypeSet.insert(groupAndType);
    }

    for (quint64 type = 0; type<Type_Max; type++) {
        for (quint64 group=0; group<PerStreamType[type].CountOfGroups; group++) {
            auto tuple = std::tuple<quint64, quint64>(group, type);
            bool visible = groupAndTypeSet.find(tuple) != groupAndTypeSet.end();
            if(groupAndTypeSet.empty()) {
                visible = PerStreamType[type].PerGroup[group].CheckedByDefault;
            }
            setPlotVisible(type, group, visible);
        }
    }

    auto tuple = std::tuple<quint64, quint64>(0, Type_Comments);
    bool visible = groupAndTypeSet.find(tuple) != groupAndTypeSet.end();
    if(groupAndTypeSet.empty()) {
        visible = true;
    }

    setCommentsVisible(visible);

    for(size_t panelIndex = 0; panelIndex < panelsCount(); ++panelIndex) {
        auto panel = panelsView(panelIndex);
        auto panelTitle = panel->panelTitle();
        qDebug() << "\tpanelTitle: " << panelTitle;
        auto panelVisible = visiblePlots.contains(panelTitle);
        qDebug() << "\tpanelVisible: " << panelVisible;

        panel->setVisible(panelVisible);
        panel->legend()->setVisible(panelVisible);
    }
}

//---------------------------------------------------------------------------
void Plots::zoomXAxis( ZoomTypes zoomType )
{
    m_zoomType = zoomType;

    if ( zoomType == ZoomIn )
        m_zoomFactor++;
    else if ( zoomType == ZoomOut && m_zoomFactor )
        m_zoomFactor--;
    else if ( zoomType == ZoomOneToOne)
        m_zoomFactor = 0;

    qDebug() << "m_zoomFactor: " << m_zoomFactor;
    int numVisibleFrames = m_fileInfoData->Frames_Count_Get() >> m_zoomFactor;

    if(m_zoomType == ZoomOneToOne)
    {
        numVisibleFrames = m_commentsPlot->canvas()->contentsRect().width();
        m_zoomFactor = log(double(m_fileInfoData->Frames_Count_Get()) / numVisibleFrames) / log(2);
    }

    int to = qMin( framePos() + numVisibleFrames / 2, numFrames() );
    int from = qMax( 0, to - numVisibleFrames );
    if ( to - from < numVisibleFrames)
        to = from + numVisibleFrames;

    setVisibleFrames( from, to );

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
    {
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
            auto type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( size_t group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group]) {
                    m_plots[streamPos][group]->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
                    m_plots[streamPos][group]->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );
                    m_plots[streamPos][group]->updateSymbols();
                }
        }
    }

    if(m_commentsPlot)
        m_commentsPlot->setAxisScale( QwtPlot::xBottom, m_timeInterval.from, m_timeInterval.to );

    m_scaleWidget->update();

    replotAll();
}

bool Plots::isZoomed() const
{
    return m_frameInterval.count() < numFrames();
}

//---------------------------------------------------------------------------
void Plots::replotAll()
{
    for ( size_t streamPos = 0; streamPos < m_fileInfoData->Stats.size(); streamPos++ )
        if ( m_fileInfoData->Stats[streamPos] && m_plots[streamPos] )
        {
		    size_t type = m_fileInfoData->Stats[streamPos]->Type_Get();

            for ( int group = 0; group < PerStreamType[type].CountOfGroups; group++ )
                if (m_plots[streamPos][group])
            {
                m_plots[streamPos][group]->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
                if ( m_plots[streamPos][group]->isVisibleTo( this ) )
                    m_plots[streamPos][group]->replot();
            }
        }

    if(m_commentsPlot)
    {
        m_commentsPlot->setAxisScaleDiv( QwtPlot::xBottom, m_scaleWidget->scaleDiv() );
        m_commentsPlot->replot();
    }

    for(auto& panel : m_PanelsViews)
    {
        panel->refresh();
    }
}

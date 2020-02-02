/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

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
#include "playercontrol.h"
#include <QComboBox>
#include <QGridLayout>
#include <QEvent>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <cmath>
#include <clocale>
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

                    connect( plot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );

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
                    legendLayout->addWidget(plot->legend());
                    layout->addLayout(legendLayout, m_plotsCount, 1);

                    int height = barchartPlotSwitch->sizeHint().height();
                    barchartConfigButton->setIcon(QIcon(":/icon/settings.png"));
                    barchartConfigButton->setMaximumSize(QSize(height, height));

                    m_plots[streamPos][group] = plot;

                    m_plotsCount++;

                    qDebug() << "g: " << plot->group() << ", t: " << plot->type() << ", m_plotsCount: " << m_plotsCount;
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
        legendLayout->addWidget(m_commentsPlot->legend());

        layout->addLayout(legendLayout, m_plotsCount, 1 );
    }

    layout->addWidget( m_scaleWidget, m_plotsCount + 1, 0, 1, 2 );

    // combo box for the axis format
    XAxisFormatBox* xAxisBox = new XAxisFormatBox();
    xAxisBox->setCurrentIndex( Plots::AxisTime );
    connect( xAxisBox, SIGNAL( currentIndexChanged( int ) ),
        this, SLOT( onXAxisFormatChanged( int ) ) );

    int axisBoxRow = layout->rowCount() - 1;
#if 1
    // one row below to have space enough for bottom scale tick labels
    layout->addWidget( xAxisBox, m_plotsCount + 1, 1 );
#else
    layout->addWidget( xAxisBox, layout_y, 1 );
#endif

    m_playerControl = new PlayerControl();
    layout->addWidget(m_playerControl, m_plotsCount + 2, 0);

    layout->setColumnStretch( 0, 10 );
    layout->setColumnStretch( 1, 0 );

    m_scaleWidget->setScale( m_timeInterval.from, m_timeInterval.to);

    setCursorPos( framePos() );
}

//---------------------------------------------------------------------------
Plots::~Plots()
{
}

const PlayerControl *Plots::playerControl() const
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

            if(type == Type_Video)
                m_commentsPlot->setCursorPos(x);
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
                        if ( object == m_plots[streamPos][group]->canvas() )
                            alignXAxis( m_plots[streamPos][group] );

                        break;
                    }
                }
            }

        if(m_commentsPlot && object == m_commentsPlot->canvas())
            alignXAxis(m_commentsPlot);
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

void Plots::changeOrder(QList<std::tuple<int, int> > orderedFilterInfo)
{
    if(orderedFilterInfo.empty())
    {
        qDebug() << "orderedFilterInfo is empty, do not reorder..";
        return;
    }

    qDebug() << "changeOrder: items = " << orderedFilterInfo.count();

    auto gridLayout = static_cast<QGridLayout*> (layout());
    auto rowsCount = gridLayout->rowCount();

    Q_ASSERT(m_plotsCount <= rowsCount);

    qDebug() << "plotsCount: " << m_plotsCount;

    QList <std::tuple<size_t, size_t, size_t>> currentOrderedPlotsInfo;
    QList <std::tuple<size_t, size_t, size_t>> expectedOrderedPlotsInfo;

    for(auto row = 0; row < m_plotsCount; ++row)
    {
        auto plotItem = gridLayout->itemAtPosition(row, 0);
        auto legendItem = gridLayout->itemAtPosition(row, 1);

        Q_ASSERT(plotItem);
        Q_ASSERT(legendItem);

        auto plot = qobject_cast<Plot*> (plotItem->widget());
        if(plot)
            currentOrderedPlotsInfo.push_back(std::make_tuple(plot->group(), plot->type(), plot->streamPos()));

        auto commentsPlot = qobject_cast<CommentsPlot*> (plotItem->widget());
        if(commentsPlot)
            currentOrderedPlotsInfo.push_back(std::make_tuple(0, Type_Max, 0));
    }

    currentOrderedPlotsInfo.push_back(std::make_tuple(0, Type_Max, 0));

    for(auto filterInfo : orderedFilterInfo)
    {
        for(auto plotInfo : currentOrderedPlotsInfo)
        {
            if(std::get<0>(plotInfo) == std::get<0>(filterInfo) && std::get<1>(plotInfo) == std::get<1>(filterInfo))
            {
                expectedOrderedPlotsInfo.push_back(plotInfo);
            }
        }
    }

    Q_ASSERT(currentOrderedPlotsInfo.length() == expectedOrderedPlotsInfo.length());
    if(currentOrderedPlotsInfo.length() != expectedOrderedPlotsInfo.length())
        return;

    for(auto i = 0; i < expectedOrderedPlotsInfo.length(); ++i)
    {
        qDebug() << "cg: " << std::get<0>(currentOrderedPlotsInfo[i])
                 << ", "
                 << "ct: " << std::get<1>(currentOrderedPlotsInfo[i])
                 << ", "
                 << "cp: " << std::get<2>(currentOrderedPlotsInfo[i])
                 << ", "
                 << "eg: " << std::get<0>(expectedOrderedPlotsInfo[i])
                 << ", "
                 << "et: " << std::get<1>(expectedOrderedPlotsInfo[i])
                 << ", "
                 << "ep: " << std::get<2>(expectedOrderedPlotsInfo[i]);
    }

    for(auto i = 0; i < expectedOrderedPlotsInfo.length(); ++i)
    {
        if(expectedOrderedPlotsInfo[i] != currentOrderedPlotsInfo[i])
        {
            // search current item which we should put at expected position
            for(auto j = 0; j < expectedOrderedPlotsInfo.length(); ++j)
            {
                if(expectedOrderedPlotsInfo[i] == currentOrderedPlotsInfo[j])
                {
                    qDebug() << "i: " << i << ", j: " << j;

                    auto plotWidget = gridLayout->itemAtPosition(j, 0)->widget();
                    auto legendItem = gridLayout->itemAtPosition(j, 1);

                    auto swapPlotWidget = gridLayout->itemAtPosition(i, 0)->widget();
                    auto swapLegendItem = gridLayout->itemAtPosition(i, 1);

                    gridLayout->removeWidget(plotWidget);
                    gridLayout->removeItem(legendItem);

                    gridLayout->removeWidget(swapPlotWidget);
                    gridLayout->removeItem(swapLegendItem);

                    gridLayout->addWidget(plotWidget, i, 0);
                    gridLayout->addItem(legendItem, i, 1);

                    gridLayout->addWidget(swapPlotWidget, j, 0);
                    gridLayout->addItem(swapLegendItem, j, 1);

                    currentOrderedPlotsInfo[j] = currentOrderedPlotsInfo[i];
                    currentOrderedPlotsInfo[i] = expectedOrderedPlotsInfo[i];

                    break;
                }
            }
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
        numVisibleFrames = plot(0, 0)->canvas()->contentsRect().width();
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
}

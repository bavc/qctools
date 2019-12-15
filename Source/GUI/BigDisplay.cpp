/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "BigDisplay.h"
#include "SelectionArea.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"
#include "GUI/Help.h"
#include "GUI/imagelabel.h"
#include "GUI/config.h"
#include "GUI/Comments.h"
#include "GUI/Plots.h"
#include "Core/FileInformation.h"
#include "Core/FFmpeg_Glue.h"

#include <QDesktopWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QComboBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QCheckBox>
#include <QLabel>
#include <QToolButton>
#include <QActionGroup>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QPainter>
#include <QSpacerItem>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QColorDialog>
#include <QShortcut>
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QSplitter>
#include <qwt_scale_widget.h>
#include "playerwindow.h"
#include "filters.h"

#include <sstream>
//---------------------------------------------------------------------------


//***************************************************************************
// Config
//***************************************************************************

//---------------------------------------------------------------------------
// Default filters (check Filters order)
const size_t Filters_Default1 = 2; // 2 = Normal
const size_t Filters_Default2 = 5; // 5 = Waveform

//***************************************************************************
// Info
//***************************************************************************



//---------------------------------------------------------------------------

//***************************************************************************
// Helper
//***************************************************************************

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BigDisplay::BigDisplay(QWidget *parent, FileInformation* FileInformationData_) :
    QDialog(parent),
    FileInfoData(FileInformationData_)
{
    m_filterSelectors[0] = new FilterSelector(FileInfoData, this);
    m_filterSelectors[1] = new FilterSelector(FileInfoData, this);

    setlocale(LC_NUMERIC, "C");
    setWindowTitle("QCTools - "+FileInfoData->fileName());
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() &(0xFFFFFFFF-Qt::WindowContextHelpButtonHint));
    resize(QDesktopWidget().screenGeometry().width()*2/5, QDesktopWidget().screenGeometry().height()*2/5);

    Layout=new QGridLayout();
    Layout->setContentsMargins(0, 0, 0, 0);
    Layout->setSpacing(0);

    // Filters
    for (size_t Pos=0; Pos<2; Pos++)
    {
        Layout->setColumnStretch(Pos * 2, 1);
        Layout->addWidget(m_filterSelectors[Pos], 0, Pos * 2, 1, 1);
        handleFilterChange(m_filterSelectors[Pos], Pos);
    }

    Layout->addItem(new QSpacerItem(0, 1, QSizePolicy::Maximum, QSizePolicy::Minimum), 0, 14, 1, 1);

    splitter = new QSplitter;
    splitter->setStyleSheet("QSplitter::handle { background-color: gray }");
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Players
    for (size_t Pos=0; Pos<2; Pos++)
    {
        imageLabels[Pos] = new PlayerWindow();
        imageLabels[Pos]->setFile(FileInformationData_->fileName());

        if(Config::instance().getDebug())
            imageLabels[Pos]->setStyleSheet("background: yellow");

        imageLabels[Pos]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        imageLabels[Pos]->setMinimumSize(20, 20);
//        imageLabels[Pos]->showDebugOverlay(Config::instance().getDebug());

        splitter->addWidget(imageLabels[Pos]);
//        imageLabels[Pos]->installEventFilter(this);
    }

    splitter->handle(1)->installEventFilter(this);
    splitter->installEventFilter(this);
    connect(splitter, &QSplitter::splitterMoved, this, [&] {
            qDebug() << "splitter moved";
    });

    Layout->addWidget(splitter, 1, 0, 1, 3);

    // Info
    InfoArea=NULL;
    //InfoArea=new Info(this, FileInfoData, Info::Style_Columns);
    //Layout->addWidget(InfoArea, 1, 1, 1, 3, Qt::AlignHCenter);
    //Layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 1, 1, 3, Qt::AlignHCenter);

    // Slider
    Slider=new QSlider(Qt::Horizontal);
    Slider->setMaximum(FileInfoData->Glue->VideoFrameCount_Get() - 1);

    connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
    connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(onCursorMoved(int)));

    Layout->addWidget(Slider, 2, 0, 1, 3);

    plot = createCommentsPlot(FileInformationData_, nullptr);
    plot->enableAxis(QwtPlot::yLeft, false);
    plot->enableAxis(QwtPlot::xBottom, true);
    plot->setAxisScale(QwtPlot::xBottom, Slider->minimum(), Slider->maximum());
    plot->setAxisAutoScale(QwtPlot::xBottom, false);

    plot->setFrameShape(QFrame::NoFrame);
    plot->setObjectName("commentsPlot");
    plot->setStyleSheet("#commentsPlot { border: 0px solid transparent; }");
    plot->canvas()->setObjectName("commentsPlotCanvas");
    dynamic_cast<QFrame*>(plot->canvas())->setFrameStyle( QFrame::NoFrame );
    dynamic_cast<QFrame*>(plot->canvas())->setContentsMargins(0, 0, 0, 0);

    connect( plot, SIGNAL( cursorMoved( int ) ), SLOT( onCursorMoved( int ) ) );
    plot->canvas()->installEventFilter( this );

    // Notes
    Layout->addWidget(
                plot,
                3, 0, 1, 3, Qt::AlignBottom);

    // Control
    ControlArea=new Control(this, FileInfoData, true);
    Layout->addWidget(ControlArea, 4, 0, 1, 3, Qt::AlignBottom);

    for (size_t Pos=0; Pos<2; Pos++)
    {
        connect(ControlArea, &Control::playClicked, imageLabels[Pos], &PlayerWindow::play);
        connect(ControlArea, &Control::stopClicked, imageLabels[Pos], &PlayerWindow::pause);
    }

    setLayout(Layout);

    // Picture
    Picture=NULL;
    Picture_Current[0] = Filters_Default1;
    Picture_Current[1] = Filters_Default2;

    for(int playerIndex = 0; playerIndex < 2; ++playerIndex)
    {
        auto& filterSelector = m_filterSelectors[playerIndex];
        filterSelector->setCurrentIndex(Picture_Current[playerIndex]);
    }

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;

    // Shortcuts
    QShortcut *shortcutJ = new QShortcut(QKeySequence(Qt::Key_J), this);
    QObject::connect(shortcutJ, SIGNAL(activated()), ControlArea->M1, SLOT(click()));
    QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QObject::connect(shortcutLeft, SIGNAL(activated()), ControlArea->Minus, SLOT(click()));
    QShortcut *shortcutK = new QShortcut(QKeySequence(Qt::Key_K), this);
    QObject::connect(shortcutK, SIGNAL(activated()), ControlArea->Pause, SLOT(click()));
    QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QObject::connect(shortcutRight, SIGNAL(activated()), ControlArea->Plus, SLOT(click()));
    QShortcut *shortcutL = new QShortcut(QKeySequence(Qt::Key_L), this);
    QObject::connect(shortcutL, SIGNAL(activated()), ControlArea->P1, SLOT(click()));
    QShortcut *shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(shortcutSpace, SIGNAL(activated()), ControlArea->PlayPause, SLOT(click()));
    QShortcut *shortcutF = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(shortcutF, SIGNAL(activated()), this, SLOT(on_Full_triggered()));
}

//---------------------------------------------------------------------------
BigDisplay::~BigDisplay()
{
    delete Picture;
}

//---------------------------------------------------------------------------


//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::InitPicture()
{
    if (!Picture)
    {
        string FileName_string=FileInfoData->fileName().toUtf8().data();
        #ifdef _WIN32
            replace(FileName_string.begin(), FileName_string.end(), '/', '\\' );
        #endif
        int width=QDesktopWidget().screenGeometry().width()*2/5;
        if (width%2)
            width--; //odd number is wanted for filters
        int height=QDesktopWidget().screenGeometry().height()*2/5;
        if (height%2)
            height--; //odd number is wanted for filters
        Picture=new FFmpeg_Glue(FileName_string.c_str(), FileInfoData->ActiveAllTracks, &FileInfoData->Stats, NULL, NULL);
        Picture->setThreadSafe(true);

        if (FileName_string.empty())
            Picture->InputData_Set(FileInfoData->Glue->InputData_Get()); // Using data from the analyzed file
        Picture->AddOutput(0, width, height, FFmpeg_Glue::Output_QImage);
        Picture->AddOutput(1, width, height, FFmpeg_Glue::Output_QImage);

        m_filterSelectors[0]->setCurrentFilter(Picture_Current[0], true);
        m_filterSelectors[1]->setCurrentFilter(Picture_Current[1], false);
    }
}

QPixmap fromImage(const FFmpeg_Glue::Image& image)
{
    if(image.isNull())
        return QPixmap();

    return QPixmap::fromImage(QImage(image.data(), image.width(), image.height(), image.linesize(), QImage::Format_RGB888));
}

void BigDisplay::ShowPicture ()
{
    if (!isVisible())
        return;

	if (!Picture)
		return;

    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;

    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;
    Picture->FrameAtPosition(Frames_Pos);

    if (QThread::currentThread() == thread())
    {
        updateImagesAndSlider(fromImage(Picture->Image_Get(0)), fromImage(Picture->Image_Get(1)), Frames_Pos);
    }
    else
    {
        QMetaObject::invokeMethod(this, "updateImagesAndSlider", Qt::BlockingQueuedConnection,
                                  Q_ARG(const QPixmap&, fromImage(Picture->Image_Get(0))),
                                  Q_ARG(const QPixmap&, fromImage(Picture->Image_Get(1))),
                                  Q_ARG(const int, Frames_Pos));
    }

    // Stats
    if (ControlArea)
        ControlArea->Update();
    if (InfoArea)
        InfoArea->Update();
}

//***************************************************************************
// Events
//***************************************************************************

//---------------------------------------------------------------------------
void BigDisplay::onSliderValueChanged(int value)
{
    Q_EMIT rewind(Slider->value());
}

//---------------------------------------------------------------------------
void BigDisplay::updateSelection(int Pos, ImageLabel* image, FilterSelector& filterSelector)
{
    auto& opts = filterSelector.getOptions();

    image->disconnect();

    if(strcmp(Filters[Pos].Name, "Waveform Target") == 0 ||
            strcmp(Filters[Pos].Name, "Vectorscope Target") ==  0 ||
            strcmp(Filters[Pos].Name, "Zoom") ==  0 ||
            strcmp(Filters[Pos].Name, "Pixel Scope") == 0)
    {
        int xIndex = 0;
        int yIndex = 1;
        int wIndex = 2;
        int hIndex = 3;

        if(strcmp(Filters[Pos].Name, "Pixel Scope") == 0)
        {
            xIndex = 1;
            yIndex = 2;
            wIndex = 3;
            hIndex = 4;
        }

        auto& xSpinBox = opts.Sliders_SpinBox[xIndex];
        auto& ySpinBox = opts.Sliders_SpinBox[yIndex];
        auto& wSpinBox = opts.Sliders_SpinBox[wIndex];
        auto& hSpinBox = opts.Sliders_SpinBox[hIndex];

        image->setSelectionArea(xSpinBox->value(), ySpinBox->value(), wSpinBox->value(), hSpinBox->value());

        image->setMinSelectionSize(QSizeF(wSpinBox->minimum(), hSpinBox->minimum()));
        image->setMaxSelectionSize(QSizeF(wSpinBox->maximum(), hSpinBox->maximum()));

        connect(xSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::moveSelectionX);
        connect(ySpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::moveSelectionY);
        connect(wSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::changeSelectionWidth);
        connect(hSpinBox, &DoubleSpinBoxWithSlider::controlValueChanged, image, &ImageLabel::changeSelectionHeight);

        connect(image, &ImageLabel::selectionChangeFinished, [&](const QRectF& geometry) {
            qDebug() << "selectionChangeFinished: "
                     << ", x: " << geometry.x() << ", y: " << geometry.y()
                     << ", w: " << geometry.width() << ", h: " << geometry.height();

            xSpinBox->applyValue(geometry.topLeft().x(), false);
            ySpinBox->applyValue(geometry.topLeft().y(), false);
            wSpinBox->applyValue(geometry.width(), false);
            hSpinBox->applyValue(geometry.height(), false);

        });

        connect(image, &ImageLabel::selectionChanged, [&](const QRectF& geometry) {
            qDebug() << "selectionChanged: "
                     << ", x: " << geometry.x() << ", y: " << geometry.y()
                     << ", w: " << geometry.width() << ", h: " << geometry.height();

            xSpinBox->applyValue(geometry.x(), false);
            ySpinBox->applyValue(geometry.y(), false);
            wSpinBox->applyValue(geometry.width(), false);
            hSpinBox->applyValue(geometry.height(), false);
        });
    }
    else
    {
        image->clearSelectionArea();
    }
}

void BigDisplay::handleFilterChange(FilterSelector *filterSelector, int Pos)
{
    connect(filterSelector, &FilterSelector::filterChanged, [this, Pos](const QString& filterString) {

        QString str = filterString;

        str.replace(QString("${width}"), QString::number(FileInfoData->Glue->Width_Get()));
        str.replace(QString("${height}"), QString::number(FileInfoData->Glue->Height_Get()));
        str.replace(QString("${dar}"), QString::number(FileInfoData->Glue->DAR_Get()));

    //    QSize windowSize = imageLabels[Pos]->pixmapSize();
        QSize windowSize = imageLabels[Pos]->size();

        str.replace(QString("${window_width}"), QString::number(windowSize.width()));
        str.replace(QString("${window_height}"), QString::number(windowSize.height()));

        QString tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QDir tempDir(tempLocation);

        QString qctoolsTmpSubDir = "qctools";
        QString fontFileName = "Anonymous_Pro_B.ttf";

        if(tempDir.exists())
        {
            QDir qctoolsTmpDir(tempLocation + "/" + qctoolsTmpSubDir);
            if(!qctoolsTmpDir.exists())
                tempDir.mkdir(qctoolsTmpSubDir);

            QFile fontFile(qctoolsTmpDir.path() + "/" + fontFileName);
            if(!fontFile.exists())
            {
                QFile::copy(":/" + fontFileName, fontFile.fileName());
            }

            if(fontFile.exists())
            {
                QString fontFileName(fontFile.fileName());
                fontFileName = fontFileName.replace(":", "\\\\:"); // ":" is a reserved character, it must be escaped
                str.replace(QString("${fontfile}"), fontFileName);
            }
        }

        imageLabels[Pos]->setFilter(str);
    });
}

bool BigDisplay::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == splitter && event->type() == QEvent::Resize)
    {
        qDebug() << "entering splitter: resize event";
        bool result = QWidget::eventFilter(watched, event);
        qDebug() << "leaving splitter: resize event";

        return result;
    }

    if((watched == imageLabels[0] || watched == imageLabels[1]) && event->type() == QEvent::Resize)
    {
        qDebug() << "entering imagelabel: resize event";
        bool result = QWidget::eventFilter(watched, event);
        qDebug() << "leaving imagelabel: resize event";
        return result;
    }

    if((watched == splitter || watched == splitter->handle(1)) && event->type() == QEvent::MouseButtonDblClick)
    {
        QList<int> sizes;

        int left = (width() - splitter->handle(0)->width()) / 2;
        int right = width() - splitter->handle(0)->width() - left;

        sizes << left << right;

        splitter->setSizes(sizes);
    } else if(watched == plot->canvas()) {

        if(event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton)
            {
                showEditFrameCommentsDialog(parentWidget(), FileInfoData, FileInfoData->ReferenceStat(), Slider->value());
            }
        } else if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if(keyEvent->key() == Qt::Key_M)
            {
                showEditFrameCommentsDialog(parentWidget(), FileInfoData, FileInfoData->ReferenceStat(), Slider->value());
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void BigDisplay::onCursorMoved(int x)
{
    plot->setCursorPos(x);
    Slider->setValue(x);
}

void BigDisplay::updateImagesAndSlider(const QPixmap &pixmap1, const QPixmap &pixmap2, int sliderPos)
{
    if (Slider->sliderPosition() != sliderPos)
        Slider->setSliderPosition(sliderPos);

    /*
    if(!pixmap1.isNull())
        imageLabels[0]->setPixmap(pixmap1);

    if(!pixmap2.isNull())
        imageLabels[1]->setPixmap(pixmap2);
        */
}

//---------------------------------------------------------------------------
void BigDisplay::on_Full_triggered()
{
    if (isMaximized())
        setWindowState(Qt::WindowActive);
    else
        setWindowState(Qt::WindowMaximized);
}

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "GUI/TinyDisplay.h"

#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "Core/FileInformation.h"
#include "Core/CommonStats.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QVector>
#include "playerwindow.h"

PlayerWindow* player;
TinyDisplay::TinyDisplay(QWidget *parent, FileInformation* FileInformationData_)
    : QWidget(parent),
      lastWidth(0),
      FileInfoData(FileInformationData_)
{
    player = new PlayerWindow();

    // To update
    ControlArea = NULL;

    BigDisplayArea = NULL;

    needsUpdate = true;
    lastFramePos = 0;

    emptyPixmap = QPixmap();

    Layout = new QHBoxLayout();
    Layout->setSpacing(1);
    Layout->setMargin(1);
    Layout->setContentsMargins(1, 0, 1, 0);

    scaledLogo = QPixmap(":/icon/logo.jpg").scaled(72, 72);
    thumbnails = QVector<QToolButton*>();

    setLayout(Layout);

    connect(this, SIGNAL(resized()), this, SLOT(thumbsLayoutResized()));
}

TinyDisplay::~TinyDisplay()
{
    while (!thumbnails.empty()) {
        QToolButton *t = thumbnails.takeLast();
        delete t;
    }

    delete Layout;
}

void TinyDisplay::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    Q_EMIT resized();
}

void TinyDisplay::thumbsLayoutResized()
{
    const int width = QWidget::width();
    const int THUMB_WANTED_WIDTH = 108;

    if (lastWidth != width) {
        int total_thumbs = width / THUMB_WANTED_WIDTH;
        if (total_thumbs % 2 == 0) total_thumbs--;

        if (total_thumbs > thumbnails.size()) {
            int diff = total_thumbs - thumbnails.size();
            for (int i = 0; i < diff; ++i) {
                QToolButton *tool_button = new QToolButton(this);
                tool_button->setIconSize(QSize(72, 72));
                tool_button->setMinimumHeight(THUMB_HEIGHT);
                tool_button->setMinimumWidth(THUMB_WIDTH);
                tool_button->setIcon(scaledLogo);

                connect(tool_button, SIGNAL(clicked(bool)),
                        this, SLOT(on_thumbnails_clicked(bool)));

                thumbnails.append(tool_button);

                Layout->addWidget(tool_button);
            }

            needsUpdate = true;
        }
        else if (total_thumbs < thumbnails.size()) {
            int diff = thumbnails.size() - total_thumbs;
            for (int i = 0; i < diff; ++i) {
                QToolButton *tool_button = thumbnails.takeLast();
                disconnect(tool_button, 0, 0, 0);
                Layout->removeWidget(tool_button);
                delete tool_button;
            }

            needsUpdate = true;
        }

        if (needsUpdate) {
            int middle = thumbnails.size() / 2;
            for (int i = 0; i < thumbnails.size(); ++i) {
                if (i == middle)
                    thumbnails[i]->setStyleSheet("");
                else
                    thumbnails[i]->setStyleSheet("background-color: grey;");

                if (!FileInfoData->PlayBackFilters_Available())
                    thumbnails[i]->setEnabled(false);
            }

            Update();
        }

        lastWidth = width;
    }
}

//***************************************************************************
// Actions
//***************************************************************************

QPixmap toPixmap(const QByteArray& bytes)
{
    QPixmap pixmap;

    if (!bytes.isEmpty())
        pixmap.loadFromData(bytes);
    else
    {
        pixmap.load(":/icon/logo.png");
        pixmap = pixmap.scaled(72, 72);
    }

    return pixmap;
}

void TinyDisplay::Update(bool updateBigDisplay)
{
    if (!FileInfoData->ReferenceStat())
        return;

    if(thread() != QThread::currentThread())
    {
        // qDebug() << "TinyDisplay::Update: called from non-UI thread";
        QMetaObject::invokeMethod(this, "Update", Q_ARG(bool, updateBigDisplay));
        return;
    }

    Q_ASSERT(QThread::currentThread() == thread());

    // start upto stop are the current movie frames that need to made into thumbs
    unsigned long currentFrame = FileInfoData->Frames_Pos_Get();

    // current frame positions in the movie
    unsigned long current = FileInfoData->ReferenceStat()->x_Current;
    unsigned long current_max = FileInfoData->ReferenceStat()->x_Current_Max;

    // do we need to update thumbnails?
    if (needsUpdate || lastFramePos != currentFrame) {
        unsigned long framePos = currentFrame;

        // NOTE: current frame should never be bigger the current max!
        if (framePos >= current_max)
            framePos = current_max - 1;

        unsigned long total_thumbs = thumbnails.size();
        unsigned int center = total_thumbs / 2;

        if (needsUpdate || framePos > lastFramePos) {
            // movie is moving forward
            unsigned long diff = framePos - lastFramePos;
            for (unsigned i = 0; i < total_thumbs; ++i) {
                if (framePos + i >= center && framePos - center + i < current) {
                    if (!needsUpdate && (diff < total_thumbs && i < total_thumbs - diff)) {
                        thumbnails[i]->setIcon(thumbnails[i+diff]->icon());
                    } else {
                        QPixmap pixmap = toPixmap(FileInfoData->Picture_Get(framePos - center + i));
                        thumbnails[i]->setIcon(pixmap.copy(0, 0, 72, 72));
                    }
                } else {
                    thumbnails[i]->setIcon(emptyPixmap);
                    needsUpdate = true;
                }
            }

        } else {
            // movie is moving backward
            unsigned long diff = lastFramePos - framePos;
            for (int i = total_thumbs - 1; i >= 0; --i) {
                unsigned ui = (unsigned) i;
                if (framePos + ui >= center && framePos - center + ui < current) {
                    if (diff < total_thumbs && i - (int) diff >= 0) {
                        thumbnails[ui]->setIcon(thumbnails[ui-diff]->icon());
					} else {
                        QPixmap pixmap = toPixmap(FileInfoData->Picture_Get(framePos - center + ui));
                        thumbnails[ui]->setIcon(pixmap.copy(0, 0, 72, 72));
					}
                } else {
                    thumbnails[ui]->setIcon(emptyPixmap);
                    needsUpdate = true;
                }
            }
        }

        lastFramePos = framePos;

        // This assures that if the thumbs are not yet available due to pre-processing,
        // we update thumbs again (till all thumbs for current frames are available)
        if (framePos - center + total_thumbs < current)
            needsUpdate = false;

        if (updateBigDisplay && BigDisplayArea) {
            BigDisplayArea->ShowPicture();
        }
    }
}

void TinyDisplay::Filters_Show()
{
    on_thumbnails_clicked(true);
}

//***************************************************************************
// Events
//***************************************************************************

void TinyDisplay::on_thumbnails_clicked(bool)
{
    int total_thumbs = thumbnails.size();

    // Positioning the current frame if any button but the center button is clicked
    if (sender() != thumbnails[total_thumbs / 2]) {
        for (int i = 0; i < total_thumbs; ++i) {
            if (sender() == thumbnails[i]) {
                unsigned long framePos = FileInfoData->Frames_Pos_Get();
                FileInfoData->Frames_Pos_Set(framePos + i - total_thumbs / 2);
                break;
            }
        }
    }

    // player->show();
    LoadBigDisplay();
}

void TinyDisplay::LoadBigDisplay()
{
    if (BigDisplayArea == NULL) {
        BigDisplayArea = new BigDisplay(this, FileInfoData);

        unsigned long width = QApplication::desktop()->screenGeometry().width();
        unsigned long height = QApplication::desktop()->screenGeometry().height();
        BigDisplayArea->resize(width - 300, height - 300);

        //FIXME: BigDisplayArea->move(geometry().left()+geometry().width(), geometry().top());
        BigDisplayArea->move(150, 150);
        if (ControlArea) {
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M9         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M9_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M2         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M2_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M1         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M1_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->M0         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_M0_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Minus      , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Minus_clicked     (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->PlayPause  , SIGNAL(clicked(bool)), ControlArea, SLOT(on_PlayPause_clicked (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Pause      , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Pause_clicked     (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->Plus       , SIGNAL(clicked(bool)), ControlArea, SLOT(on_Plus_clicked      (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P0         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P0_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P1         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P1_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P2         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P2_clicked        (bool)));
            BigDisplayArea->connect(BigDisplayArea->ControlArea->P9         , SIGNAL(clicked(bool)), ControlArea, SLOT(on_P9_clicked        (bool)));
            connect(BigDisplayArea, SIGNAL(rewind(int)), ControlArea, SLOT(rewind(int)));
        }
    }

    BigDisplayArea->hide();
    BigDisplayArea->InitPicture();

    BigDisplayArea->show();
    BigDisplayArea->ShowPicture();
}

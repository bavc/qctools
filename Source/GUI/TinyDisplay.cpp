/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "GUI/TinyDisplay.h"

#include "GUI/BigDisplay.h"
#include "GUI/Control.h"
#include "GUI/FileInformation.h"
#include "Core/CommonStats.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>

TinyDisplay::TinyDisplay(QWidget *parent, FileInformation* FileInformationData_)
    : QWidget(parent),
      FileInfoData(FileInformationData_)
{
    // To update
    ControlArea = NULL;

    BigDisplayArea = NULL;

    needsUpdate = true;
    lastFramePos = 0;

    emptyPixmap = QPixmap();

    QHBoxLayout* Layout = new QHBoxLayout();
    Layout->setSpacing(1);
    Layout->setMargin(1);
    Layout->setContentsMargins(1, 0, 1, 0);

    QPixmap scaled_logo = QPixmap(":/icon/logo.jpg").scaled(72, 72);

    for (int i = 0; i < TOTAL_THUMBS; ++i) {
        thumbnails[i] = new QToolButton(this);
        thumbnails[i]->setIconSize(QSize(72, 72));
        thumbnails[i]->setMinimumHeight(84);
        thumbnails[i]->setMinimumWidth(84);
        thumbnails[i]->setIcon(scaled_logo);

        connect(thumbnails[i], SIGNAL(clicked(bool)),
                this, SLOT(on_thumbnails_clicked(bool)));

        Layout->addWidget(thumbnails[i]);
        if (i != MID_THUMB_INDEX) {
            thumbnails[i]->setStyleSheet("background-color: grey;");
        }
    }

    setLayout(Layout);

    // Disable PlayBackFilters if the source file is not available
    if (!FileInfoData->PlayBackFilters_Available()) {
        for (int i = 0; i < TOTAL_THUMBS; ++i)
            thumbnails[i]->setEnabled(false);
    }
}

TinyDisplay::~TinyDisplay()
{
    for (int i = 0; i < TOTAL_THUMBS; ++i) {
        if (thumbnails[i])
            delete thumbnails[i];
    }
}

//***************************************************************************
// Actions
//***************************************************************************

void TinyDisplay::Update()
{
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

        unsigned long center = MID_THUMB_INDEX;

        if (needsUpdate || framePos > lastFramePos) {
            // movie is moving forward
            unsigned long diff = framePos - lastFramePos;
            for (unsigned i = 0; i < TOTAL_THUMBS; ++i) {
                if (framePos + i >= center && framePos - center + i < current) {
                    if (!needsUpdate && (diff < TOTAL_THUMBS && i < TOTAL_THUMBS - diff)) {
                        thumbnails[i]->setIcon(thumbnails[i+diff]->icon());
                    } else {
                        QPixmap *pixmap = FileInfoData->Picture_Get(framePos - center + i);
                        thumbnails[i]->setIcon(pixmap->copy(0, 0, 72, 72));
                    }
                } else {
                    thumbnails[i]->setIcon(emptyPixmap);
                }
            }

        } else {
            // movie is moving backward
            unsigned long diff = lastFramePos - framePos;
            for (int i = TOTAL_THUMBS - 1; i >= 0; --i) {
                unsigned ui = (unsigned) i;
                if (framePos + ui >= center && framePos - center + ui < current) {
                    if (diff < TOTAL_THUMBS && i - (int) diff >= 0) {
                        thumbnails[ui]->setIcon(thumbnails[ui-diff]->icon());
                    } else {
                        QPixmap *pixmap = FileInfoData->Picture_Get(framePos - center + ui);
                        thumbnails[ui]->setIcon(pixmap->copy(0, 0, 72, 72));
                    }
                } else {
                    thumbnails[ui]->setIcon(emptyPixmap);
                }
            }
        }

        lastFramePos = framePos;

        // This assures that if the thumbs are not yet available due to pre-processing,
        // we update thumbs again (till all thumbs for current frames are available)
        if (framePos - center + TOTAL_THUMBS < current)
            needsUpdate = false;

        if (BigDisplayArea) {
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
    // Positioning the current frame if any button but the center button is clicked
    if (sender() != thumbnails[MID_THUMB_INDEX]) {
        for (int i = 0; i < TOTAL_THUMBS; ++i) {
            if (sender() == thumbnails[i]) {
                unsigned long framePos = FileInfoData->Frames_Pos_Get();
                FileInfoData->Frames_Pos_Set(framePos + i - MID_THUMB_INDEX);
                break;
            }
        }
    }

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
        }
    }

    BigDisplayArea->hide();
    BigDisplayArea->show();
    BigDisplayArea->ShowPicture();
}

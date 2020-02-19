/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef GUI_TinyDisplay_H
#define GUI_TinyDisplay_H

#include <QWidget>
#include <QVector>
#include <QResizeEvent>

class FileInformation;
class Control;
class Player;

class QLabel;
class QToolButton;
class QHBoxLayout;

class TinyDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit TinyDisplay(QWidget *parent, FileInformation* FileInfoData);
    ~TinyDisplay();

    // To update
    Control                    *ControlArea;

    // Commands
    void                        Filters_Show(); //Quick hack for showing filters

public Q_SLOTS:
    void                        Update(bool updateBigDisplay = true);

private:
    static const int            TOTAL_THUMBS = 9;
    static const int            THUMB_WIDTH = 84;
    static const int            THUMB_HEIGHT = 84;

    QPixmap                     emptyPixmap;
    QPixmap                     scaledLogo;

    int                         lastWidth;

    QHBoxLayout*                Layout;

protected:
    FileInformation*            FileInfoData;

    bool                        needsUpdate;
    unsigned long               lastFramePos;

    QVector<QToolButton*>       thumbnails;

    virtual void                resizeEvent(QResizeEvent *);

Q_SIGNALS:
    void                        resized();
    void                        thumbnailClicked();

private Q_SLOTS:
    void                        thumbsLayoutResized();
    void                        on_thumbnails_clicked(bool checked);
};

#endif

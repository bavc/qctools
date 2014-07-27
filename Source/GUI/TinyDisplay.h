/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_TinyDisplay_H
#define GUI_TinyDisplay_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QWidget>

class FileInformation;
class Control;
class BigDisplay;

class QLabel;
class QToolButton;
//---------------------------------------------------------------------------

//***************************************************************************
// Class
//***************************************************************************

class TinyDisplay : public QWidget
{
    Q_OBJECT
    
public:
    explicit TinyDisplay(QWidget *parent, FileInformation* FileInfoData);
    ~TinyDisplay();

    // To update
    Control*                    ControlArea;
    BigDisplay*                 BigDisplayArea;

    // Commands
    void                        Update                      ();
    void                        Filters_Show                (); //Quick hack for showing filters

    // Info
    bool                        ShouldUpate;

protected:
    // File information
    FileInformation*            FileInfoData;
    int                         Frames_Pos;

    // Widgets
    QToolButton*                Labels[9];
    QPixmap                     Labels_Temp[9];

private Q_SLOTS:
    void on_Labels_Middle_clicked(bool checked);
};

#endif // GUI_TinyDisplay_H

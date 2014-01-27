/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_BigDisplay_H
#define GUI_BigDisplay_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QDialog>

class FFmpeg_Glue;
class Control;
class Info;
class FileInformation;

class QLabel;
class QToolButton;
class QComboBox;
class QPixmap;
class QSlider;
class QCheckBox;
class QGridLayout;
//---------------------------------------------------------------------------

//***************************************************************************
// Helper
//***************************************************************************

class ImageLabel : public QWidget
{
    Q_OBJECT

public:
    explicit ImageLabel (FFmpeg_Glue** Picture, size_t Pos, QWidget *parent=NULL);
    void                        Remove ();
    bool                        Pixmap_MustRedraw;
    bool                        IsMain;

protected:
    void paintEvent (QPaintEvent*);

private:
    QPixmap                     Pixmap;
    FFmpeg_Glue**               Picture;
    size_t                      Pos;
};

//***************************************************************************
// Class
//***************************************************************************

class BigDisplay : public QDialog
{
    Q_OBJECT
    
public:
    // Constructor/Destructor
    explicit BigDisplay(QWidget *parent, FileInformation* FileInfoData);
    ~BigDisplay();

    // Actions
    void ShowPicture ();

    // Info
    bool                        ShouldUpate;

    // Content
    Control*                    ControlArea;

protected:
    // File information
    FileInformation*            FileInfoData;
    int                         Frames_Pos;

    // Filters
    FFmpeg_Glue*                Picture;
    size_t                      Picture_Current1;
    size_t                      Picture_Current2;

    // Content
    Info*                       InfoArea;
    ImageLabel*                 Image1;
    ImageLabel*                 Image2;
    QSlider*                    Slider;
    QToolButton*                FiltersList1;
    QToolButton*                FiltersList2;

    // Temp
    int                         Image_Width;
    int                         Image_Height;
    QGridLayout*                Layout;
    
    // Events
    void                        resizeEvent (QResizeEvent * event);

public Q_SLOTS:
    void on_FiltersList1_currentIndexChanged(QAction * action);
    void on_FiltersList2_currentIndexChanged(QAction * action);
    void on_Slider_sliderMoved(int value);
    void on_Slider_actionTriggered (int action);
    void on_FiltersSource_stateChanged(int state);
    void on_FiltersList1_click();
    void on_FiltersList2_click();
};

#endif // GUI_BigDisplay_H

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
#include <string>

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
class QRadioButton;
class QHBoxLayout;
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
    struct options
    {
        QCheckBox*              Checks[4];
        QSlider*                Sliders[4];
        QLabel*                 Sliders_Label[4];
        QRadioButton*           Radios[4][3];
        //QToolButton*          FiltersList1;
        QComboBox*              FiltersList;
        QLabel*                 FiltersList_Fake;
    };
    options                     Options[2];
    struct previous_values
    {
        int                     Values[4];

        previous_values()
        {
            Values[0]=-2;
            Values[1]=-2;
            Values[2]=-2;
            Values[3]=-2;
        }
    };
    std::vector<previous_values>PreviousValues[2];
    Info*                       InfoArea;
    ImageLabel*                 Image1;
    ImageLabel*                 Image2;
    QSlider*                    Slider;

    // Temp
    int                         Image_Width;
    int                         Image_Height;
    QGridLayout*                Layout;
    size_t                      FiltersListDefault_Count;
    
    // Events
    void                        resizeEvent (QResizeEvent * event);
    void                        FiltersList_currentIndexChanged(size_t Pos, size_t FilterPos, QGridLayout* Layout0);
    void                        FiltersList1_currentIndexChanged(size_t FilterPos);
    void                        FiltersList2_currentIndexChanged(size_t FilterPos);
    std::string                 FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current);
    void                        FiltersList1_currentOptionChanged(size_t Picture_Current);
    void                        FiltersList2_currentOptionChanged(size_t Picture_Current);

public Q_SLOTS:
    void on_FiltersList1_currentIndexChanged(QAction * action);
    void on_FiltersList2_currentIndexChanged(QAction * action);
    void on_FiltersList1_currentIndexChanged(int Pos);
    void on_FiltersList2_currentIndexChanged(int Pos);
    void on_Slider_sliderMoved(int value);
    void on_Slider_actionTriggered (int action);
    void on_FiltersSource_stateChanged(int state);
    void on_FiltersList1_click();
    void on_FiltersList2_click();
    void on_FiltersOptions1_click();
    void on_FiltersOptions2_click();

    void on_M1_triggered();
    void on_Minus_triggered();
    void on_PlayPause_triggered();
    void on_Pause_triggered();
    void on_Plus_triggered();
    void on_P1_triggered();
    void on_Full_triggered();
};

#endif // GUI_BigDisplay_H

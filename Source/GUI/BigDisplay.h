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
#include <QDoubleSpinBox>
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
class QButtonGroup;
class QHBoxLayout;
class QPushButton;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
const size_t Args_Max=7;
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

class BigDisplay;
class DoubleSpinBoxWithSlider : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit DoubleSpinBoxWithSlider (DoubleSpinBoxWithSlider** Others, int Min, int Max, int Divisor, int Current, const char* Name, BigDisplay* Display, size_t Pos, bool IsBitSlice, bool IsFilter, bool IsPeak, bool IsMode, bool IsScale, bool IsColorspace, QWidget *parent=NULL);
    ~DoubleSpinBoxWithSlider();

    bool IsBitSlice;
    bool IsFilter;
    bool IsPeak;
    bool IsMode;
    bool IsScale;
    bool IsColorspace;
    void ChangeMax(int Max);

protected:
    void enterEvent (QEvent* event);
    void leaveEvent (QEvent* event);
    void keyPressEvent (QKeyEvent* event);
    void moveEvent (QMoveEvent * event);

    void hidePopup ();

    void showEvent (QShowEvent* event);
    QString textFromValue (double value) const;
    double  valueFromText (const QString& text) const;

private:
    DoubleSpinBoxWithSlider**   Others;
    QWidget*                    Popup;
    QSlider*                    Slider;
    int                         Min;
    int                         Max;
    int                         Divisor;
    BigDisplay*                 Display;
    size_t                      Pos;

public Q_SLOTS:
    void on_valueChanged(double);
    void on_sliderMoved(int);
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
        QCheckBox*              Checks[Args_Max];
        QLabel*                 Sliders_Label[Args_Max];
        DoubleSpinBoxWithSlider* Sliders_SpinBox[Args_Max];
        QRadioButton*           Radios[Args_Max][4];
        QButtonGroup*           Radios_Group[Args_Max];
        QPushButton*            ColorButton[Args_Max];
        int                     ColorValue[Args_Max];
        QComboBox*              FiltersList;
        QLabel*                 FiltersList_Fake;
    };
    options                     Options[2];
    struct previous_values
    {
        int                     Values[Args_Max];

        previous_values()
        {
            for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
                Values[OptionPos]=-2;
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
    void on_FiltersOptions1_toggle(bool checked);
    void on_FiltersOptions2_toggle(bool checked);
    void on_FiltersSpinBox1_click();
    void on_FiltersSpinBox2_click();
    void on_Color1_click(bool checked);
    void on_Color2_click(bool checked);

    void on_Full_triggered();
};

#endif // GUI_BigDisplay_H

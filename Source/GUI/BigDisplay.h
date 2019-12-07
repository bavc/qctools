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
#include <QResizeEvent>
#include <QTimer>
#include <string>
#include "doublespinboxwithslider.h"

class PlayerWindow;
class FFmpeg_Glue;
class Control;
class Info;
class FileInformation;
class ImageLabel;

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
class QSplitter;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
const size_t Args_Max=7;
//---------------------------------------------------------------------------

class BigDisplay;

//***************************************************************************
// Class
//***************************************************************************

class CommentsPlot;
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
    
    void InitPicture();
    
protected:
    // File information
    FileInformation*            FileInfoData;
    int                         Frames_Pos;

    // Filters
    FFmpeg_Glue*                Picture;
    size_t                      Picture_Current[2];

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

    PlayerWindow*               imageLabels[2];
    QSlider*                    Slider;

    // Temp
    QGridLayout*                Layout;
    size_t                      FiltersListDefault_Count;

    // Events
    void                        FiltersList_currentIndexChanged(size_t Pos, size_t FilterPos, QGridLayout* Layout0);
    void                        FiltersList_currentIndexChanged(size_t Pos, size_t FilterPos);

    std::string                 FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current);
    void                        on_FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current);

    void updateSelection(int Pos, ImageLabel* image, options& opts);

private:
    QSplitter* splitter;
    QTimer timer;

    CommentsPlot* plot;

    void setCurrentFilter(size_t playerIndex, size_t filterIndex);
    void hideOthersOnEntering(DoubleSpinBoxWithSlider* doubleSpinBox, DoubleSpinBoxWithSlider** others);

protected:
    bool eventFilter(QObject *, QEvent *);
    void resizeEvent(QResizeEvent * e);

Q_SIGNALS:
	void rewind(int pos);

private Q_SLOTS:
    void onCursorMoved(int x);

public Q_SLOTS:
    void updateImagesAndSlider(const QPixmap& pixmap1, const QPixmap& pixmap2, int sliderPos);
    void onSliderValueChanged(int value);

    void on_FiltersList_currentIndexChanged(int Pos);

    void on_FiltersSource_stateChanged(int state);
    void on_FiltersOptions1_click();
    void on_FiltersOptions2_click();
    void on_FiltersOptions1_toggle(bool checked);
    void on_FiltersOptions2_toggle(bool checked);
    void on_FiltersSpinBox1_click();
    void on_FiltersSpinBox2_click();
    void on_Color1_click(bool checked);
    void on_Color2_click(bool checked);

    void on_Full_triggered();

private Q_SLOTS:
    void onAfterResize();
};

#endif // GUI_BigDisplay_H

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
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <string>
#include "doublespinboxwithslider.h"
#include "filters.h"
#include "filterselector.h"

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

    Info*                       InfoArea;

    PlayerWindow*               imageLabels[2];
    QSlider*                    Slider;

    // Temp
    QGridLayout*                Layout;

    std::string                 FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current);
    void                        on_FiltersList_currentOptionChanged(size_t Pos, size_t Picture_Current);

    void updateSelection(int Pos, ImageLabel* image, FilterSelector& opts);

private:
    QSplitter* splitter;
    CommentsPlot* plot;

    QPointer<FilterSelector> m_filterSelectors[2];

    void handleFilterChange(FilterSelector* filterSelector, int pos);

protected:
    bool eventFilter(QObject *, QEvent *);

Q_SIGNALS:
	void rewind(int pos);

private Q_SLOTS:
    void onCursorMoved(int x);

public Q_SLOTS:
    void updateImagesAndSlider(const QPixmap& pixmap1, const QPixmap& pixmap2, int sliderPos);
    void onSliderValueChanged(int value);

    void on_Full_triggered();
};

#endif // GUI_BigDisplay_H

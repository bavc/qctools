/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QByteArray>
#include <QMutex>

#include <vector>
using namespace std;

#include "Core/Core.h"
#include "GUI/FileInformation.h"
#include "GUI/TinyDisplay.h"
#include "GUI/Control.h"
#include "GUI/Info.h"
#include "GUI/FilesList.h"

namespace Ui {
class MainWindow;
}

class Plots;
class QPixmap;
class QLabel;
class QToolButton;
class QDropEvent;
class QDragEnterEvent;
class QPushButton;
class QComboBox;
class QCheckBox;

class PerPicture;
class Preferences;

class BlackmagicDeckLink_Glue;
class DraggableChildrenBehaviour;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //Functions
    void dragEnterEvent         (QDragEnterEvent *event);
    void dropEvent              (QDropEvent *event);

    // UI
    void                        Ui_Init                     ();
    void                        configureZoom               ();
    void                        openFile                    ();
    void                        openCapture                 ();
    void                        closeFile                   ();
    void                        closeAllFiles               ();
    void                        Zoom_Move                   (size_t Begin);
    void                        Zoom_In                     ();
    void                        Zoom_Out                    ();
    void                        Export_CSV                  ();
    void                        Export_PDF                  ();
    void                        refreshDisplay              ();
    void                        Options_Preferences         ();
    void                        Help_GettingStarted         ();
    void                        Help_HowToUse               ();
    void                        Help_FilterDescriptions     ();
    void                        Help_PlaybackFilters        ();
    void                        Help_DataFormat             ();
    void                        Help_About                  ();

    // Helpers
    void                        processFile                 (const QString &FileName);
    void                        clearFiles                  ();
    void                        clearDragDrop               ();
    void                        createDragDrop              ();
    void                        clearFilesList              ();
    void                        createFilesList             ();
    void                        clearGraphsLayout           ();
    void                        createGraphsLayout          ();
    void                        addFile                     (const QString &FileName);
    void                        addFile                     (BlackmagicDeckLink_Glue* BlackmagicDeckLink_Glue, int FrameCount, const std::string &Encoding_FileName=std::string(), const std::string &Encoding_Format=std::string());
    void                        addFile_finish              ();
    void                        selectFile                  (int newFilePos);
    void                        selectDisplayFile           (int newFilePos);
    void                        selectDisplayFiltersFile    (int newFilePos);

    // Visual elements
    FilesList*                  FilesListArea;
    Plots*                      PlotsArea;
    TinyDisplay*                TinyDisplayArea;
    Control*                    ControlArea;
    Info*                       InfoArea;
    QLabel*                     DragDrop_Image;
    QLabel*                     DragDrop_Text;

    //CheckBoxes
    std::vector<QCheckBox*>     CheckBoxes[Type_Max];

    // Files
    std::vector<FileInformation*> Files;
    size_t                      Files_CurrentPos;
    size_t                      Thumbnails_Modulo;

    // Deck
    bool                        DeckRunning;

    //Preferences
    Preferences*                Prefs;

    QList<std::tuple<int, int> > getFilterSelectorsOrder(int start, int end);
public Q_SLOTS:
	void Update();

private Q_SLOTS:

    void TimeOut();
    void TimeOut_Refresh();

    void on_actionQuit_triggered();

    void on_actionOpen_triggered();

    void on_actionBlackmagicDeckLinkCapture_triggered();

    void on_actionClose_triggered();

    void on_actionCloseAll_triggered();

    void on_horizontalScrollBar_valueChanged(int value);

    void on_actionZoomIn_triggered();

    void on_actionZoomOut_triggered();

    void on_actionGoTo_triggered();

    void on_actionToolbar_triggered();

    void on_Toolbar_visibilityChanged(bool visible);

    void on_actionImport_XmlGz_Prompt_triggered();

    void on_actionExport_XmlGz_Prompt_triggered();

    void on_actionExport_XmlGz_Sidecar_triggered();

    void on_actionExport_XmlGz_SidecarAll_triggered();

    void on_actionExport_XmlGz_Custom_triggered();

    void on_actionCSV_triggered();

    void on_actionPrint_triggered();

    void on_actionFilesList_triggered();

    void on_actionGraphsLayout_triggered();

    void on_actionFiltersLayout_triggered();

    void on_actionPreferences_triggered();

    void on_actionGettingStarted_triggered();

    void on_actionHowToUseThisTool_triggered();

    void on_actionFilterDescriptions_triggered();

    void on_actionPlaybackFilters_triggered();

    void on_actionDataFormat_triggered();

    void on_actionAbout_triggered();

    void on_fileNamesBox_currentIndexChanged(int index);

    void on_check_toggled(bool checked);

    void on_M1_triggered();

    void on_Minus_triggered();

    void on_PlayPause_triggered();

    void on_Pause_triggered();

    void on_Plus_triggered();

    void on_P1_triggered();

    void on_Full_triggered();

    void on_CurrentFrameChanged();


    void on_actionZoomOne_triggered();

    void on_actionPlay_at_Frame_Rate_triggered();

    void on_actionPlay_All_Frames_triggered();

    void positionChanged(QWidget* child, int oldPos, int newPos);

private:
    void updateScrollBar( bool blockSignals = false );
    bool isPlotZoomable() const;
    void Zoom( bool );
    void changeFilterSelectorsOrder(QList<std::tuple<int, int> > filtersInfo);

    DraggableChildrenBehaviour* draggableBehaviour;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

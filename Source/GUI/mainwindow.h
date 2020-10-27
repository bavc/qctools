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
#include <QPushButton>
#include <QJsonDocument>
#include <QPointer>

#include <vector>
using namespace std;

#include "Core/Core.h"
#include "Core/FileInformation.h"
#include "Core/SignalServerConnectionChecker.h"
#include "GUI/TinyDisplay.h"
#include "GUI/Info.h"
#include "GUI/FilesList.h"
#include "GUI/plotschooser.h"

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
class PreferencesDialog;

class DraggableChildrenBehaviour;
class SignalServer;
class Preferences;
class Player;

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
    bool                        canCloseFile                (size_t index);
    void                        closeFile                   ();
    void                        closeAllFiles               ();
    void                        Zoom_Move                   (size_t Begin);
    void                        Zoom_In                     ();
    void                        Zoom_Out                    ();
    void                        Export_PDF                  ();
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
    void                        addFile_finish              ();
    void                        selectFile                  (int newFilePos);
    void                        selectDisplayFile           (int newFilePos);
    void                        selectDisplayFiltersFile    (int newFilePos);

    // Visual elements
    FilesList*                  FilesListArea;
    Plots*                      PlotsArea;
    TinyDisplay*                TinyDisplayArea;
    Info*                       InfoArea;
    QLabel*                     DragDrop_Image;
    QLabel*                     DragDrop_Text;

    QPointer<PlotsChooser> m_plotsChooser;

    // Files
    std::vector<FileInformation*> Files;
    size_t                      Thumbnails_Modulo;

    // Deck
    bool                        DeckRunning;

    //Preferences
    PreferencesDialog*          Prefs;
    Preferences*                preferences;
    
    QList<QAction*>             recentFilesActions;

    SignalServer*               getSignalServer();
    QStringList                 getSelectedFilters() const;

    QAction* uploadAction() const;
    QAction* uploadAllAction() const;

    size_t getFilesCurrentPos() const;
    FileInformation* getCurrenFileInformation() const;

    void setFilesCurrentPos(const size_t &value);
    bool isFileSelected() const;
    bool isFileSelected(size_t pos) const;

    bool hasMediaFile() const;

Q_SIGNALS:
    void fileSelected(bool selected);
    void filePositionChanged(size_t filePosition);

public Q_SLOTS:
	void Update();
    void applyBarchartsProfile();
    void loadBarchartsProfile(const QString& profile);
    void saveBarchartsProfile(const QString& profile);
    void setPlotVisible(quint64 group, quint64 type, bool visible);

private Q_SLOTS:

    void TimeOut();

    void on_actionQuit_triggered();

    void on_actionOpen_triggered();

    void on_actionClose_triggered();

    void on_actionCloseAll_triggered();

    void on_horizontalScrollBar_valueChanged(int value);

    void on_actionZoomIn_triggered();

    void on_actionZoomOut_triggered();

    void on_actionGoTo_triggered();

    void on_actionToolbar_triggered();

    void on_Toolbar_visibilityChanged(bool visible);

    void on_actionExport_Mkv_Prompt_triggered();

    void on_actionExport_Mkv_Sidecar_triggered();

    void on_actionExport_Mkv_SidecarAll_triggered();

    void on_actionExport_Mkv_QCvault_triggered();

    void on_actionExport_Mkv_QCvaultAll_triggered();

    void on_actionExport_XmlGz_Prompt_triggered();

    void on_actionExport_XmlGz_Sidecar_triggered();

    void on_actionExport_XmlGz_SidecarAll_triggered();

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

    void on_Full_triggered();

    void on_CurrentFrameChanged();


    void on_actionZoomOne_triggered();

    void on_actionUploadToSignalServer_triggered();
    void on_actionUploadToSignalServerAll_triggered();

    void onSignalServerConnectionChanged(SignalServerConnectionChecker::State state);
    void updateConnectionIndicator();
    void updateSignalServerSettings();

    void updateSignalServerCheckUploadedStatus();
    void updateSignalServerUploadStatus();
    void updateSignalServerUploadProgress(qint64, qint64);

    void updateExportActions();
    void updateExportAllAction();
    void showPlayer();

    void on_actionNavigateNextComment_triggered();

    void on_actionNavigatePreviousComment_triggered();

    void openRecentFile();

    void on_actionClear_Recent_History_triggered();

    void on_actionReveal_file_location_triggered();

    void on_actionGrab_frame_triggered();

    void on_actionGrab_plots_image_triggered();

    void on_actionShow_hide_debug_panel_triggered();

    void on_actionShow_hide_filters_panel_triggered();

    void on_copyToClipboard_pushButton_clicked();

    void on_setupFilters_pushButton_clicked();

protected:
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);
    void moveEvent(QMoveEvent *event);

private:
    void updateScrollBar( bool blockSignals = false );
    bool isPlotZoomable() const;
    void Zoom( bool );

    QAction* createOpenRecentAction(const QString& fileName);
    void updateRecentFiles(const QString& fileName);

    SignalServer* signalServer;
    SignalServerConnectionChecker* connectionChecker;
    QWidget* connectionIndicator;
    size_t files_CurrentPos;

    QJsonDocument m_barchartsProfile;
    QComboBox* m_profileSelectorCombobox;
    Player* m_player;
    QTimer m_playbackSimulationTimer;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

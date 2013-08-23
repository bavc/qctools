#ifndef PERPICTURE_H
#define PERPICTURE_H

#include "Core/Core.h"

#include <QDialog>
#include <QProcess>
#include <QTemporaryDir>

class QLabel;
class QPixmap;
class ffmpeg_Pictures;
class PerFile;

namespace Ui {
class PerPicture;
}

class PerPicture : public QDialog
{
    Q_OBJECT
    
public:
    explicit PerPicture(QWidget *parent = 0);
    ~PerPicture();

    void ShowPicture(size_t PicturePos, double** y, QString FileName, PerFile* Source);

    void Load_Update();
    void Load_Finished();

    QLabel* Values[PlotName_Max];
    
    // ffmpeg
    QProcess*       Process;
    size_t          PicturePos_Current;
    size_t          PicturePos_Computing;
    size_t          PicturePos_Computing_Base;
    QPixmap*        PicturePos_Pixmaps[1+100*2];
    double**        y_Current;
    QString         FileName_Current;
    QTemporaryDir   TempDir;

    ffmpeg_Pictures*            Pictures[2];
    size_t                      Pictures_Current;
    size_t                      PicturePos;

private:
    Ui::PerPicture *ui;

private Q_SLOTS:
    void ProcessMessage();
    void ProcessError();
    void ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_Keep_clicked(bool checked);
};

#endif // PERPICTURE_H

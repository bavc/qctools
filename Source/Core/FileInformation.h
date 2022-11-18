/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_FileInformation_H
#define GUI_FileInformation_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/Core.h"
#include "Core/SignalServer.h"

#include <string>

#include <QThread>
#include <QFile>
#include <QSharedPointer>
#include <QFileInfo>
#include <QSize>
#include <QPixmap>
#include <map>
#include <string>

class QAVVideoFrame;
class CommonStats;
class StreamsStats;
class FormatStats;
class FFmpeg_Glue;

typedef QSharedPointer<QFile> SharedFile;
class QAVPlayer;

//---------------------------------------------------------------------------
class FileInformation : public QThread
{
    //thread part
    Q_OBJECT
    void run();
    void runParse();
    void runExport();

public:
    enum JobTypes
    {
        Parsing,
        Exporting
    };

    JobTypes jobType() const;

    // Constructor/Destructor
                                FileInformation             (SignalServer* signalServer, const QString &fileName,
                                                             activefilters ActiveFilters, activealltracks ActiveAllTracks,
                                                             QMap<QString, std::tuple<QString, QString, QString, QString, int>> activePanels,
                                                             const QString &cacheFileNamePrefix,
                                                             int FrameCount=0);
                                ~FileInformation            ();

    // Parsing
    void startParse();
    void startExport(const QString& exportFileName = QString());

    // Dumps
    void                        Export_XmlGz                (const QString &ExportFileName, const activefilters& filters);
    void                        Export_QCTools_Mkv          (const QString &ExportFileName, const activefilters& filters);

    // Infos
    QByteArray Picture_Get (size_t Pos);
    QPixmap getThumbnail(size_t pos);
    QString	fileName() const;

    activefilters               ActiveFilters;
    activealltracks             ActiveAllTracks;

    size_t                      ReferenceStream_Pos_Get     () const {return ReferenceStream_Pos;}
    int                         Frames_Count_Get            (size_t Stats_Pos=(size_t)-1);
    int                         Frames_Pos_Get              (size_t Stats_Pos=(size_t)-1);
    QString                     Frame_Type_Get              (size_t Stats_Pos=(size_t)-1, size_t frameIndex = (size_t)-1) const;
    void                        Frames_Pos_Set              (int Frames_Pos, size_t Stats_Pos=(size_t)-1);
    void                        Frames_Pos_Minus            ();
    bool                        Frames_Pos_Plus             ();
    bool                        Frames_Pos_AtEnd            ();
    bool                        PlayBackFilters_Available   ();
    size_t                      VideoFrameCount_Get         ();
    bool                        IsRGB_Get                   ();

    qreal                       averageFrameRate        () const;
    double                      TimeStampOfCurrentFrame () const;


    bool isValid() const;

    // FFmpeg glue
    FFmpeg_Glue*                Glue { nullptr };

    std::vector<CommonStats*>   Stats;

    StreamsStats*               streamsStats;
    FormatStats*                formatStats;
    CommonStats*                ReferenceStat               () const {if (ReferenceStream_Pos<Stats.size()) return Stats[ReferenceStream_Pos]; else return NULL;}

    int                         BitsPerRawSample            (int streamType = Type_Video) const;
    int audioSampleFormat() const;
    QPair<int, int> audioRanges() const;

    enum SignalServerCheckUploadedStatus {
        NotChecked,
        Checking,
        Uploaded,
        NotUploaded,
        CheckError
    };

    Q_ENUM(SignalServerCheckUploadedStatus);

    SignalServerCheckUploadedStatus signalServerCheckUploadedStatus() const;
    QString signalServerCheckUploadedStatusString() const;
    QString signalServerCheckUploadedStatusErrorString() const;

    enum SignalServerUploadStatus {
        Idle,
        Uploading,
        Done,
        UploadError
    };

    Q_ENUM(SignalServerUploadStatus);

    SignalServerUploadStatus signalServerUploadStatus() const;
    QString signalServerUploadStatusString() const;
    QString signalServerUploadStatusErrorString() const;

    bool hasStats() const;

    void setAutoCheckFileUploaded(bool enable);
    void setAutoUpload(bool enable);

    // index in FileList
    int index() const;
    void setIndex(int value);
    bool parsed() const;

    void setExportFilters(const activefilters& exportFilters);
    bool commentsUpdated() const;

    void readStats(QIODevice& StatsFromExternalData_FileName, bool StatsFromExternalData_FileName_IsCompressed);

    QSize panelSize() const;
    const QMap<std::string, QVector<int>>& panelOutputsByTitle() const;
    const std::map<std::string, std::string> & getPanelOutputMetadata(size_t index) const;
    size_t getPanelFramesCount(size_t index) const;
    QAVVideoFrame getPanelFrame(size_t index, size_t panelFrameIndex) const;

public Q_SLOTS:

    void checkFileUploaded(const QString& fileName);
    void upload(const QFileInfo& fileInfo);
    void upload(SharedFile file, const QString& fileName);
    void cancelUpload();
    void setCommentsUpdated(CommonStats* stats);

Q_SIGNALS:
    void positionChanged();
    void statsFileGenerated(SharedFile statsFile, const QString& name);
    void statsFileGenerationProgress(quint64 bytesWritten, quint64 totalBytes);

    void statsFileLoaded(SharedFile statsFile);
    void parsingCompleted(bool success);

    void signalServerCheckUploadedStatusChanged();
    void signalServerUploadProgressChanged(qint64, qint64);
    void signalServerUploadStatusChanged();

    void commentsUpdated(CommonStats* stats);
private Q_SLOTS:
    void checkFileUploadedDone();
    void uploadDone();
    void parsingDone(bool success);
    void handleAutoUpload();

private:
    JobTypes m_jobType;

    QString                     FileName;
    size_t                      ReferenceStream_Pos;
    int                         Frames_Pos {0};
    int                         AudioFrames_Pos {0};

    // FFmpeg part
    bool                        WantToStop;

    SignalServer* signalServer;
    QSharedPointer<CheckFileUploadedOperation> checkFileUploadedOperation;
    QSharedPointer<UploadFileOperation> uploadOperation;

    int m_index;
    QString m_exportFileName;
    std::atomic<bool> m_parsed;

    bool m_autoCheckFileUploaded;
    bool m_autoUpload;
    bool m_hasStats;
    bool m_commentsUpdated;
    QSize m_panelSize;

    activefilters m_exportFilters;

    QMap<std::string, QVector<int>> m_panelOutputsByTitle;
    QVector<std::map<std::string, std::string>> m_panelMetadata;
    QVector<QVector<QAVVideoFrame>> m_panelFrames;

    std::vector<QByteArray> m_thumbnails;
    std::vector<QPixmap> m_thumbnails_pixmap;

    QAVPlayer* m_mediaParser;
};

#endif // GUI_FileInformation_H

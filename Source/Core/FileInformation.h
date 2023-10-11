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
#include <map>
#include <string>

class QAVVideoFrame;
class CommonStats;
class StreamsStats;
class FormatStats;

typedef QSharedPointer<QFile> SharedFile;
class QAVPlayer;

std::string FFmpeg_Version();
int FFmpeg_Year();
std::string FFmpeg_Compiler();
std::string FFmpeg_Configuration();
std::string FFmpeg_LibsVersion();

bool isDpx(QString mediaFileName);
QString adjustDpxFileName(QString mediaFileName, int& dpxOffset);

//---------------------------------------------------------------------------
class FileInformation : public QThread
{
    //thread part
    Q_OBJECT
    void run();
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
    void makeMkvReport(QString exportFileName, QByteArray attachment, QString attachmentFileName, const std::function<void(int, int)>& thumbnailsCallback = {}, const std::function<void(int, int)>& panelsCallback = {});

    size_t thumbnailsCount();
    // Infos
    QAVVideoFrame getThumbnail(size_t pos);
    QString	fileName() const;

    // extracted from FFMpeg_Glue
    int width() const;
    int height() const;
    int bitsPerRawSample() const;
    double dar() const;
    std::string pixFormatName() const;
    int isRgbSet() const;

    std::string containerFormat;
    int streamCount { 0 };
    int bitRate { 0 };

    double duration() const;
    std::string videoFormat() const;
    std::string fieldOrder() const;
    std::string sar() const;

    struct FrameRate {
        FrameRate(int  num, int den) : num(num), den(den) {}

        int num;
        int den;

        double value() {
            return double(num) / den;
        }

        bool isValid() const {
            return num > 0 && den > 0;
        }
    };

    FrameRate getAvgVideoFrameRate() const;
    double framesDivDuration() const;
    std::string rvideoFrameRate() const;
    std::string avgVideoFrameRate() const;
    std::string colorSpace() const;
    std::string colorRange() const;
    std::string audioFormat() const;
    std::string sampleFormat() const;
    double samplingRate() const;
    std::string channelLayout() const;
    double abitDepth() const;

    bool hasVideoStreams() const;
    bool hasAudioStreams() const;

    activefilters               ActiveFilters;
    activealltracks             ActiveAllTracks;

    size_t                      ReferenceStream_Pos_Get     () const {return ReferenceStream_Pos;}
    int                         Frames_Count_Get            (size_t Stats_Pos=(size_t)-1) const;
    int                         Frames_Pos_Get              (size_t Stats_Pos=(size_t)-1);
    QString                     Frame_Type_Get              (size_t Stats_Pos=(size_t)-1, size_t frameIndex = (size_t)-1) const;
    void                        Frames_Pos_Set              (int Frames_Pos, size_t Stats_Pos=(size_t)-1);
    void                        Frames_Pos_Minus            ();
    bool                        Frames_Pos_Plus             ();
    bool                        Frames_Pos_AtEnd            ();
    bool                        PlayBackFilters_Available   ();
    size_t                      VideoFrameCount_Get         ();

    qreal                       averageFrameRate        () const;
    double                      TimeStampOfCurrentFrame () const;


    bool isValid() const;

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
    size_t                      ReferenceStream_Pos {0};
    int                         Frames_Pos {0};

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

    std::vector<QAVVideoFrame> m_thumbnails_frames;

    QAVPlayer* m_mediaParser { nullptr };
    QAVPlayer* m_mediaPlayer { nullptr };
};

#endif // GUI_FileInformation_H

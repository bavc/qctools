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
class CommonStats;
class StreamsStats;
class FormatStats;
class FFmpeg_Glue;

#ifdef BLACKMAGICDECKLINK_YES
class BlackmagicDeckLink_Glue;
#endif // BLACKMAGICDECKLINK_YES

typedef QSharedPointer<QFile> SharedFile;

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
                                FileInformation             (SignalServer* signalServer, const QString &FileName, activefilters ActiveFilters, activealltracks ActiveAllTracks,
#ifdef BLACKMAGICDECKLINK_YES
                                                             BlackmagicDeckLink_Glue* blackmagicDeckLink_Glue=NULL,
#endif //BLACKMAGICDECKLINK_YES
                                                             int FrameCount=0, const std::string &Encoding_FileName=std::string(), const std::string &Encoding_Format=std::string());
                                ~FileInformation            ();

    // Parsing
    void startParse();
    void startExport(const QString& exportFileName = QString());

    // Dumps
    void                        Export_XmlGz                (const QString &ExportFileName, const activefilters& filters);
    void                        Export_CSV                  (const QString &ExportFileName);

    // Infos
    QByteArray Picture_Get (size_t Pos);
    QString	fileName() const;

    activefilters               ActiveFilters;
    activealltracks             ActiveAllTracks;
    size_t                      ReferenceStream_Pos_Get     () const {return ReferenceStream_Pos;}
    int                         Frames_Count_Get            (size_t Stats_Pos=(size_t)-1);
    int                         Frames_Pos_Get              (size_t Stats_Pos=(size_t)-1);
    QString                     Frame_Type_Get              (size_t Stats_Pos=(size_t)-1, size_t frameIndex = (size_t)-1) const;
    void                        Frames_Pos_Set              (int Frames_Pos, size_t Stats_Pos=(size_t)-1);
    void                        Frames_Pos_Minus            ();
    void                        Frames_Pos_Plus             ();
    bool                        PlayBackFilters_Available   ();

    qreal                       averageFrameRate        () const;

#ifdef BLACKMAGICDECKLINK_YES
    // Deck control information
    BlackmagicDeckLink_Glue*    blackmagicDeckLink_Glue;
#endif //BLACKMAGICDECKLINK_YES

    bool isValid() const;

    // FFmpeg glue
    FFmpeg_Glue*                Glue;

    std::vector<CommonStats*>   Stats;

    StreamsStats*               streamsStats;
    FormatStats*                formatStats;
    CommonStats*                ReferenceStat               () const {if (ReferenceStream_Pos<Stats.size()) return Stats[ReferenceStream_Pos]; else return NULL;}

    int                         BitsPerRawSample            () const;

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

public Q_SLOTS:

    void checkFileUploaded(const QString& fileName);
    void upload(const QFileInfo& fileInfo);
    void upload(SharedFile file, const QString& fileName);
    void cancelUpload();

Q_SIGNALS:
    void positionChanged();
    void statsFileGenerated(SharedFile statsFile, const QString& name);
    void statsFileGenerationProgress(quint64 bytesWritten, quint64 totalBytes);

    void statsFileLoaded(SharedFile statsFile);
    void parsingCompleted(bool success);

    void signalServerCheckUploadedStatusChanged();
    void signalServerUploadProgressChanged(qint64, qint64);
    void signalServerUploadStatusChanged();

private Q_SLOTS:
    void checkFileUploadedDone();
    void uploadDone();
    void parsingDone(bool success);
    void handleAutoUpload();

private:
    JobTypes m_jobType;

    QString                     FileName;
    size_t                      ReferenceStream_Pos;
    int                         Frames_Pos;

    // FFmpeg part
    bool                        WantToStop;

    SignalServer* signalServer;
    QSharedPointer<CheckFileUploadedOperation> checkFileUploadedOperation;
    QSharedPointer<UploadFileOperation> uploadOperation;

    int m_index;
    QString m_exportFileName;
    bool m_parsed;

    bool m_autoCheckFileUploaded;
    bool m_autoUpload;
    bool m_hasStats;

    activefilters m_exportFilters;
};

#endif // GUI_FileInformation_H

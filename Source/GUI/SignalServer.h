#ifndef SIGNALSERVER_H
#define SIGNALSERVER_H

#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

class SignalServerOperation : public QObject
{
    Q_OBJECT
public:
    SignalServerOperation(const QString& fileName, QSharedPointer<QNetworkReply> reply);

    QString errorString() const;
    QString fileName() const;

Q_SIGNALS:
    void finished();

public Q_SLOTS:
    void cancel();

protected Q_SLOTS:
    virtual void onFinished() = 0;

protected:
    QString m_errorString;
    QString m_fileName;
    QSharedPointer<QNetworkReply> m_reply;
};

class CheckFileUploadedOperation : public SignalServerOperation
{
public:
    enum State
    {
        Unknown,
        Uploaded,
        NotUploaded,
        Error
    };

    State state() const;

    CheckFileUploadedOperation(const QString& fileName, QSharedPointer<QNetworkReply> reply);

protected:
    virtual void onFinished();

private:
    State m_state;
};

class UploadFileOperation : public SignalServerOperation
{
    Q_OBJECT

public:
    enum State
    {
        Uploading,
        Uploaded,
        Error
    };

    State state() const;

    UploadFileOperation(const QString& fileName, QSharedPointer<QIODevice> data, QSharedPointer<QNetworkReply> reply);

Q_SIGNALS:
    void uploadProgress(qint64, qint64);

protected:
    virtual void onFinished();

private:
    QSharedPointer<QIODevice> m_data;
    State m_state;
};

class SignalServer : public QObject
{
    Q_OBJECT
public:
    explicit SignalServer(QObject *parent = 0);

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString login() const;
    void setLogin(const QString &login);

    QString password() const;
    void setPassword(const QString &password);

    QSharedPointer<CheckFileUploadedOperation> checkFileUploaded(const QString& fileName);
    QSharedPointer<UploadFileOperation> uploadFile(const QString& fileName, QSharedPointer<QIODevice> data);

private:
    QSharedPointer<QNetworkReply> get(const QUrl& request);
    QSharedPointer<QNetworkReply> put(const QUrl& request, QIODevice* device);

private:
    QUrl m_url;
    QString m_login;
    QString m_password;

    QNetworkAccessManager m_manager;
};

#endif // SIGNALSERVER_H

#include "SignalServer.h"

SignalServer::SignalServer(QObject *parent) : QObject(parent)
{

}

QUrl SignalServer::url() const
{
    return m_url;
}

void SignalServer::setUrl(const QUrl &url)
{
    m_url = url;
}

QString SignalServer::login() const
{
    return m_login;
}

void SignalServer::setLogin(const QString &login)
{
    m_login = login;
}

QString SignalServer::password() const
{
    return m_password;
}

void SignalServer::setPassword(const QString &password)
{
    m_password = password;
}

QSharedPointer<CheckFileUploadedOperation> SignalServer::checkFileUploaded(const QString &fileName)
{
    QUrl checkFileUploadedUrl = QUrl(m_url.toString() + "/fileuploads/check_exist/" + QUrl::toPercentEncoding(fileName));
    QSharedPointer<QNetworkReply> reply = get(checkFileUploadedUrl);

    return QSharedPointer<CheckFileUploadedOperation>::create(fileName, reply);
}

QSharedPointer<UploadFileOperation> SignalServer::uploadFile(const QString &fileName, QSharedPointer<QIODevice> data)
{
    QUrl uploadFileUrl = QUrl(m_url.toString() + "/fileuploads/upload/" + QUrl::toPercentEncoding(fileName));
    QSharedPointer<QNetworkReply> reply = put(uploadFileUrl, data.data());

    return QSharedPointer<UploadFileOperation>::create(fileName, data, reply);
}

QSharedPointer<QNetworkReply> SignalServer::get(const QUrl& url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2")
                                                                .arg(m_login)
                                                                .arg(m_password).toLocal8Bit().toBase64()));

    return QSharedPointer<QNetworkReply>(m_manager.get(request), &QObject::deleteLater);
}

QSharedPointer<QNetworkReply> SignalServer::put(const QUrl &url, QIODevice* device)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2")
                                                                .arg(m_login)
                                                                .arg(m_password).toLocal8Bit().toBase64()));

    return QSharedPointer<QNetworkReply>(m_manager.put(request, device), &QObject::deleteLater);
}

SignalServerOperation::SignalServerOperation(const QString &fileName, QSharedPointer<QNetworkReply> reply) : m_fileName(fileName), m_reply(reply)
{
    connect(reply.data(), SIGNAL(finished()), this, SLOT(onFinished()));
}

QString SignalServerOperation::errorString() const
{
    return m_errorString;
}

QString SignalServerOperation::fileName() const
{
    return m_fileName;
}

void SignalServerOperation::cancel()
{
    if(m_reply)
        m_reply->abort();
}

CheckFileUploadedOperation::CheckFileUploadedOperation(const QString& fileName, QSharedPointer<QNetworkReply> reply) : SignalServerOperation(fileName, reply), m_state(Unknown)
{
}

CheckFileUploadedOperation::State CheckFileUploadedOperation::state() const
{
    return m_state;
}

void CheckFileUploadedOperation::onFinished()
{
    if(m_reply->error() == QNetworkReply::NoError)
    {
        int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray data = m_reply->readAll();
        if(statusCode != 200)
        {
            m_state = Error;
            m_errorString = QString("Failure: statusCode = %1").arg(statusCode);
        } else {
            m_state = data == "true" ? Uploaded : NotUploaded;
            m_errorString.clear();
        }
    }
    else
    {
        m_state = Error;
        m_errorString = m_reply->errorString();
    }

    Q_EMIT finished();
}

UploadFileOperation::UploadFileOperation(const QString &fileName, QSharedPointer<QIODevice> data, QSharedPointer<QNetworkReply> reply)
    : SignalServerOperation(fileName, reply), m_data(data), m_state(Uploading)
{
    connect(reply.data(), SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));
}

UploadFileOperation::State UploadFileOperation::state() const
{
    return m_state;
}

void UploadFileOperation::onFinished()
{
    if(m_reply->error() == QNetworkReply::NoError)
    {
        int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 204) // at the moment that's what backend replies if upload successfull
        {
            m_state = Uploaded;
            m_errorString.clear();
        }
        else
        {
            m_state = Error;
            m_errorString = QString("Failure: statusCode = %1").arg(statusCode);
        }
    }
    else
    {
        m_state = Error;
        m_errorString = m_reply->errorString();
    }

    Q_EMIT finished();
}

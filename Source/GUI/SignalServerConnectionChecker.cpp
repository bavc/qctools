#include "SignalServerConnectionChecker.h"
#include <QNetworkReply>
#include <cassert>
#include <limits.h>

SignalServerConnectionChecker::SignalServerConnectionChecker(QObject *parent) : QObject(parent), m_state(NotChecked), m_running(false), m_reply(NULL)
{
    qRegisterMetaType<SignalServerConnectionChecker::State>("SignalServerConnectionChecker::State");

    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(checkConnectionDone()));

    m_retryTimer.setInterval(5 * 100);
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
}

SignalServerConnectionChecker::State SignalServerConnectionChecker::state() const
{
    return m_state;
}

QString SignalServerConnectionChecker::errorString() const
{
    return m_errorString;
}

bool SignalServerConnectionChecker::isRunning() const
{
    return m_running;
}

void SignalServerConnectionChecker::start(const QUrl &url, const QString &login, const QString &password)
{
	m_url = url;
	m_login = login;
	m_password = password;

    if(m_running)
        return;

    m_running = true;

    checkConnection();
}

void SignalServerConnectionChecker::stop()
{
    if(!m_running)
        return;

    m_running = false;
    m_retryTimer.stop();

    abortCheckConnection();

    changeState(NotChecked);
}

void SignalServerConnectionChecker::checkConnection(const QUrl &url, const QString &login, const QString &password, bool abortPendingCheck /* true */)
{
	if (abortPendingCheck)
		abortCheckConnection();
	else if (m_reply != NULL) /* check pending, do nothing for now */
		return;

	QString checkConnectionUrl = url.toString() + "/fileuploads/upload/test";
	qDebug() << "checkConnectionUrl: " << checkConnectionUrl;

    QNetworkRequest request(checkConnectionUrl);
    request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2")
                             .arg(login)
                             .arg(password).toLocal8Bit().toBase64()));

    QByteArray test(1, 0);
    m_reply = m_networkManager.put(request, test);
    connect(m_reply, SIGNAL(finished()), this, SLOT(checkConnectionDone()));

    m_timeoutTimer.start(5000);
}

void SignalServerConnectionChecker::checkConnection()
{
    checkConnection(m_url, m_login, m_password, false);
}

void SignalServerConnectionChecker::abortCheckConnection()
{
    if (m_reply)
    {
		m_timeoutTimer.stop();

		disconnect(m_reply, SIGNAL(finished()), this, SLOT(checkConnectionDone()));
        m_reply->abort();
        m_reply->deleteLater();

		qDebug() << "aborted!";
    }

    m_reply = NULL;
}

void SignalServerConnectionChecker::changeState(SignalServerConnectionChecker::State newState)
{
    if(newState != m_state)
    {
        m_state = newState;
        Q_EMIT connectionStateChanged(newState);
    }
}

void SignalServerConnectionChecker::checkConnectionDone()
{
    assert(m_reply);
    m_errorString.clear();

    if(!m_timeoutTimer.isActive())
    {
        changeState(Timeout);
		qDebug() << Timeout;

        disconnect(m_reply, SIGNAL(finished()), this, SLOT(checkConnectionDone()));
        m_reply->abort();
    }
    else
    {
		m_timeoutTimer.stop();
		
		if(m_reply->error() == QNetworkReply::NoError)
        {
            int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 204) // at the moment that's what backend replies if upload successfull
            {
                changeState(Online);
            }
            else
            {
                m_errorString = QString("Failure: statusCode = %0").arg(statusCode);
                changeState(Error);

                qDebug() << m_reply->readAll() << ", status: " << m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            }
        }
        else
        {
            m_errorString = QString("%0").arg(m_reply->errorString());
            changeState(Error);

            qDebug() << "error: " << m_reply->errorString();
        }
    }

    m_reply->deleteLater();
    m_reply = NULL;

    Q_EMIT done();

    if(m_running)
        m_retryTimer.start();
}

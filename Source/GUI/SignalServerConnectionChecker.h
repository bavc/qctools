#ifndef SIGNALSERVERCONNECTIONCHECKER_H
#define SIGNALSERVERCONNECTIONCHECKER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

class SignalServerConnectionChecker : public QObject
{
    Q_OBJECT
public:
    enum State {
        NotChecked,
        Online,
        Timeout,
        Error
    };

    explicit SignalServerConnectionChecker(QObject *parent = 0);

    State state() const;
    QString errorString() const;
    bool isRunning() const;

Q_SIGNALS:
    void connectionStateChanged(SignalServerConnectionChecker::State newState);
    void done();

public Q_SLOTS:
    void start(const QUrl& url, const QString& login, const QString& password);
    void stop();

    void checkConnection(const QUrl& url, const QString& login, const QString& password, bool abortPendingCheck = true);
    void checkConnection();

private Q_SLOTS:
    void checkConnectionDone();

private:
    void abortCheckConnection();
    void changeState(State newState);

    bool m_running;
    State m_state;
    QString m_errorString;

    QUrl m_url;
    QString m_login;
    QString m_password;

    QNetworkAccessManager m_networkManager;

    QTimer m_retryTimer;

    QTimer m_timeoutTimer;
    QNetworkReply* m_reply;
};

#endif // SIGNALSERVERCONNECTIONCHECKER_H

#include <QCoreApplication>
#include "cli.h"

// suppress debug output
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

int main(int argc, char *argv[])
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");

    qInstallMessageHandler(messageHandler);
    QCoreApplication a(argc, argv);

    Cli cli;
    return cli.exec(a);
}

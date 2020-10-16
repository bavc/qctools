#ifndef LOGGING_H
#define LOGGING_H

#include <QtGlobal>
#include <QDebug>
#include <QLoggingCategory>
#include <QElapsedTimer>

class Logging
{
public:
    Logging();
    ~Logging();

    void enable();

private:
    QtMessageHandler prevMessageHandler;
};

#endif // LOGGING_H

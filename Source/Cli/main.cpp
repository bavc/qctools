#include <QCoreApplication>
#include "Core/SignalServer.h"
#include "Core/FileInformation.h"
#include "Core/Preferences.h"
#include <memory>
#include <iostream>

enum Errors {
    Success = 0,
    NoInput = 1,
    ParsingFailure = 2,
    OutputAlreadyExists = 3
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString input;
    QString output;
    bool forceOutput = false;

    for(int i = 0; i < a.arguments().length(); ++i)
    {
        if(a.arguments().at(i) == "-i" && (i + 1) < a.arguments().length())
        {
            input = a.arguments().at(i + 1);
            ++i;
        } else if(a.arguments().at(i) == "-o" && (i + 1) < a.arguments().length())
        {
            output = a.arguments().at(i + 1);
            ++i;
        } else if(a.arguments().at(i) == "-y")
        {
            forceOutput = true;
        }
    }

    if(input.isEmpty())
        return NoInput;

    if(output.isEmpty())
        output = input + ".qctools.xml.gz";

    QFile file(output);
    if(file.exists() && !forceOutput)
    {
        std::cout << "file " << output.toStdString() << " already exists, exiting.. ";
        return OutputAlreadyExists;
    }

    Preferences prefs;

    std::unique_ptr<SignalServer> signalServer(new SignalServer());

    QString urlString = prefs.signalServerUrlString();
    if(!urlString.startsWith("http", Qt::CaseInsensitive))
        urlString.prepend("http://");

    QUrl url(urlString);

    signalServer->setUrl(url);
    signalServer->setLogin(prefs.signalServerLogin());
    signalServer->setPassword(prefs.signalServerPassword());
    signalServer->setAutoUpload(prefs.isSignalServerAutoUploadEnabled());

    if(file.exists() && forceOutput)
    {
        std::cout << "file " << output.toStdString() << " already exists and will be overwritten..." << std::endl;
        file.remove();
    }

    std::unique_ptr<FileInformation> info(new FileInformation(signalServer.get(), input, prefs.activeFilters(), prefs.activeAllTracks()));
    info->startParse();

    std::cout << "parsing... " << std::endl;
    QObject::connect(info.get(), SIGNAL(parsingCompleted(bool)), &a, SLOT(quit()));
    a.exec();

    std::cout << "parsing... " << (info->parsed() ? "succeed" : "failed") << std::endl;
    if(!info->parsed())
        return ParsingFailure;

    info->startExport(output);

    std::cout << "exporting... " << std::endl;
    QObject::connect(info.get(), SIGNAL(statsFileGenerated(SharedFile, const QString&)), &a, SLOT(quit()));
    a.exec();

    std::cout << "exporting... done" << std::endl;
    return 0;
}

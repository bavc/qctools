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
    OutputAlreadyExists = 3,
    InvalidInput = 4
};

// suppress debug output
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);
    QCoreApplication a(argc, argv);

    QString input;
    QString output;
    bool forceOutput = false;
    bool showHelp = false;

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
        } else if(a.arguments().at(i) == "-h")
        {
            showHelp = true;
        }
    }

    if(a.arguments().length() == 1)
        showHelp = true;

    if(showHelp)
    {
        std::cout <<
                     "QCTools $version, $copyright-summary" << std::endl <<
                     "Usage: qctools-cli -i <qctools-input> -o <qctools-output>" << std::endl << std::endl <<
                     "If no output file is declared, qctools will create an output named similarly to the input file with suffixed with \".qctools.xml.gz\"." << std::endl <<
                     "The filters used in qctools-cli may be declared via the qctools-gui (see the Preferences panel)." << std::endl << std::endl;
    }

    if(input.isEmpty())
        return NoInput;

    if(output.isEmpty())
        output = input + ".qctools.xml.gz";

    if(!output.endsWith(".xml.gz"))
    {
        std::cout << "warning: non-standard extension (not *.xml.gz) has been specified for output file. " << std::endl;
    }

    QFile file(output);
    if(file.exists() && !forceOutput)
    {
        std::cout << "file " << output.toStdString() << " already exists, exiting.. " << std::endl;
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
    info->setAutoCheckFileUploaded(false);
    info->setAutoUpload(false);

    std::cout << "analyzing input file... " << std::endl;

    if(!info->isValid())
    {
        std::cout << "invalid input, analyzing aborted.. " << std::endl;
        return InvalidInput;
    }

    info->startParse();
    QObject::connect(info.get(), SIGNAL(parsingCompleted(bool)), &a, SLOT(quit()));
    a.exec();

    std::cout << "analyzing... " << (info->parsed() ? "completed" : "failed") << std::endl;
    if(!info->parsed())
        return ParsingFailure;

    info->startExport(output);

    std::cout << "generating QCTools report... " << std::endl;
    QObject::connect(info.get(), SIGNAL(statsFileGenerated(SharedFile, const QString&)), &a, SLOT(quit()));
    a.exec();

    std::cout << "generating QCTools report... done" << std::endl;
    return 0;
}

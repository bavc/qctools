#include "cli.h"
#include "version.h"

Cli::Cli() : indexOfStreamWithKnownFrameCount(0), statsFileBytesWritten(0), statsFileBytesTotal(0)
{

}

int Cli::exec(QCoreApplication &a)
{
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
                     "QCTools " << (VERSION) << ", $copyright-summary" << std::endl <<
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

    info = unique_ptr<FileInformation>(new FileInformation(signalServer.get(), input, prefs.activeFilters(), prefs.activeAllTracks()));
    info->setAutoCheckFileUploaded(false);
    info->setAutoUpload(false);

    std::cout << std::endl << "analyzing input file... " << input.toStdString() << std::endl;

    if(!info->isValid())
    {
        std::cout << "invalid input, analyzing aborted.. " << std::endl;
        return InvalidInput;
    }

    std::vector<size_t> framesCountForAllStreams = info->Glue->FramesCountForAllStreams();
    size_t indexOfStreamWithKnownTotal = 0;

    for(size_t i = 0; i < framesCountForAllStreams.size(); ++i)
    {
        if(framesCountForAllStreams[i] > framesCountForAllStreams[indexOfStreamWithKnownTotal])
            indexOfStreamWithKnownTotal = framesCountForAllStreams[i];
    }

    // parse

    progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));
    QObject::connect(&progressTimer, SIGNAL(timeout()), this, SLOT(updateParsingProgress()));
    progressTimer.start(500);

    QObject::connect(info.get(), SIGNAL(parsingCompleted(bool)), &a, SLOT(quit()));
    info->startParse();
    a.exec();

    QObject::disconnect(&progressTimer, SIGNAL(timeout()), this, SLOT(updateParsingProgress()));
    if(info->parsed())
        progress->setValue(100);

    std::cout << std::endl << "analyzing " << (info->parsed() ? "completed" : "failed") << std::endl;

    if(!info->parsed())
        return ParsingFailure;

    // export
    std::cout << std::endl << "generating QCTools report... " << std::endl;

    progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));
    QObject::connect(&progressTimer, SIGNAL(timeout()), this, SLOT(updateExportingProgress()));
    progressTimer.start(500);

    QObject::connect(info.get(), SIGNAL(statsFileGenerationProgress(int, int)), this, SLOT(onStatsFileGenerationProgress(int, int)));
    QObject::connect(info.get(), SIGNAL(statsFileGenerated(SharedFile, const QString&)), &a, SLOT(quit()));
    info->startExport(output);
    a.exec();

    QObject::disconnect(&progressTimer, SIGNAL(timeout()), this, SLOT(updateExportingProgress()));

    std::cout << std::endl << "generating QCTools report... done" << std::endl;
    return 0;
}

void Cli::updateParsingProgress()
{
    int value = info->Glue->FramesProcessedPerStream(indexOfStreamWithKnownFrameCount) * progress->getMax() /
            info->Glue->FramesCountPerStream(indexOfStreamWithKnownFrameCount);

    progress->setValue(value);
}

void Cli::updateExportingProgress()
{
    if(statsFileBytesWritten != 0 && statsFileBytesTotal != 0)
    {
        int value = statsFileBytesWritten * progress->getMax() / statsFileBytesTotal;
        progress->setValue(value);
    }
}

void Cli::onStatsFileGenerationProgress(int written, int total)
{
    statsFileBytesWritten = written;
    statsFileBytesTotal = total;

    updateExportingProgress();
}

#include "cli.h"

int ProgressBar::getMax() const
{
    return max;
}

void ProgressBar::setMax(int value)
{
    max = value;
}

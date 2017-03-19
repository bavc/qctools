#include "cli.h"
#include "version.h"

Cli::Cli() : indexOfStreamWithKnownFrameCount(0), statsFileBytesWritten(0), statsFileBytesTotal(0), statsFileBytesUploaded(0), statsFileBytesToUpload(0)
{

}

int Cli::exec(QCoreApplication &a)
{
    QString input;
    QString output;
    QStringList filterStrings;
    bool forceOutput = false;
    bool showHelp = false;

    bool uploadToSignalServer = false;
    bool forceUploadToSignalServer = false;

    QString checkUploadFileName;

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
        } else if(a.arguments().at(i) == "-u")
        {
            uploadToSignalServer = true;
        } else if(a.arguments().at(i) == "-uf")
        {
            forceUploadToSignalServer = true;
        } else if(a.arguments().at(i) == "-c" && (i + 1) < a.arguments().length())
        {
            checkUploadFileName = a.arguments().at(i + 1);
            ++i;
        } else if(a.arguments().at(i) == "-f" && (i + 1) < a.arguments().length())
        {
            filterStrings = a.arguments().at(i + 1).split('+');
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

        std::cout << "other options: " << std::endl
                  << "\t"
                  << "-f - specify filter list as 'filter1;filter2;...;filterN'. If not specificed default fitlers will be used" << std::endl
                  << "\t\t" << "filters available: " << std::endl
                  << "\t\t\t" << "signalstats" << std::endl
                  << "\t\t\t" << "cropdetect" << std::endl
                  << "\t\t\t" << "psnr" << std::endl
                  << "\t\t\t" << "ebur128" << std::endl
                  << "\t\t\t" << "aphasemeter" << std::endl
                  << "\t\t\t" << "astats" << std::endl
                  << "\t\t\t" << "ssim" << std::endl
                  << "\t\t\t" << "idet" << std::endl
                  << std::endl
                  << "\t"
                  << "-y - force creation of <qctools-report> even if it already exists"
                  << std::endl
                  << "\t"
                  << "-u - upload to signalserver if <qctools-report> not exists here"
                  << std::endl
                  << "\t"
                  << "-uf - force upload <qctools-report> to signalserver (even if file already exists)"
                  << std::endl
                  << "\t"
                  << "-c <qctools-report> - check if uploaded to signalserver"
                  << std::endl
                  << std::endl;

        std::cout << "usage example: " << std::endl
                  << "\t" <<
                     "qcli -i file.mkv -u - generate stats from file.mkv and upload to signalserver if stats wasn't uploaded previously"
                  << std::endl
                  << "\t" <<
                     "qcli -i file.mkv -uf - generate stats from file.mkv and upload to signalserver unconditionally"
                  << std::endl
                  << "\t" <<
                     "qcli -i file.mkv.qctools.xml.gzip -u - upload stats to signalserver if stats wasn't uploaded"
                  << std::endl
                  << "\t" <<
                     "qcli -i file.mkv.qctools.xml.gzip -uf - upload stats to signalserver unconditionally"
                  << std::endl
                  << "\t" <<
                     "qcli -c file.mkv.qctools.xml.gzip - checks if such a file exists on signalserver"
                  << std::endl
                  << std::endl;
    }

    Preferences prefs;

    signalServer = std::unique_ptr<SignalServer>(new SignalServer());

    QString urlString = prefs.signalServerUrlString();
    if(!urlString.startsWith("http", Qt::CaseInsensitive))
        urlString.prepend("http://");

    QUrl url(urlString);

    signalServer->setUrl(url);
    signalServer->setLogin(prefs.signalServerLogin());
    signalServer->setPassword(prefs.signalServerPassword());
    signalServer->setAutoUpload(prefs.isSignalServerAutoUploadEnabled());

    if(!checkUploadFileName.isEmpty()) {
        std::cout << std::endl << "checking if " << QFileInfo(checkUploadFileName).fileName().toStdString() << " exists on signalserver side..." << std::endl;

        QSharedPointer<CheckFileUploadedOperation> op = signalServer->checkFileUploaded(QFileInfo(checkUploadFileName).fileName());
        QObject::connect(op.data(), SIGNAL(finished()), &a, SLOT(quit()));
        a.exec();

        if(op->state() == CheckFileUploadedOperation::Error)
        {
            std::cout << std::endl << "checking failed: " << op->errorString().toStdString()
                      << ", exiting... " << std::endl;

            return CheckFileUploadedError;
        }

        std::cout << (op->state() == CheckFileUploadedOperation::Uploaded ? "exists" : "not exists") << std::endl;
        return op->state() == CheckFileUploadedOperation::Uploaded ? Uploaded : NotUploaded;
    }

    if(input.isEmpty())
        return NoInput;

    if(!input.endsWith(".qctools.xml.gz")) // skip output if input is already .qctools.xml.gz
    {
        if(output.isEmpty())
            output = input + ".qctools.xml.gz";
    }

    if(!output.isEmpty() && !output.endsWith(".xml.gz"))
    {
        std::cout << "warning: non-standard extension (not *.xml.gz) has been specified for output file. " << std::endl;
    }

    QFile file(output);
    if(file.exists() && !forceOutput)
    {
        std::cout << "file " << output.toStdString() << " already exists, exiting.. " << std::endl;
        return OutputAlreadyExists;
    }

    if(file.exists() && forceOutput)
    {
        std::cout << "file " << output.toStdString() << " already exists and will be overwritten..." << std::endl;
        file.remove();
    }

    activefilters filters = prefs.activeFilters();
    if(!filterStrings.empty())
    {
        filters = 0;
        foreach(QString filterString, filterStrings)
        {
            if(filterString == "signalstats")
                filters |= 1 << ActiveFilter_Video_signalstats;
            else if(filterString == "cropdetect")
                filters |= 1 << ActiveFilter_Video_cropdetect;
            else if(filterString == "psnr")
                filters |= 1 << ActiveFilter_Video_Psnr;
            else if(filterString == "ebur128")
                filters |= 1 << ActiveFilter_Audio_EbuR128;
            else if(filterString == "aphasemeter")
                filters |= 1 << ActiveFilter_Audio_aphasemeter;
            else if(filterString == "astats")
                filters |= 1 << ActiveFilter_Audio_astats;
            else if(filterString == "ssim")
                filters |= 1 << ActiveFilter_Video_Ssim;
            else if(filterString == "idet")
                filters |= 1 << ActiveFilter_Video_Idet;
        }

        std::cout << "filters selected: ";
        if(filters.test(ActiveFilter_Video_signalstats))
            std::cout << "signalstats" << " ";
        if(filters.test(ActiveFilter_Video_cropdetect))
            std::cout << "cropdetect" << " ";
        if(filters.test(ActiveFilter_Video_Psnr))
            std::cout << "psnr" << " ";
        if(filters.test(ActiveFilter_Audio_EbuR128))
            std::cout << "ebur128" << " ";
        if(filters.test(ActiveFilter_Audio_aphasemeter))
            std::cout << "aphasemeter" << " ";
        if(filters.test(ActiveFilter_Audio_astats))
            std::cout << "astats" << " ";
        if(filters.test(ActiveFilter_Video_Ssim))
            std::cout << "ssim" << " ";
        if(filters.test(ActiveFilter_Video_Idet))
            std::cout << "idet" << " ";

        std::cout << std::endl;
    }

    info = std::unique_ptr<FileInformation>(new FileInformation(signalServer.get(), input, filters, prefs.activeAllTracks()));
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

    if(!info->hasStats())
    {
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

        QObject::connect(info.get(), SIGNAL(statsFileGenerationProgress(int, int)), this, SLOT(onStatsFileGenerationProgress(int, int)));
        QObject::connect(info.get(), SIGNAL(statsFileGenerated(SharedFile, const QString&)), &a, SLOT(quit()));
        info->startExport(output);
        a.exec();

        QObject::disconnect(info.get(), SIGNAL(statsFileGenerationProgress(int, int)), this, SLOT(onStatsFileGenerationProgress(int, int)));

        std::cout << std::endl << "generating QCTools report... done" << std::endl;
    }
    else
    {
        // stats already generated
        output = input;
    }

    if(uploadToSignalServer || forceUploadToSignalServer)
    {
        std::cout << std::endl << "checking if " << output.toStdString() << " exists on signalserver side..." << std::endl;

        QObject::connect(info.get(), SIGNAL(signalServerCheckUploadedStatusChanged()), &a, SLOT(quit()));
        QString outputFileName = QFileInfo(output).fileName();

        info->checkFileUploaded(outputFileName);
        a.exec();

        if(info->signalServerCheckUploadedStatus() == FileInformation::CheckError)
        {
            std::cout << std::endl << "checking failed: " << info->signalServerCheckUploadedStatusErrorString().toStdString()
                      << ", exiting... " << std::endl;

            return CheckFileUploadedError;
        }

        std::cout << outputFileName.toStdString() << " "
                  << (info->signalServerCheckUploadedStatus() == FileInformation::NotUploaded ? "not" : "already")
                    << " exists on signalserver" << std::endl;

        if(info->signalServerCheckUploadedStatus() == FileInformation::NotUploaded || forceUploadToSignalServer)
        {
            QObject::connect(info.get(), SIGNAL(signalServerUploadStatusChanged()), &a, SLOT(quit()));
            QObject::connect(info.get(), SIGNAL(signalServerUploadProgressChanged(qint64, qint64)), this, SLOT(onSignalServerUploadProgressChanged(qint64, qint64)));

            std::cout << "uploading... " << std::endl;
            progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));

            info->upload(output);
            a.exec();

            std::cout << std::endl;

            if(info->signalServerUploadStatus() == FileInformation::Done)
            {
                std::cout <<"uploading done" << std::endl;
            }
            else
            {
                std::cout <<"uploading failed: " << info->signalServerUploadStatusErrorString().toStdString() << std::endl;
            }
        }
    }

    return 0;
}

void Cli::updateParsingProgress()
{
    int value = info->Glue->FramesProcessedPerStream(indexOfStreamWithKnownFrameCount) * progress->getMax() /
            info->Glue->FramesCountPerStream(indexOfStreamWithKnownFrameCount);

    progress->setValue(value);
}

void Cli::onStatsFileGenerationProgress(int written, int total)
{
    statsFileBytesWritten = written;
    statsFileBytesTotal = total;

    if(statsFileBytesWritten != 0 && statsFileBytesTotal != 0)
    {
        int value = statsFileBytesWritten * progress->getMax() / statsFileBytesTotal;
        progress->setValue(value);
    }
}

void Cli::onSignalServerUploadProgressChanged(qint64 written, qint64 total)
{
    statsFileBytesUploaded = written;
    statsFileBytesToUpload = total;

    if(statsFileBytesUploaded != 0 && statsFileBytesToUpload != 0)
    {
        int value = statsFileBytesUploaded * progress->getMax() / statsFileBytesToUpload;
        progress->setValue(value);
    }
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

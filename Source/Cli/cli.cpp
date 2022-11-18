#include "cli.h"
#include <QVideoFrame>
#include "QAVVideoFrame.h"
#include "version.h"
#include "Core/FFmpegVideoEncoder.h"
#include "Core/FFmpeg_Glue.h"
#include <QDir>

Cli::Cli() : indexOfStreamWithKnownFrameCount(0), statsFileBytesWritten(0), statsFileBytesTotal(0), statsFileBytesUploaded(0), statsFileBytesToUpload(0)
{

}

int Cli::exec(QCoreApplication &a)
{
    std::string appName = "qcli";
    std::string copyright = "Copyright (C): 2013-2020, BAVC.\nCopyright (C): 2018-2020, RiceCapades LLC & MediaArea.net SARL.";
    Preferences prefs;

    QString input;
    QString output;
    QStringList filterStrings;
    bool forceOutput = false;
    bool showLongHelp = false;
    bool showShortHelp = false;
    bool showVersion = false;
    bool createMkv = true;
    QString useQCvault;
    bool ignoreQCvault = false;
    auto activeAllTracks = prefs.activeAllTracks();
    bool setActiveAllTracks = false;
    bool configIsSet = false;
    bool configHasIssues = false;

    bool uploadToSignalServer = false;
    bool forceUploadToSignalServer = false;

    QString checkUploadFileName;

    for(int i = 1; i < a.arguments().length(); ++i)
    {
        if(a.arguments().at(i) == "-i" && (i + 1) < a.arguments().length())
        {
            input = a.arguments().at(i + 1);
            ++i;
        } else if(a.arguments().at(i) == "-o" && (i + 1) < a.arguments().length())
        {
            ignoreQCvault = true;
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
            showLongHelp = true;
        } else if(a.arguments().at(i) == "-v")
        {
            showVersion = true;
        }
        else if(a.arguments().at(i) == "--resetsettings")
        {
            Preferences().resetSettings();
            std::cout << "Settings cleared." << std::endl;
            configIsSet = true;
        }
        else if (a.arguments().at(i) == "-qcvault")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }

            useQCvault = a.arguments().at(i);
            if (useQCvault == "default")
            {
                useQCvault = prefs.defaultQCvaultPathString();
                if (useQCvault.isEmpty())
                {
                    std::cout << "QCvault location can not be found." << std::endl;
                    configHasIssues = true;
                    continue;
                }
            }
        } else if (a.arguments().at(i) == "-clear-qcvault")
        {
            configIsSet = true;

            prefs.setQCvaultPathString(QString());
            useQCvault = prefs.QCvaultPathString();
            if (!useQCvault.isEmpty())
            {
                std::cout << "QCvault location can not be saved." << std::endl;
                configHasIssues = true;
                continue;
            }
            std::cout << "QCvault cleared." << std::endl;
        } else if (a.arguments().at(i) == "-set-qcvault")
        {
            ++i;
            if (i >= a.arguments().length() || a.arguments().at(i).isEmpty())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }
            configIsSet = true;

            prefs.setQCvaultPathString(a.arguments().at(i));
            useQCvault = prefs.QCvaultPathString();
            if (useQCvault.isEmpty())
            {
                std::cout << "QCvault location can not be saved." << std::endl;
                configHasIssues = true;
                continue;
            }
            std::cout << "QCvault set to " << useQCvault.toStdString() << std::endl;
        } else if (a.arguments().at(i) == "-show-qcvault")
        {
            configIsSet = true;
            bool HasError;
            auto QCvaultPathString = prefs.QCvaultPathString(&HasError);
            if (HasError)
            {
                std::cout << "Default QCvault location not available, use -qcvault instead, analyzing stopped." << std::endl;
                configHasIssues = true;
                continue;
            }
            if (QCvaultPathString.isEmpty())
                std::cout << "QCvault path is not set." << std::endl;
            else
                std::cout << "QCvault path is " << QCvaultPathString.toStdString() << '.' << std::endl;
        } else if (a.arguments().at(i) == "-s")
        {
            createMkv = false;
        } else if (a.arguments().at(i) == "-a")
        {
            createMkv = true;
        } else if (a.arguments().at(i) == "-show-panels")
        {
            configIsSet = true;
            cout << "List of available panels:" << endl;
            auto availablePanels = prefs.availablePanels();
            auto activePanels = prefs.activePanels();
            for (auto availablePanel = availablePanels.constBegin(); availablePanel != availablePanels.constEnd(); ++availablePanel)
            {
                std::cout << availablePanel - availablePanels.constBegin() + 1 << " | " << (activePanels.find(availablePanel->name) != activePanels.end() ? "Active | " : "       | ") << availablePanel->name.toStdString() << std::endl;
            }
        } else if (a.arguments().at(i) == "-activate-panel")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }
            configIsSet = true;

            // Get panel name
            auto index = a.arguments().at(i).toInt();
            QString panelName;
            auto availablePanels = prefs.availablePanels();
            if (a.arguments().at(i).toLower() == "all")
            {
                // add panel name to list
                QSet<QString> activePanels;
                for (auto availablePanel = availablePanels.constBegin(); availablePanel != availablePanels.constEnd(); ++availablePanel)
                    activePanels.insert(availablePanel->name);
                prefs.setActivePanels(activePanels);
                std::cout << "All panels activated." << std::endl;
            }
            else
            {
                for (auto availablePanel = availablePanels.constBegin(); availablePanel != availablePanels.constEnd(); ++availablePanel)
                {
                    if (index == availablePanel - availablePanels.constBegin() + 1 || a.arguments().at(i) == availablePanel->name)
                    {
                        panelName = availablePanel->name;
                        break;
                    }
                }
                if (panelName.isEmpty())
                {
                    std::cout << "Invalid panel name or index." << std::endl;
                    configHasIssues = true;
                    continue;
                }

                // add panel name to list
                auto activePanels = prefs.activePanels();
                auto panelNameInActivePanels = activePanels.insert(panelName);
                prefs.setActivePanels(activePanels);
                std::cout << "Panel " << panelName.toStdString() << " activated." << std::endl;
            }
        } else if (a.arguments().at(i) == "-deactivate-panel")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }
            configIsSet = true;

            if (a.arguments().at(i).toLower() == "all")
            {
                // add panel name to list
                QSet<QString> activePanels;
                prefs.setActivePanels(activePanels);
                std::cout << "All panels deactivated." << std::endl;
            }
            else
            {
                // Get panel name
                auto index = a.arguments().at(i).toInt();
                QString panelName;
                auto availablePanels = prefs.availablePanels();
                for (auto availablePanel = availablePanels.constBegin(); availablePanel != availablePanels.constEnd(); ++availablePanel)
                {
                    if (index == availablePanel - availablePanels.constBegin() + 1 || a.arguments().at(i) == availablePanel->name)
                    {
                        panelName = availablePanel->name;
                        break;
                    }
                }
                if (panelName.isEmpty())
                {
                    std::cout << "Invalid panel name or index." << std::endl;
                    configHasIssues = true;
                    continue;
                }

                // remove panel name from list
                auto activePanels = prefs.activePanels();
                auto panelNameInActivePanels = activePanels.find(panelName);
                if (panelNameInActivePanels != activePanels.end())
                {
                    activePanels.remove(*panelNameInActivePanels);
                    prefs.setActivePanels(activePanels);
                }
                std::cout << "Panel " << panelName.toStdString() << " deactivated." << std::endl;
            }
        } else if (a.arguments().at(i) == "-video")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }

            if (a.arguments().at(i) == "1")
                activeAllTracks.set(Type_Video, false);
            else if (a.arguments().at(i) == "all")
                activeAllTracks.set(Type_Video, true);
            else
            {
                std::cout << "-video option argument must be either 1 or all" << std::endl;
                configHasIssues = true;
                continue;
            }
        } else if (a.arguments().at(i) == "-set-video")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }
            configIsSet = true;

            if (a.arguments().at(i) == "1")
                activeAllTracks.set(Type_Video, false);
            else if (a.arguments().at(i) == "all")
                activeAllTracks.set(Type_Video, true);
            else
            {
                std::cout << "-set-video option argument must be either 1 or all" << std::endl;
                configHasIssues = true;
                continue;
            }

            std::cout << (a.arguments().at(i) == "1" ? "First video track will be analyzed." : "All video tracks will be analyzed.") << std::endl;
            setActiveAllTracks = true;
        } else if (a.arguments().at(i) == "-audio")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }

            if (a.arguments().at(i) == "1")
                activeAllTracks.set(Type_Audio, false);
            else if (a.arguments().at(i) == "all")
                activeAllTracks.set(Type_Audio, true);
            else
            {
                std::cout << "-audio option argument must be either 1 or all" << std::endl;
                configHasIssues = true;
                continue;
            }
        } else if (a.arguments().at(i) == "-set-audio")
        {
            ++i;
            if (i >= a.arguments().length())
            {
                std::cout << "Missing argument after last option." << std::endl;
                configHasIssues = true;
                continue;
            }
            configIsSet = true;

            if (a.arguments().at(i) == "1")
                activeAllTracks.set(Type_Audio, false);
            else if (a.arguments().at(i) == "all")
                activeAllTracks.set(Type_Audio, true);
            else
            {
                std::cout << "-set-audio option argument must be either 1 or all" << std::endl;
                configHasIssues = true;
                continue;
            }

            std::cout << (a.arguments().at(i) == "1" ? "First audio track will be analyzed." : "All audio tracks will be analyzed.") << std::endl;
            setActiveAllTracks = true;
        }
    }

    // QCvault
    if (!ignoreQCvault && useQCvault.isEmpty())
    {
        bool HasError;
        useQCvault = prefs.QCvaultPathString(&HasError);
        if (HasError)
        {
            std::cout << "App local data location not available, use -qcvault instead, analyzing stopped.. " << std::endl;
            configHasIssues = true;
        }
    }

    // Single/all video/audio tracks
    if (setActiveAllTracks)
    {
        prefs.setActiveAllTracks(activeAllTracks);
    }

    if (configHasIssues)
        return InvalidInput;

    if(!showLongHelp)
    {
        if(a.arguments().length() == 1 || (checkUploadFileName.isEmpty() && input.isEmpty()))
            showShortHelp = true;
    }

    if(showVersion) {
        std::cout << appName << " " << (VERSION) << std::endl << std::endl;
        std::cout << "FFMpeg version: " << FFmpeg_Glue::FFmpeg_Version() << std::endl;
        std::cout << "FFMpeg configuration: " << FFmpeg_Glue::FFmpeg_Configuration() << std::endl;
        std::cout << "FFMpeg libraries: " << FFmpeg_Glue::FFmpeg_LibsVersion() << std::endl;

        return Success;
    }

    if(showLongHelp || showShortHelp)
    {
        if (configIsSet)
            return Success;

        if(showShortHelp)
        {
            std::cout <<
                 appName << " " << (VERSION) << std::endl << copyright << std::endl <<
                 "Usage: " << appName << " -i <qctools-input> [-o <qctools-output>]" << std::endl << std::endl <<
                 "Use " << appName << " -h to get detailed help" << std::endl << std::endl;
        }
        else
        {
            std::cout
                << appName << " " << (VERSION) << std::endl << copyright << std::endl
                << "Usage: " << appName << " -i <qctools-input> [-o <qctools-output>]" << std::endl
                << std::endl
                << "-i <input file>" << std::endl
                << "    Specifies absolute path of input file, including extension." << std::endl
                << "-o <output file>" << std::endl
                << "    Specifies output file path, including extension. If no output file is" << std::endl
                << "    declared, qctools will create an output named after the input file, suffixed" << std::endl
                << "    with \".qctools.xml.gz\" (if -s used) or  \".qctools.mkv\" (if -a used)." << std::endl
                << "-s" << std::endl
                << "    Stats only (no thumbnails, no panels)." << std::endl
                << "-a" << std::endl
                << "    All (stats + thumbnails + panels)." << std::endl
                << "    Is default." << std::endl
                << "-qcvault <QCvault path>" << std::endl
                << "    Use the indicated path as the QCvault location." << std::endl
                << "    Use \"-qcvault default\" for using the standard QCvault location." << std::endl
                << "-set-qcvault <QCvault path>" << std::endl
                << "    Register the indicated path as the QCvault location." << std::endl
                << "    Use \"-set-qcvault default\" for using the standard QCvault location." << std::endl
                << "-show-qcvault" << std::endl
                << "    Show the registered QCvault location." << std::endl
                << "-clear-qcvault" << std::endl
                << "    Clear the QCvault location. Default directory becomes the input directory." << std::endl
                << "-show-panels" << std::endl
                << "    Show the available panels." << std::endl
                << "    First column is an index to be used with -activate-panel or -deactivate-panel." << std::endl
                << "    Second column is the status of the panel (\"active\" if the panel is used during analysis)." << std::endl
                << "    Third column is the name of the panel" << std::endl
                << "-activate-panel <panel name or index>" << std::endl
                << "    Register the panel as active." << std::endl
                << "    Use \"all\" panel name for registering all panels as active." << std::endl
                << "-deactivate-panel <panel name or index>" << std::endl
                << "    Register the panel as not active." << std::endl
                << "    Use \"all\" panel name for registering all panels as not active." << std::endl
                << "-video <1 or all>" << std::endl
                << "    Analyze only the first video track or all video tracks." << std::endl
                << "-set-video <1 or all>" << std::endl
                << "    Register the choice of analyzing either the first video track or all video tracks." << std::endl
                << "-audio <1 or all>" << std::endl
                << "    Analyze only the first video track or all audio tracks." << std::endl
                << "-set-audio <1 or all>" << std::endl
                << "    Register the choice of analyzing either the first audio track or all audio tracks." << std::endl
                << "-f" << std::endl
                << "    Specifies '+'-separated string of filters used. Example: -f signalstats+cropdetect" << std::endl
                << "    The filters used in " << appName << " may also be declared via the qctools-gui (see the" << std::endl
                << "    Preferences panel)." << std::endl
                << "        Available filters: " << std::endl
                << "            signalstats" << std::endl
                << "            cropdetect" << std::endl
                << "            psnr" << std::endl
                << "            ebur128" << std::endl
                << "            aphasemeter" << std::endl
                << "            astats" << std::endl
                << "            ssim" << std::endl
                << "            idet" << std::endl
                << "            deflicker" << std::endl
                << "            entropy" << std::endl
                << "            entropy-diff" << std::endl
                << std::endl
                << "-y" << std::endl
                << "    Force creation of <qctools-report> even if it already exists" << std::endl
                << "-resetsettings" << std::endl
                << "    Reset application settings" << std::endl
                << std::endl;

            std::cout
                << "Signal Server flags:" << std::endl
                << "-u" << std::endl
                << "    Upload to Signal Server if <qctools-report> not exists here" << std::endl
                << "-uf" << std::endl
                << "    Force upload <qctools-report> to signalserver (even if file already exists)" << std::endl
                << "-c <qctools-report>" << std::endl
                << "    Check if uploaded to Signal Server" << std::endl
                << std::endl;

            std::cout
                << "Usage example: " << std::endl
                << "    " << appName << " -i file.mkv" << std::endl
                << "        generates qctools file in same directory named file.mkv.qctools.xml.gz" << std::endl
                << "    " << appName << " -i file.mkv -o report.xml.gz" << std::endl
                << "        generates qctools file in same directory named report.xml.gz" << std::endl
                << "    " << appName << " -i file.mkv -u" << std::endl
                << "        generate stats from file.mkv and upload to Signal Server if stats wasn't" << std::endl
                << "        uploaded previously" << std::endl
                << "    " << appName << " -i file.mkv -uf" << std::endl
                << "        generate stats from file.mkv and upload to Signal Server unconditionally" << std::endl
                << "    " << appName << " -i file.mkv.qctools.xml.gzip -u" << std::endl
                << "        upload stats to Signal Server if stats wasn't uploaded" << std::endl
                << "    " << appName << " -i file.mkv.qctools.xml.gzip -uf" << std::endl
                << "        upload stats to Signal Server unconditionally" << std::endl
                << "    " << appName << " -c file.mkv.qctools.xml.gzip" << std::endl
                << "        checks if such a file exists on Signal Server" << std::endl
                << std::endl;
        }

        return Success;
    }

    std::cout << appName << " " << (VERSION) << std::endl;

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

    if(!input.endsWith(".qctools.xml.gz") && !input.endsWith(".qctools.mkv")) // skip output if input is already .qctools.xml.gz
    {
        if (!useQCvault.isEmpty())
        {
            if (!output.isEmpty())
            {
                std::cout << "-qcvault and -o can not be used at same time, analyzing stopped." << std::endl;
                return InvalidInput;
            }

            auto fileNameQCvault = prefs.createQCvaultFileNameString(input, useQCvault);
            if (fileNameQCvault.isEmpty())
            {
                std::cout << "Problem while creating output file name, analyzing stopped." << std::endl;
                return InvalidInput;
            }

            output = fileNameQCvault + (createMkv ? ".qctools.mkv" : ".qctools.xml.gz");
            auto outPath = QFileInfo(output).dir();
            if (!outPath.mkpath("."))
            {
                std::cout << "Can not create output directory, analyzing stopped." << std::endl;
                return InvalidInput;
            }
        }

        if(output.isEmpty())
            output = input + (createMkv ? ".qctools.mkv" : ".qctools.xml.gz");
    }

    bool mkvReport = output.endsWith(".qctools.mkv");
    bool xmlGzReport = output.endsWith(".xml.gz");

    if(!output.isEmpty() && !xmlGzReport && !mkvReport)
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
            else if(filterString == "deflicker")
                filters |= 1 << ActiveFilter_Video_Deflicker;
            else if(filterString == "entropy")
                filters |= 1 << ActiveFilter_Video_Entropy;
            else if(filterString == "entropy-diff")
                filters |= 1 << ActiveFilter_Video_EntropyDiff;
        }
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
    if(filters.test(ActiveFilter_Video_Deflicker))
        std::cout << "deflicker" << " ";
    if(filters.test(ActiveFilter_Video_Entropy))
        std::cout << "entropy" << " ";
    if(filters.test(ActiveFilter_Video_EntropyDiff))
        std::cout << "entropy-diff" << " ";

    std::cout << std::endl;

    info = std::unique_ptr<FileInformation>(new FileInformation(signalServer.get(), input, filters, activeAllTracks, prefs.getActivePanels(), useQCvault.isEmpty() ? QString() : prefs.createQCvaultFileNameString(input)));
    info->setAutoCheckFileUploaded(false);
    info->setAutoUpload(false);

    std::cout << std::endl << "analyzing input file... " << input.toStdString() << std::endl;

    if(!info->isValid())
    {
        std::cout << "invalid input, analyzing stopped.. " << std::endl;
        return InvalidInput;
    }

    for(int i = 0; i < info->Glue->StreamCount_Get(); ++i)
    {
        if(info->Glue->FramesCountPerStream(i) > info->Glue->FramesCountPerStream(indexOfStreamWithKnownFrameCount))
            indexOfStreamWithKnownFrameCount = i;
    }

    if(!info->hasStats() || forceOutput)
    {
        // parse

        progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));
        progress->setValue(0);

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
        progress->setValue(0);

        QObject::connect(info.get(), &FileInformation::statsFileGenerationProgress, this, &Cli::onStatsFileGenerationProgress);
        QObject::connect(info.get(), &FileInformation::statsFileGenerated, this, [&](SharedFile statsFile, const QString& name) {
            if(mkvReport) {
                QByteArray attachment;
                QString attachmentFileName;

                qDebug() << "fileName: " << statsFile.data()->fileName();
                attachment = statsFile->readAll();
                attachmentFileName = name;

                FFmpegVideoEncoder encoder;
                FFmpegVideoEncoder::Metadata metadata;
                metadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString("QCTools Report for %1").arg(QFileInfo(info->fileName()).fileName()));
                metadata << FFmpegVideoEncoder::MetadataEntry(QString("creation_time"), QString("now"));

                encoder.setMetadata(metadata);

                int thumbnailsCount = info->Glue->Thumbnails_Size(0);
                int thumbnailIndex = 0;

                int num = 0;
                int den = 0;

                info->Glue->getOutputTimeBase(0, num, den);

                FFmpegVideoEncoder::Source source;
                source.width = info->Glue->OutputThumbnailWidth_Get();
                source.height = info->Glue->OutputThumbnailHeight_Get();
                source.bitrate = info->Glue->OutputThumbnailBitRate_Get();

                FFmpegVideoEncoder::Metadata streamMetadata;
                streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString("Frame Thumbnails"));

                source.metadata = streamMetadata;
                source.num = num;
                source.den = den;
                source.getPacket = [&]() -> std::shared_ptr<AVPacket> {

                    if(thumbnailIndex == 0) {
                        std::cout << std::endl << "adding thumbnails to QCTools report... " << std::endl;

                        progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));
                        progress->setValue(0);
                    }

                    bool hasNext = thumbnailIndex < thumbnailsCount;

                    progress->setValue(100 * thumbnailIndex/ thumbnailsCount);
                    QCoreApplication::processEvents();

                    if(!hasNext) {
                        return nullptr;
                    }

                    return info->Glue->ThumbnailPacket_Get(0, thumbnailIndex++);
                };

                QVector<FFmpegVideoEncoder::Source> sources;
                sources.push_back(source);

                for(auto& panelTitle : info->panelOutputsByTitle().keys())
                {
                    auto panelOutputIndexes = info->panelOutputsByTitle()[panelTitle];
                    for(auto panelOutputIndex : panelOutputIndexes)
                    {
                        auto panelFramesCount = info->getPanelFramesCount(panelOutputIndex);
                        if(panelFramesCount == 0)
                            continue;

                        auto panelsCount = info->getPanelFramesCount(panelOutputIndex);
                        auto panelIndex = 0;

                        FFmpegVideoEncoder::Metadata streamMetadata;
                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("title"), QString::fromStdString(panelTitle));
                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("filterchain"), QString::fromStdString(info->Glue->getOutputFilter(panelOutputIndex)));

                        auto outputMetadata = info->getPanelOutputMetadata(panelOutputIndex);
                        auto versionIt = outputMetadata.find("version");
                        auto yaxisIt = outputMetadata.find("yaxis");
                        auto legendIt = outputMetadata.find("legend");
                        auto panelTypeIt = outputMetadata.find("panel_type");
                        auto version = versionIt != outputMetadata.end() ? versionIt->second : "";
                        auto yaxis = yaxisIt != outputMetadata.end() ? yaxisIt->second : "";
                        auto legend = legendIt != outputMetadata.end() ? legendIt->second : "";
                        auto panelType = panelTypeIt != outputMetadata.end() ? panelTypeIt->second : "video";
                        auto isAudioPanel = panelType != "video";

                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("version"), QString::fromStdString(version));
                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("yaxis"), QString::fromStdString(yaxis));
                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("legend"), QString::fromStdString(legend));
                        streamMetadata << FFmpegVideoEncoder::MetadataEntry(QString("panel_type"), QString::fromStdString(panelType));

                        FFmpegVideoEncoder::Source panelSource;
                        panelSource.metadata = streamMetadata;
                        panelSource.width = info->panelSize().width();
                        panelSource.height = info->panelSize().height();
                        panelSource.bitrate = info->Glue->OutputThumbnailBitRate_Get() / info->panelSize().width();
                        panelSource.num = num;
                        panelSource.den = den;
                        panelSource.getPacket = [panelIndex, panelsCount, panelOutputIndex, this]() mutable -> std::shared_ptr<AVPacket> {

                            if(panelIndex == 0)
                            {
                                std::cout << std::endl << "adding panels to QCTools report... " << std::endl;

                                progress = unique_ptr<ProgressBar>(new ProgressBar(0, 100, 50, "%"));
                                progress->setValue(0);
                            }

                            bool hasNext = panelIndex < panelsCount;

                            progress->setValue(100 * panelIndex / panelsCount);
                            QCoreApplication::processEvents();

                            if(!hasNext) {
                                return nullptr;
                            }

                            auto frame = info->getPanelFrame(panelOutputIndex, panelIndex);
                            auto packet = info->Glue->encodePanelFrame(panelOutputIndex, frame.frame());

                            ++panelIndex;

                            return packet;
                        };

                        sources.push_back(panelSource);
                    }
                }

                encoder.makeVideo(output, sources, attachment, attachmentFileName);
            }

            a.quit();
        });
        info->setExportFilters(filters);

        if(mkvReport) {
            info->startExport();
        } else {
            info->startExport(output);
        }
        a.exec();

        QObject::disconnect(info.get(), SIGNAL(statsFileGenerationProgress(quint64, quint64)), this, SLOT(onStatsFileGenerationProgress(quint64, quint64)));

        std::cout << std::endl << "generating QCTools report... done, in " << output.toStdString() << std::endl;
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
            progress->setValue(0);

            info->upload(QFileInfo(output));
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

void Cli::onStatsFileGenerationProgress(quint64 written, quint64 total)
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

int ProgressBar::getMax() const
{
    return max;
}

void ProgressBar::setMax(int value)
{
    max = value;
}

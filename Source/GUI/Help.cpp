/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/Help.h"
#include "Core/FFmpeg_Glue.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QTabWidget>
#include <QFile>
#include <QLabel>
#include <QtAV>
#include "CMarkdown.h"
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
// Constructor
Help::Help(QWidget * parent)
: QDialog(parent)
{
    move(QApplication::desktop()->screenGeometry().width()/5, y());
    resize(QApplication::desktop()->screenGeometry().width()-QApplication::desktop()->screenGeometry().width()/5*2, QApplication::desktop()->screenGeometry().height()*3/4);

    setWindowFlags(windowFlags()&(0xFFFFFFFF-Qt::WindowContextHelpButtonHint));
    setWindowTitle("QCTools help");

    Close=new QPushButton("&Close");
    Close->setDefault(true);
    QDialogButtonBox* Dialog=new QDialogButtonBox();
    Dialog->addButton(Close, QDialogButtonBox::AcceptRole);
    connect(Dialog, SIGNAL(accepted()), this, SLOT(close()));

    QVBoxLayout* L=new QVBoxLayout();
    Central=new QTabWidget(this);


    QTextBrowser* Text1=new QTextBrowser(this);
    Text1->setReadOnly(true);
    Text1->setOpenExternalLinks(true);
    CMarkdown getting_started;
    QString html1 = getting_started.processFile(":/Help/getting_started.md");
    html1.replace("src=\"media/", "src=\"qrc:/Help/");
    Text1->setHtml(html1);
    Central->addTab(Text1, tr("Getting Started"));


    QTextBrowser* Text2 =new QTextBrowser(this);
    Text2->setReadOnly(true);
    Text2->setOpenExternalLinks(true);
    CMarkdown how_to_use;
    QString html2 = how_to_use.processFile(":/Help/how_to_use.md");
    html2.replace("src=\"media/", "src=\"qrc:/Help/");
    Text2->setHtml(html2);
    Central->addTab(Text2, tr("How To Use"));


    QTextBrowser* Text3 =new QTextBrowser(this);
    Text3->setReadOnly(true);
    Text3->setOpenExternalLinks(true);
    CMarkdown filter_descriptions;
    QString html3 = filter_descriptions.processFile(":/Help/filter_descriptions.md");
    html3.replace("src=\"media/", "src=\"qrc:/Help/");
    Text3->setHtml(html3);
    Central->addTab(Text3, tr("Filter Descriptions"));


    QTextBrowser* Text4 =new QTextBrowser(this);
    Text4->setReadOnly(true);
    Text4->setOpenExternalLinks(true);
    CMarkdown playback_filters;
    QString html4 = playback_filters.processFile(":/Help/playback_filters.md");
    html4.replace("src=\"media/", "src=\"qrc:/Help/");
    Text4->setHtml(html4);
    Central->addTab(Text4, tr("Playback Filters"));


    QTextBrowser* Text5 =new QTextBrowser(this);
    Text5->setReadOnly(true);
    Text5->setOpenExternalLinks(true);
    CMarkdown data_format;
    QString html5 = data_format.processFile(":/Help/data_format.md");
    html5.replace("src=\"media/", "src=\"qrc:/Help/");
    Text5->setHtml(html5);
    Central->addTab(Text5, tr("Data Format"));

    QTextBrowser* Text6 =new QTextBrowser(this);
    Text6->setReadOnly(true);
    Text6->setOpenExternalLinks(true);
    CMarkdown recording;
    QString html6 = recording.processFile(":/Help/recording.md");
    html6.replace("src=\"media/", "src=\"qrc:/Help/");
    Text6->setHtml(html6);
    Central->addTab(Text6, tr("Recording"));



    QTextBrowser* Text7=new QTextBrowser(this);
    Text7->setReadOnly(true);
    Text7->setOpenExternalLinks(true);
    Text7->setSource(QUrl("qrc:/Help/About.html"));
    Central->addTab(Text7, tr("About"));

    auto label = new QTextBrowser();
    QString ffmpegGlueVersionInfo = QString("<h3>QCTools ffmpeg integration</h3><p>%1</p><p>%2</p><p>%3</p><p>%4</p><p>%5</p>");

    {
        auto version = FFmpeg_Glue::FFmpeg_Version();
        auto year = FFmpeg_Glue::FFmpeg_Year();
        auto compiler = FFmpeg_Glue::FFmpeg_Compiler();
        auto configuration = FFmpeg_Glue::FFmpeg_Configuration();
        auto libsVersion = FFmpeg_Glue::FFmpeg_LibsVersion();

        ffmpegGlueVersionInfo = ffmpegGlueVersionInfo.arg(QString::fromStdString(version))
                .arg(year)
                .arg(QString::fromStdString(compiler))
                .arg(QString::fromStdString(configuration))
                .arg(QString::fromStdString(libsVersion));
    }

    label->setHtml(QString("%1<br>*******************<br>%2<br>*******************<br>%3").arg(ffmpegGlueVersionInfo).arg(QtAV::aboutQtAV_HTML()).arg(QtAV::aboutFFmpeg_HTML()));
    Central->addTab(label, tr("Version"));

    L->addWidget(Central);
    L->addWidget(Close);
    setLayout(L);
}

//***************************************************************************
// Direct access
//***************************************************************************

//---------------------------------------------------------------------------
void Help::GettingStarted ()
{
    Central->setCurrentIndex(0);
    show();
}

//---------------------------------------------------------------------------
void Help::HowToUseThisTool ()
{
    Central->setCurrentIndex(1);
    show();
}

//---------------------------------------------------------------------------
void Help::FilterDescriptions ()
{
    Central->setCurrentIndex(2);
    show();
}

//---------------------------------------------------------------------------
void Help::PlaybackFilters ()
{
    Central->setCurrentIndex(3);
    show();
}

//---------------------------------------------------------------------------
void Help::DataFormat ()
{
    Central->setCurrentIndex(4);
    show();
}

//---------------------------------------------------------------------------
void Help::About ()
{
    Central->setCurrentIndex(6);
    show();
}

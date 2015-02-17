/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "GUI/Help.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QTabWidget>
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
    Text1->setSource(QUrl("qrc:/Help/Getting Started/Getting Started.html"));
    Central->addTab(Text1, tr("Getting Started"));

    QTextBrowser* Text2=new QTextBrowser(this);
    Text2->setReadOnly(true);
    Text2->setOpenExternalLinks(true);
    Text2->setSource(QUrl("qrc:/Help/How To Use/How To Use.html"));
    Central->addTab(Text2, tr("How To Use"));

    QTextBrowser* Text3=new QTextBrowser(this);
    Text3->setReadOnly(true);
    Text3->setOpenExternalLinks(true);
    Text3->setSource(QUrl("qrc:/Help/Filter Descriptions/Filter Descriptions.html"));
    Central->addTab(Text3, tr("Filter Descriptions"));

    QTextBrowser* Text4=new QTextBrowser(this);
    Text4->setReadOnly(true);
    Text4->setOpenExternalLinks(true);
    Text4->setSource(QUrl("qrc:/Help/Playback Filters/Playback Filters.html"));
    Central->addTab(Text4, tr("Playback Filters"));

    QTextBrowser* Text5=new QTextBrowser(this);
    Text5->setReadOnly(true);
    Text5->setOpenExternalLinks(true);
    Text5->setSource(QUrl("qrc:/Help/Data Format/Data Format.html"));
    Central->addTab(Text5, tr("Data Format"));

    QTextBrowser* Text6=new QTextBrowser(this);
    Text6->setReadOnly(true);
    Text6->setOpenExternalLinks(true);
    Text6->setSource(QUrl("qrc:/Help/Recording/Recording.html"));
    Central->addTab(Text6, tr("Recording"));

    QTextBrowser* Text7=new QTextBrowser(this);
    Text7->setReadOnly(true);
    Text7->setOpenExternalLinks(true);
    Text7->setSource(QUrl("qrc:/Help/About.html"));
    Central->addTab(Text7, tr("About"));

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

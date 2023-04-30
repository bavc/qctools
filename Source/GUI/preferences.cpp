/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "preferences.h"
#include "ui_preferences.h"
#include "Core/SignalServerConnectionChecker.h"
#include "Core/Preferences.h"
#include <QSettings>
#include <QStandardPaths>
#include <QMetaType>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QFileDialog>
#include <QDesktopServices>
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
PreferencesDialog::PreferencesDialog(Preferences* preferences, SignalServerConnectionChecker* connectionChecker, QWidget *parent) :
    preferences(preferences),
    connectionChecker(connectionChecker),
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);

    //Configuration
    connect(this, SIGNAL(accepted()), this, SLOT(OnAccepted()));
    connect(this, SIGNAL(rejected()), this, SLOT(OnRejected()));

    Load();
}

//---------------------------------------------------------------------------
PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

QString PreferencesDialog::QCvaultPathString() const
{
    return preferences->QCvaultPathString();
}

QString PreferencesDialog::defaultQCvaultPathString() const
{
    return preferences->defaultQCvaultPathString();
}

bool PreferencesDialog::isSignalServerEnabled() const
{
    return preferences->isSignalServerEnabled();
}

bool PreferencesDialog::isSignalServerAutoUploadEnabled() const
{
    return preferences->isSignalServerAutoUploadEnabled();
}

QString PreferencesDialog::signalServerUrlString() const
{
    return preferences->signalServerUrlString();
}

QString PreferencesDialog::signalServerLogin() const
{
    return preferences->signalServerLogin();
}

QString PreferencesDialog::signalServerPassword() const
{
    return preferences->signalServerPassword();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void PreferencesDialog::Load()
{
    ActiveFilters = preferences->activeFilters();
    ActiveAllTracks = preferences->activeAllTracks();
    for(auto panelInfo : preferences->availablePanels())
    {
        if(preferences->activePanels().contains(panelInfo.name))
            ActivePanels[panelInfo.name] = panelInfo.filterchain;
    }

    ui->panelsGroupBox->setUpdatesEnabled(false);
    auto children = ui->panelsGroupBox->findChildren<QCheckBox*>();
    qDeleteAll(children);
    auto allPanels = preferences->availablePanels();
    for(auto panel : allPanels) {
        auto panelCheckbox = new QCheckBox(panel.name, ui->panelsGroupBox);
        panelCheckbox->setProperty("filterchain", panel.filterchain);

        ui->panelsGroupBox->layout()->addWidget(panelCheckbox);
        if(ActivePanels.contains(panel.name))
            panelCheckbox->setChecked(true);
    }
    ui->panelsGroupBox->setUpdatesEnabled(true);

    ui->Filters_Video_signalstats->setChecked(ActiveFilters[ActiveFilter_Video_signalstats]);
    ui->Filters_Video_cropdetect->setChecked(ActiveFilters[ActiveFilter_Video_cropdetect]);
    ui->Filters_Video_Psnr->setChecked(ActiveFilters[ActiveFilter_Video_Psnr]);
    ui->Filters_Video_Ssim->setChecked(ActiveFilters[ActiveFilter_Video_Ssim]);
    ui->Filters_Video_Idet->setChecked(ActiveFilters[ActiveFilter_Video_Idet]);
    ui->Filters_Video_Deflicker->setChecked(ActiveFilters[ActiveFilter_Video_Deflicker]);
    ui->Filters_Video_Entropy->setChecked(ActiveFilters[ActiveFilter_Video_Entropy]);
    ui->Filters_Video_EntropyDiff->setChecked(ActiveFilters[ActiveFilter_Video_EntropyDiff]);
    ui->Filters_Video_blockdetect->setChecked(ActiveFilters[ActiveFilter_Video_blockdetect]);
    ui->Filters_Video_blurdetect->setChecked(ActiveFilters[ActiveFilter_Video_blurdetect]);
    ui->Filters_Audio_EbuR128->setChecked(ActiveFilters[ActiveFilter_Audio_EbuR128]);
    ui->Filters_Audio_aphasemeter->setChecked(ActiveFilters[ActiveFilter_Audio_aphasemeter]);
    ui->Filters_Audio_astats->setChecked(ActiveFilters[ActiveFilter_Audio_astats]);

    ui->Tracks_Video_First->setChecked(!ActiveAllTracks[Type_Video]);
    ui->Tracks_Video_All->setChecked(ActiveAllTracks[Type_Video]);
    ui->Tracks_Audio_First->setChecked(!ActiveAllTracks[Type_Audio]);
    ui->Tracks_Audio_All->setChecked(ActiveAllTracks[Type_Audio]);

    if (QCvaultPathString().isEmpty())
        ui->QCvaultPath_None->setChecked(true);
    else if (QCvaultPathString()==defaultQCvaultPathString())
    {
        ui->QCvaultPath_lineEdit->setText(defaultQCvaultPathString());
        ui->QCvaultPath_Default->setChecked(true);
    }
    else
    {
        ui->QCvaultPath_lineEdit->setText(QCvaultPathString());
        ui->QCvaultPath_Custom->setChecked(true);
    }

    ui->signalServerUrl_lineEdit->setText(signalServerUrlString());
    ui->signalServerLogin_lineEdit->setText(signalServerLogin());
    ui->signalServerPassword_lineEdit->setText(signalServerPassword());
    ui->signalServerEnable_checkBox->setChecked(isSignalServerEnabled());
}

//---------------------------------------------------------------------------
void PreferencesDialog::Save()
{
    preferences->setActiveFilters(ActiveFilters);
    preferences->setActiveAllTracks(ActiveAllTracks);

    qDebug() << "ActivePanels: " << ActivePanels.size();
    auto activePanelsNames = ActivePanels.keys();

    QSet<QString> A;
    for (auto it = activePanelsNames.begin(); it != activePanelsNames.end(); ++it)
        A.insert(*it);
    preferences->setActivePanels(A);

    preferences->setQCvaultPathString(ui->QCvaultPath_lineEdit->text());

    preferences->setSignalServerUrlString(ui->signalServerUrl_lineEdit->text());
    preferences->setSignalServerLogin(ui->signalServerLogin_lineEdit->text());
    preferences->setSignalServerPassword(ui->signalServerPassword_lineEdit->text());
    preferences->setSignalServerEnabled(ui->signalServerEnable_checkBox->isChecked());
    preferences->setSignalServerAutoUploadEnabled(ui->signalServerEnableAutoUpload_checkBox->isChecked());

    preferences->sync();
}

//***************************************************************************
// Slots
//***************************************************************************

//---------------------------------------------------------------------------
void PreferencesDialog::OnAccepted()
{
    ActiveFilters.reset();
    if (ui->Filters_Video_signalstats->isChecked())
        ActiveFilters.set(ActiveFilter_Video_signalstats);
    if (ui->Filters_Video_cropdetect->isChecked())
        ActiveFilters.set(ActiveFilter_Video_cropdetect);
    if (ui->Filters_Video_Psnr->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Psnr);
    if (ui->Filters_Video_Ssim->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Ssim);
    if (ui->Filters_Video_Idet->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Idet);
    if (ui->Filters_Video_Deflicker->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Deflicker);
    if (ui->Filters_Video_Entropy->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Entropy);
    if (ui->Filters_Video_EntropyDiff->isChecked())
        ActiveFilters.set(ActiveFilter_Video_EntropyDiff);
    if (ui->Filters_Video_blockdetect->isChecked())
        ActiveFilters.set(ActiveFilter_Video_blockdetect);
    if (ui->Filters_Video_blurdetect->isChecked())
        ActiveFilters.set(ActiveFilter_Video_blurdetect);
    if (ui->Filters_Audio_EbuR128->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_EbuR128);
    if (ui->Filters_Audio_aphasemeter->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_aphasemeter);
    if (ui->Filters_Audio_astats->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_astats);

    ActiveAllTracks.reset();
    if (ui->Tracks_Video_All->isChecked())
        ActiveAllTracks.set(Type_Video);
    if (ui->Tracks_Audio_All->isChecked())
        ActiveAllTracks.set(Type_Audio);

    ActivePanels.clear();
    for(auto checkbox : ui->panelsGroupBox->findChildren<QCheckBox*>())
    {
        if(checkbox->isChecked())
            ActivePanels.insert(checkbox->text(), checkbox->property("filterchain").toString());
    }

    Save();

    Q_EMIT saved();
}

//---------------------------------------------------------------------------
void PreferencesDialog::OnRejected()
{
    Load();
}

void PreferencesDialog::on_testConnection_pushButton_clicked()
{
    // workaround for focus bug on mac
    on_signalServerUrl_lineEdit_editingFinished();

    struct UI {
        static void setSuccess(QLabel* label, QPushButton* button) {
            label->setStyleSheet("color: green");
            label->setText("Success!");
            button->setEnabled(true);
        }
        static void setError(QLabel* label, const QString& error, QPushButton* button) {
            label->setStyleSheet("color: red");
            label->setText(error);
            button->setEnabled(true);
        }
        static void setChecking(QLabel* label, QPushButton* button) {
            label->setStyleSheet("");
            label->setText("Checking..");
            button->setEnabled(false);
        }
    };

    UI::setChecking(ui->connectionTest_label, ui->testConnection_pushButton);

    QString url = ui->signalServerUrl_lineEdit->text();
    if(!url.startsWith("http", Qt::CaseInsensitive))
        url.prepend("http://");

    connectionChecker->checkConnection(url, ui->signalServerLogin_lineEdit->text(), ui->signalServerPassword_lineEdit->text());

    QEventLoop loop;
    connect(connectionChecker, SIGNAL(done()), &loop, SLOT(quit()));
    loop.exec();

    if(connectionChecker->state() == SignalServerConnectionChecker::Online)
    {
        UI::setSuccess(ui->connectionTest_label, ui->testConnection_pushButton);
    } else if(connectionChecker->state() == SignalServerConnectionChecker::Timeout)
    {
       UI::setError(ui->connectionTest_label, "Connection timeout", ui->testConnection_pushButton);
    } else if(connectionChecker->state() == SignalServerConnectionChecker::Error)
    {
       UI::setError(ui->connectionTest_label, QString("%0").arg(connectionChecker->errorString()), ui->testConnection_pushButton);
    }
}

void PreferencesDialog::on_signalServerUrl_lineEdit_editingFinished()
{
    if(ui->signalServerUrl_lineEdit->text().endsWith("/"))
    {
        ui->signalServerUrl_lineEdit->blockSignals(true);

        QString url = ui->signalServerUrl_lineEdit->text();
        while(url.endsWith("/"))
            url.resize(url.length() - 1);

        ui->signalServerUrl_lineEdit->setText(url);
        ui->signalServerUrl_lineEdit->blockSignals(false);
    }
}

void PreferencesDialog::on_QCvaultPath_None_toggled(bool checked)
{
    if (checked)
    {
        ui->QCvaultPath_lineEdit->setText(QString());
        ui->QCvaultPath_lineEdit->setEnabled(false);
        ui->QCvaultPath_select->setEnabled(false);
        ui->QCvaultPath_open->setEnabled(false);
    }
}

void PreferencesDialog::on_QCvaultPath_Default_toggled(bool checked)
{
    if (checked)
    {
        ui->QCvaultPath_lineEdit->setText(defaultQCvaultPathString());
        ui->QCvaultPath_lineEdit->setEnabled(false);
        ui->QCvaultPath_select->setEnabled(false);
        ui->QCvaultPath_open->setEnabled(true);
    }
}

void PreferencesDialog::on_QCvaultPath_Custom_toggled(bool checked)
{
    if (checked)
    {
        //ui->QCvaultPath_lineEdit->setEnabled(true);
        ui->QCvaultPath_select->setEnabled(true);
        ui->QCvaultPath_open->setEnabled(true);
    }
}

void PreferencesDialog::on_QCvaultPath_select_clicked(bool checked)
{
    QString Directory = QFileDialog::getExistingDirectory(this, "Select QCvault directory",
                                                                ui->QCvaultPath_lineEdit->text(),
                                                                QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);
    if (!Directory.isEmpty())
        ui->QCvaultPath_lineEdit->setText(Directory);
}

void PreferencesDialog::on_QCvaultPath_open_clicked(bool checked)
{
    QDesktopServices::openUrl(ui->QCvaultPath_lineEdit->text());
}

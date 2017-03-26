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
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
PreferencesDialog::PreferencesDialog(SignalServerConnectionChecker* connectionChecker, QWidget *parent) :
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

QList<std::tuple<int, int> > PreferencesDialog::loadFilterSelectorsOrder()
{
    return preferences.loadFilterSelectorsOrder();
}

void PreferencesDialog::saveFilterSelectorsOrder(const QList<std::tuple<int, int> > &order)
{
    preferences.saveFilterSelectorsOrder(order);
}

bool PreferencesDialog::isSignalServerEnabled() const
{
    return preferences.isSignalServerEnabled();
}

bool PreferencesDialog::isSignalServerAutoUploadEnabled() const
{
    return preferences.isSignalServerAutoUploadEnabled();
}

QString PreferencesDialog::signalServerUrlString() const
{
    return preferences.signalServerUrlString();
}

QString PreferencesDialog::signalServerLogin() const
{
    return preferences.signalServerLogin();
}

QString PreferencesDialog::signalServerPassword() const
{
    return preferences.signalServerPassword();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void PreferencesDialog::Load()
{
    ActiveFilters = preferences.activeFilters();
    ActiveAllTracks = preferences.activeAllTracks();

    ui->Filters_Video_signalstats->setChecked(ActiveFilters[ActiveFilter_Video_signalstats]);
    ui->Filters_Video_cropdetect->setChecked(ActiveFilters[ActiveFilter_Video_cropdetect]);
    ui->Filters_Video_Psnr->setChecked(ActiveFilters[ActiveFilter_Video_Psnr]);
    ui->Filters_Video_Ssim->setChecked(ActiveFilters[ActiveFilter_Video_Ssim]);
    ui->Filters_Video_Idet->setChecked(ActiveFilters[ActiveFilter_Video_Idet]);
    ui->Filters_Audio_EbuR128->setChecked(ActiveFilters[ActiveFilter_Audio_EbuR128]);
    ui->Filters_Audio_aphasemeter->setChecked(ActiveFilters[ActiveFilter_Audio_aphasemeter]);
    ui->Filters_Audio_astats->setChecked(ActiveFilters[ActiveFilter_Audio_astats]);

    ui->Tracks_Video_First->setChecked(!ActiveAllTracks[Type_Video]);
    ui->Tracks_Video_All->setChecked(ActiveAllTracks[Type_Video]);
    ui->Tracks_Audio_First->setChecked(!ActiveAllTracks[Type_Audio]);
    ui->Tracks_Audio_All->setChecked(ActiveAllTracks[Type_Audio]);

    ui->signalServerUrl_lineEdit->setText(signalServerUrlString());
    ui->signalServerLogin_lineEdit->setText(signalServerLogin());
    ui->signalServerPassword_lineEdit->setText(signalServerPassword());
    ui->signalServerEnable_checkBox->setChecked(isSignalServerEnabled());
}

//---------------------------------------------------------------------------
void PreferencesDialog::Save()
{
    preferences.setActiveFilters(ActiveFilters);
    preferences.setActiveAllTracks(ActiveAllTracks);
    preferences.setSignalServerUrlString(ui->signalServerUrl_lineEdit->text());
    preferences.setSignalServerLogin(ui->signalServerLogin_lineEdit->text());
    preferences.setSignalServerPassword(ui->signalServerPassword_lineEdit->text());
    preferences.setSignalServerEnabled(ui->signalServerEnable_checkBox->isChecked());
    preferences.setSignalServerAutoUploadEnabled(ui->signalServerEnableAutoUpload_checkBox->isChecked());

    preferences.sync();
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

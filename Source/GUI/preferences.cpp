/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "preferences.h"
#include "ui_preferences.h"
#include "SignalServerConnectionChecker.h"
#include <QSettings>
#include <QStandardPaths>
#include <QMetaType>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
//---------------------------------------------------------------------------

#include "qblowfish.h"

// Random key generated at http://www.random.org/bytes/
#define KEY_HEX "911dae7a4ce9a24300efe3b8a4534301"
QByteArray BlowfishKey = QByteArray::fromHex(KEY_HEX);

typedef std::tuple<int, int> GroupAndType;
Q_DECLARE_METATYPE(GroupAndType)

typedef QList<GroupAndType> FilterSelectorsOrder;
Q_DECLARE_METATYPE(FilterSelectorsOrder)

QString KeySignalServerUrl = "SignalServerUrl";
QString KeySignalServerEnable = "SignalServerEnable";
QString KeySignalServerEnableAutoUpload = "SignalServerEnableAutoUpload";
QString KeySignalServerLogin = "SignalServerLogin";
QString KeySignalServerPassword = "SignalServerPassword";

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Preferences::Preferences(SignalServerConnectionChecker* connectionChecker, QWidget *parent) :
    connectionChecker(connectionChecker),
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);

    //Configuration
    connect(this, SIGNAL(accepted()), this, SLOT(OnAccepted()));
    connect(this, SIGNAL(rejected()), this, SLOT(OnRejected()));

    //Loading preferences
    QCoreApplication::setOrganizationName("MediaArea");
    QCoreApplication::setOrganizationDomain("mediaarea.net");
    QCoreApplication::setApplicationName("QCTools");

    Load();

    qRegisterMetaType<GroupAndType>("GroupAndType");
    qRegisterMetaType<FilterSelectorsOrder>("FilterSelectorsOrder");
    qRegisterMetaTypeStreamOperators<FilterSelectorsOrder>("FilterSelectorsOrder");
}

//---------------------------------------------------------------------------
Preferences::~Preferences()
{
    delete ui;
}

QDataStream &operator<<(QDataStream &out, const FilterSelectorsOrder &order) {

    qDebug() << "serializing total " << order.length() << ": \n";

    for(auto item : order) {
        qDebug() << "g: " << std::get<0>(item) << ", t: " << std::get<1>(item);;
    }

    for(auto filterInfo : order)
        out << std::get<0>(filterInfo) << std::get<1>(filterInfo);

    return out;
}
QDataStream &operator>>(QDataStream &in, FilterSelectorsOrder &order) {
    while(!in.atEnd())
    {
        int group;
        int type;
        in >> group;
        in >> type;

        auto entry = std::make_tuple(group, type);
        if(!order.contains(entry))
            order.push_back(entry);
    }

    qDebug() << "deserialized: total " << order.length() << "\n";

    for(auto item : order) {
        qDebug() << "g: " << std::get<0>(item) << ", t: " << std::get<1>(item);
    }

    return in;
}

QList<std::tuple<int, int> > Preferences::loadFilterSelectorsOrder()
{
    QSettings Settings;

    auto order = Settings.value("filterSelectorsOrder", QVariant::fromValue(FilterSelectorsOrder())).value<FilterSelectorsOrder>();

    return order;
}

void Preferences::saveFilterSelectorsOrder(const QList<std::tuple<int, int> > &order)
{
    QSettings Settings;

    Settings.setValue("filterSelectorsOrder", QVariant::fromValue(order));
}

bool Preferences::isSignalServerEnabled() const
{
    QSettings Settings;

    return Settings.value(KeySignalServerEnable, false).toBool();
}

bool Preferences::isSignalServerAutoUploadEnabled() const
{
    QSettings Settings;

    return Settings.value(KeySignalServerEnableAutoUpload, false).toBool();
}

QString Preferences::signalServerUrlString() const
{
    QSettings Settings;

    return Settings.value(KeySignalServerUrl).toString();
}

QString Preferences::signalServerLogin() const
{
    QSettings Settings;

    return Settings.value(KeySignalServerLogin).toString();
}

QString Preferences::signalServerPassword() const
{
    QSettings Settings;
    QString pwd;

    QBlowfish bf(BlowfishKey);
    QByteArray cipherText = Settings.value(KeySignalServerPassword).toByteArray();
    if(cipherText.startsWith(BlowfishKey))
    {
        cipherText.remove(0, BlowfishKey.length());
        bf.setPaddingEnabled(true);
        QByteArray decryptedBa = bf.decrypted(cipherText);

        pwd = QString::fromUtf8(decryptedBa.constData(), decryptedBa.size());
    }
    else
    {
        pwd = Settings.value(KeySignalServerPassword ).toString();
    }

    return pwd;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void Preferences::Load()
{
    QSettings Settings;
    ActiveFilters=activefilters(Settings.value("ActiveFilters", (1<<ActiveFilter_Video_signalstats)|(1<<ActiveFilter_Video_Psnr)|(1<<ActiveFilter_Audio_astats)).toInt());
    ActiveAllTracks=activealltracks(Settings.value("ActiveAllTracks", 0).toInt());

    ui->Filters_Video_signalstats->setChecked(ActiveFilters[ActiveFilter_Video_signalstats]);
    ui->Filters_Video_cropdetect->setChecked(ActiveFilters[ActiveFilter_Video_cropdetect]);
    ui->Filters_Video_Psnr->setChecked(ActiveFilters[ActiveFilter_Video_Psnr]);
    ui->Filters_Video_Ssim->setChecked(ActiveFilters[ActiveFilter_Video_Ssim]);
    ui->Filters_Video_Idet->setChecked(ActiveFilters[ActiveFilter_Video_Idet]);
    ui->Filters_Video_Palettegen->setChecked(ActiveFilters[ActiveFilter_Video_Palettegen]);
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
void Preferences::Save()
{
    QSettings Settings;
    Settings.setValue("ActiveFilters", (uint)ActiveFilters.to_ulong());
    Settings.setValue("ActiveAllTracks", (uint)ActiveAllTracks.to_ulong());

    Settings.setValue(KeySignalServerUrl, ui->signalServerUrl_lineEdit->text());
    Settings.setValue(KeySignalServerLogin, ui->signalServerLogin_lineEdit->text());


    QBlowfish bf(BlowfishKey);
    bf.setPaddingEnabled(true); // Enable padding to be able to encrypt an arbitrary length of bytes
    QByteArray cipherText = bf.encrypted(ui->signalServerPassword_lineEdit->text().toUtf8());

    QVariant pwdValue(BlowfishKey + cipherText);
    Settings.setValue(KeySignalServerPassword, pwdValue);
    Settings.setValue(KeySignalServerEnable, ui->signalServerEnable_checkBox->isChecked());
    Settings.setValue(KeySignalServerEnableAutoUpload, ui->signalServerEnableAutoUpload_checkBox->isChecked());

    Settings.sync();
}

//***************************************************************************
// Slots
//***************************************************************************

//---------------------------------------------------------------------------
void Preferences::OnAccepted()
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
    if (ui->Filters_Video_Palettegen->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Palettegen);
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
void Preferences::OnRejected()
{
    Load();
}

void Preferences::on_testConnection_pushButton_clicked()
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

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/Preferences.h"
#include <QMetaType>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QSettings>

#include "qblowfish.h"

// Random key generated at http://www.random.org/bytes/
#define KEY_HEX "911dae7a4ce9a24300efe3b8a4534301"
QByteArray BlowfishKey = QByteArray::fromHex(KEY_HEX);

QString KeySignalServerUrl = "SignalServerUrl";
QString KeySignalServerEnable = "SignalServerEnable";
QString KeySignalServerEnableAutoUpload = "SignalServerEnableAutoUpload";
QString KeySignalServerLogin = "SignalServerLogin";
QString KeySignalServerPassword = "SignalServerPassword";

QString KeyActiveFilters = "ActiveFilters";
QString KeyActiveAllTracks = "ActiveAllTracks";
QString KeyFilterSelectorsOrder = "filterSelectorsOrder";

Preferences::Preferences(QObject *parent) : QObject(parent)
{
    static struct RegisterMetatypes {
        RegisterMetatypes() {
            qRegisterMetaTypeStreamOperators<FilterSelectorsOrder>("FilterSelectorsOrder");
        }
    } registerMetatypes;

    //Loading preferences
    QCoreApplication::setOrganizationName("MediaArea");
    QCoreApplication::setOrganizationDomain("mediaarea.net");
    QCoreApplication::setApplicationName("QCTools");
}

activefilters Preferences::activeFilters() const
{
    QSettings settings;
    return activefilters(settings.value(KeyActiveFilters, (1<<ActiveFilter_Video_signalstats)|(1<<ActiveFilter_Video_Psnr)|(1<<ActiveFilter_Audio_astats)).toInt());
}

void Preferences::setActiveFilters(const activefilters &filters)
{
    QSettings settings;
    settings.setValue(KeyActiveFilters, (uint) filters.to_ulong());
}

activealltracks Preferences::activeAllTracks() const
{
    QSettings settings;
    return activealltracks(settings.value(KeyActiveAllTracks, 0).toInt());
}

void Preferences::setActiveAllTracks(const activealltracks &alltracks)
{
    QSettings settings;
    settings.setValue(KeyActiveAllTracks, (uint) alltracks.to_ulong());
}

FilterSelectorsOrder Preferences::loadFilterSelectorsOrder()
{
    QSettings settings;
    auto order = settings.value(KeyFilterSelectorsOrder, QVariant::fromValue(FilterSelectorsOrder())).value<FilterSelectorsOrder>();
    return order;
}

void Preferences::saveFilterSelectorsOrder(const FilterSelectorsOrder &order)
{
    QSettings settings;
    settings.setValue(KeyFilterSelectorsOrder, QVariant::fromValue(order));
}

QStringList Preferences::loadSelectedFilters()
{
    QStringList selectedFilters;
    QSettings settings;

    auto selectedFiltersCount = settings.beginReadArray("selectedFilters");
    for (int selectedFiltersIndex = 0; selectedFiltersIndex < selectedFiltersCount; ++selectedFiltersIndex) {
        settings.setArrayIndex(selectedFiltersIndex);
        selectedFilters.append(settings.value("filterName").toString());
    }
    settings.endArray();

    return selectedFilters;
}

void Preferences::saveSelectedFilters(const QStringList &filters)
{
    QSettings settings;
    settings.beginWriteArray("selectedFilters");
    for (int i = 0; i < filters.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("filterName", filters.at(i));
    }
    settings.endArray();
}

bool Preferences::isSignalServerEnabled() const
{
    QSettings settings;
    return settings.value(KeySignalServerEnable, false).toBool();
}

void Preferences::setSignalServerEnabled(bool enabled)
{
    QSettings settings;
    settings.setValue(KeySignalServerEnable, enabled);
}

bool Preferences::isSignalServerAutoUploadEnabled() const
{
    QSettings settings;
    return settings.value(KeySignalServerEnableAutoUpload, false).toBool();
}

void Preferences::setSignalServerAutoUploadEnabled(bool enabled)
{
    QSettings settings;
    settings.setValue(KeySignalServerEnableAutoUpload, enabled);
}

QString Preferences::signalServerUrlString() const
{
    QSettings settings;
    return settings.value(KeySignalServerUrl).toString();
}

void Preferences::setSignalServerUrlString(const QString &urlString)
{
    QSettings settings;
    return settings.setValue(KeySignalServerUrl, urlString);
}

QString Preferences::signalServerLogin() const
{
    QSettings settings;
    return settings.value(KeySignalServerLogin).toString();
}

void Preferences::setSignalServerLogin(const QString &login)
{
    QSettings settings;
    return settings.setValue(KeySignalServerLogin, login);
}

QString Preferences::signalServerPassword() const
{
    QSettings settings;
    QString pwd;

    QBlowfish bf(BlowfishKey);
    QByteArray cipherText = settings.value(KeySignalServerPassword).toByteArray();
    if(cipherText.startsWith(BlowfishKey))
    {
        cipherText.remove(0, BlowfishKey.length());
        bf.setPaddingEnabled(true);
        QByteArray decryptedBa = bf.decrypted(cipherText);

        pwd = QString::fromUtf8(decryptedBa.constData(), decryptedBa.size());
    }
    else
    {
        pwd = settings.value(KeySignalServerPassword ).toString();
    }

    return pwd;
}

void Preferences::setSignalServerPassword(const QString &password)
{
    QSettings settings;

    QBlowfish bf(BlowfishKey);
    bf.setPaddingEnabled(true); // Enable padding to be able to encrypt an arbitrary length of bytes
    QByteArray cipherText = bf.encrypted(password.toUtf8());

    QVariant pwdValue(BlowfishKey + cipherText);
    settings.setValue(KeySignalServerPassword, pwdValue);
}

QStringList Preferences::recentFiles() const
{
    QSettings settings;
    QStringList recentFiles;

    auto recentFilesCount = settings.beginReadArray("recentFiles");
    for (int recentFileIndex = 0; recentFileIndex < recentFilesCount; ++recentFileIndex) {
        settings.setArrayIndex(recentFileIndex);
        recentFiles.append(settings.value("file").toString());
    }
    settings.endArray();

    return recentFiles;
}

void Preferences::setRecentFiles(const QStringList &recentFiles)
{
    QSettings settings;

    auto recentFilesCount = recentFiles.size();
    settings.beginWriteArray("recentFiles");
    for (int recentFileIndex = 0; recentFileIndex < recentFilesCount; ++recentFileIndex) {
        settings.setArrayIndex(recentFileIndex);
        settings.setValue("file", recentFiles.at(recentFileIndex));
    }
    settings.endArray();
}

void Preferences::sync()
{
    QSettings settings;
    settings.sync();
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

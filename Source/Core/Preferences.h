/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Preferences_H
#define Preferences_H
//---------------------------------------------------------------------------

#include "Core/Core.h"

#include <QObject>
#include <QList>
#include <tuple>

typedef std::tuple<quint64, quint64> GroupAndType;
typedef QList<GroupAndType> FilterSelectorsOrder;

struct PanelInfo {
    QString name;
    QString yaxis;
    QString filterchain;
    QString version;
    QString legend;
    int panelType; // AVMEDIA_TYPE_VIDEO / AVMEDIA_TYPE_AUDIO
};

class Preferences : public QObject
{
public:
    Preferences(QObject* parent = NULL);

    activefilters activeFilters() const;
    void setActiveFilters(const activefilters& filters);

    activealltracks activeAllTracks() const;
    void setActiveAllTracks(const activealltracks& alltracks);

    QMap<QString, std::tuple<QString, QString, QString, QString, int>> getActivePanels() const;

    QSet<QString> activePanels() const;
    void setActivePanels(const QSet<QString>& activePanels);

    QList<PanelInfo> availablePanels() const;

    FilterSelectorsOrder loadFilterSelectorsOrder();
    void saveFilterSelectorsOrder(const FilterSelectorsOrder& order);

    QStringList loadSelectedFilters();
    void saveSelectedFilters(const QStringList& filters);

    bool isSignalServerEnabled() const;
    void setSignalServerEnabled(bool enabled);

    bool isSignalServerAutoUploadEnabled() const;
    void setSignalServerAutoUploadEnabled(bool enabled);

    QString QCvaultPathString(bool* IssueWithDefaultQCvaultPath = nullptr) const;
    void setQCvaultPathString(const QString & urlString);
    QString defaultQCvaultPathString() const;
    QString createQCvaultFileNameString(const QString & input, const QString & cacheDir = QString()) const;

    QString signalServerUrlString() const;
    void setSignalServerUrlString(const QString & urlString);

    QString signalServerLogin() const;
    void setSignalServerLogin(const QString& login);

    QString signalServerPassword() const;
    void setSignalServerPassword(const QString& password);

    QStringList recentFiles() const;
    void setRecentFiles(const QStringList& recentFiles);

    void sync();
};

Q_DECLARE_METATYPE(GroupAndType)
Q_DECLARE_METATYPE(FilterSelectorsOrder)

#endif // Preferences_H

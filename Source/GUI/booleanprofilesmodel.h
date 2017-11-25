#ifndef BOOLEANPROFILESMODEL_H
#define BOOLEANPROFILESMODEL_H

#include <QDir>
#include <QObject>
#include <QStandardItemModel>

class BooleanProfilesModel : public QStandardItemModel {
public:
    enum Roles {
        Data = Qt::UserRole,
        Display = Qt::UserRole + 1,
        IsSystem = Qt::UserRole + 2
    };

    BooleanProfilesModel(QObject* parent, const QString& profilesLocation);

    QVariant data(const QModelIndex &index, int role) const;

    void append(const QString& path, const QString& name, bool isSystem);

    QString absoluteProfilesPath() const;

private:
    QString m_absoluteProfilesPath;
};

#endif // BOOLEANPROFILESMODEL_H

#include "booleanprofilesmodel.h"

BooleanProfilesModel::BooleanProfilesModel(QObject *parent) : QStandardItemModel(parent) {
    // system profiles
    {
        QDir dir(":/boolean_profiles");
        auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
        for(auto entry : entries) {
            append(entry.filePath(), entry.fileName(), true);
        }
    }

    // user profiles
    {
        QDir dir;
        auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
        for(auto entry : entries) {
            append(entry.filePath(), entry.fileName(), false);
        }
    }
}

QVariant BooleanProfilesModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole) {
        bool isSystem = QStandardItemModel::data(index, IsSystem).toBool();
        return isSystem ? QStandardItemModel::data(index, Display).toString() + " (system)" : QStandardItemModel::data(index, Display).toString();
    }

    if(role == Qt::EditRole)
        return QStandardItemModel::data(index, Data);

    return QStandardItemModel::data(index, role);
}

void BooleanProfilesModel::append(const QString &path, const QString &name, bool isSystem) {
    auto data = new QStandardItem;
    data->setData(path, Data);
    data->setData(name, Display);
    data->setData(isSystem, IsSystem);

    appendRow(data);
}

#include "managebooleanconditions.h"
#include "ui_managebooleanconditions.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrl>

ManageBooleanConditions::ManageBooleanConditions(BooleanProfilesModel *model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageBooleanConditions)
{
    ui->setupUi(this);
    ui->listView->setModel(model);
}

ManageBooleanConditions::~ManageBooleanConditions()
{
    delete ui;
}

QString ManageBooleanConditions::getProfileName(const QString& defaultProfileFileName)
{
    QInputDialog dialog(this);
    dialog.setWindowTitle("Enter profile name");
    dialog.setLabelText("Profile name:");
    dialog.setTextValue(defaultProfileFileName);
    dialog.setMinimumWidth(200);

    if(QDialog::Accepted != dialog.exec())
        return QString();

    auto profileName = dialog.textValue();
    if(profileName.isEmpty()) {
        profileName = defaultProfileFileName;
    }

    if(!profileName.endsWith(".json")) {
        profileName = profileName + ".json";
    }

    return profileName;
}

QString ManageBooleanConditions::pickDefaultProfileName()
{
    BooleanProfilesModel* model = static_cast<BooleanProfilesModel*> (ui->listView->model());

    QString defaultProfileName = "profile";

    QString defaultProfileFileName;
    size_t number = 1;
    while(number < 100) {
        defaultProfileFileName = defaultProfileName + QString::number(number) + ".json";

        for(auto i = 0; i < ui->listView->model()->rowCount(); ++i) {
            auto existingProfileName = model->item(i, 0)->data(BooleanProfilesModel::Display).toString();
            if(defaultProfileFileName == existingProfileName) {
                defaultProfileFileName.clear();
                break;
            }
        }

        if(defaultProfileFileName.isEmpty())
            ++number;
        else
            break;
    }

    return defaultProfileFileName;
}

void ManageBooleanConditions::on_add_pushButton_clicked()
{
    BooleanProfilesModel* model = static_cast<BooleanProfilesModel*> (ui->listView->model());

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileName = getProfileName(defaultProfileFileName);

    if(profileName.isEmpty())
        return;

    if(QFileInfo(profileName).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileName);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileName));
            } else {
                Q_EMIT newProfile(profileName);
            }
        }
    } else {
        model->append(profileName, profileName, false);
        Q_EMIT newProfile(profileName);
    }
}

void ManageBooleanConditions::on_copy_pushButton_clicked()
{
    BooleanProfilesModel* model = static_cast<BooleanProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFileName = model->item(selectedRow, 0)->data(BooleanProfilesModel::Data).toString();

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileName = getProfileName(defaultProfileFileName);

    if(profileName.isEmpty() || profileName == selectedProfileFileName)
        return;

    if(QFileInfo(profileName).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileName);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileName));
            } else {
                if(QFile::copy(selectedProfileFileName, profileName)) {
                    QFile file(profileName);
                    file.setPermissions(QFile::WriteOther);
                    Q_EMIT profileUpdated(profileName);
                } else {
                    QMessageBox::warning(this, "Warning", QString("Failed to copy profile %1 to %2").arg(selectedProfileFileName).arg(profileName));
                }
            }
        }
    } else {
        if(QFile::copy(selectedProfileFileName, profileName)) {
            QFile file(profileName);
            file.setPermissions(QFile::WriteOther);
            model->append(profileName, profileName, false);
            Q_EMIT profileUpdated(profileName);
        } else {
            QMessageBox::warning(this, "Warning", QString("Failed to copy profile %1 to %2").arg(selectedProfileFileName).arg(profileName));
        }
    }

}

void ManageBooleanConditions::on_remove_pushButton_clicked()
{
    BooleanProfilesModel* model = static_cast<BooleanProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFileName = model->item(selectedRow, 0)->data(BooleanProfilesModel::Data).toString();

    QFile profile(selectedProfileFileName);
    if(profile.remove()) {
        model->removeRow(selectedRow);
    } else {
        QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(selectedProfileFileName));
    }
}

void ManageBooleanConditions::on_rename_pushButton_clicked()
{
    BooleanProfilesModel* model = static_cast<BooleanProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFileName = model->item(selectedRow, 0)->data(BooleanProfilesModel::Data).toString();

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileName = getProfileName(defaultProfileFileName);

    if(profileName.isEmpty() || profileName == selectedProfileFileName)
        return;

    if(QFileInfo(profileName).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileName);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileName));
            } else {
                if(QFile::rename(selectedProfileFileName, profileName)) {
                    model->removeRow(selectedRow);
                    Q_EMIT profileUpdated(profileName);
                } else {
                    QMessageBox::warning(this, "Warning", QString("Failed to rename profile %1 to %2").arg(selectedProfileFileName).arg(profileName));
                }
            }
        }
    } else {
        if(QFile::rename(selectedProfileFileName, profileName)) {
            model->removeRow(selectedRow);
            model->append(profileName, profileName, false);
            Q_EMIT profileUpdated(profileName);
        } else {
            QMessageBox::warning(this, "Warning", QString("Failed to rename profile %1 to %2").arg(selectedProfileFileName).arg(profileName));
        }
    }
}

void ManageBooleanConditions::on_openLocation_pushButton_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile("."));
}

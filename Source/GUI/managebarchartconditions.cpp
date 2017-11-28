#include "managebarchartconditions.h"
#include "ui_managebarchartconditions.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrl>

ManageBarchartConditions::ManageBarchartConditions(BarchartProfilesModel *model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageBarchartConditions)
{
    ui->setupUi(this);
    ui->listView->setModel(model);
    ui->profileLocationUrl_label->setText(QString("<a href='%1'>%2</a>")
                                          .arg(QUrl::fromLocalFile(model->absoluteProfilesPath()).toString()).arg(model->absoluteProfilesPath()));
}

ManageBarchartConditions::~ManageBarchartConditions()
{
    delete ui;
}

QString ManageBarchartConditions::getProfileName(const QString& defaultProfileFileName)
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

QString ManageBarchartConditions::pickDefaultProfileName()
{
    auto model = static_cast<BarchartProfilesModel*> (ui->listView->model());

    QString defaultProfileName = "profile";

    QString defaultProfileFileName;
    size_t number = 1;
    while(number < 100) {
        defaultProfileFileName = defaultProfileName + QString::number(number) + ".json";

        for(auto i = 0; i < ui->listView->model()->rowCount(); ++i) {
            auto existingProfileName = model->item(i, 0)->data(BarchartProfilesModel::Display).toString();
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

void ManageBarchartConditions::on_add_pushButton_clicked()
{
    auto model = static_cast<BarchartProfilesModel*> (ui->listView->model());

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileFileName = getProfileName(defaultProfileFileName);
    QString profileFilePath = model->absoluteProfilesPath() + "/" + profileFileName;

    if(profileFileName.isEmpty())
        return;

    if(QFileInfo(profileFileName).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileFileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileFileName);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileFileName));
            } else {
                Q_EMIT newProfile(profileFilePath);
            }
        }
    } else {
        model->append(profileFilePath, profileFileName, false);
        Q_EMIT newProfile(profileFilePath);
    }
}

void ManageBarchartConditions::on_copy_pushButton_clicked()
{
    auto model = static_cast<BarchartProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFilePath = model->item(selectedRow, 0)->data(BarchartProfilesModel::Data).toString();

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileFileName = getProfileName(defaultProfileFileName);
    QString profileFilePath = model->absoluteProfilesPath() + "/" + profileFileName;

    if(profileFileName.isEmpty() || profileFilePath == selectedProfileFilePath)
        return;

    if(QFileInfo(profileFilePath).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileFileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileFilePath);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileFilePath));
            } else {
                if(QFile::copy(selectedProfileFilePath, profileFilePath)) {
                    QFile file(profileFilePath);
                    file.setPermissions(file.permissions() | QFile::WriteOwner);
                    Q_EMIT profileUpdated(profileFilePath);
                } else {
                    QMessageBox::warning(this, "Warning", QString("Failed to copy profile %1 to %2").arg(selectedProfileFilePath).arg(profileFilePath));
                }
            }
        }
    } else {
        if(QFile::copy(selectedProfileFilePath, profileFilePath)) {
            QFile file(profileFilePath);
            file.setPermissions(file.permissions() | QFile::WriteOwner);
            model->append(profileFilePath, profileFileName, false);
            Q_EMIT profileUpdated(profileFilePath);
        } else {
            QMessageBox::warning(this, "Warning", QString("Failed to copy profile %1 to %2").arg(selectedProfileFilePath).arg(profileFilePath));
        }
    }

}

void ManageBarchartConditions::on_remove_pushButton_clicked()
{
    auto model = static_cast<BarchartProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFileName = model->item(selectedRow, 0)->data(BarchartProfilesModel::Data).toString();

    QFile profile(selectedProfileFileName);
    if(profile.remove()) {
        model->removeRow(selectedRow);
    } else {
        QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(selectedProfileFileName));
    }
}

void ManageBarchartConditions::on_rename_pushButton_clicked()
{
    auto model = static_cast<BarchartProfilesModel*> (ui->listView->model());
    if(ui->listView->selectionModel()->selectedRows().empty())
        return;

    auto selectedRow = ui->listView->selectionModel()->selectedRows().first().row();
    auto selectedProfileFilePath = model->item(selectedRow, 0)->data(BarchartProfilesModel::Data).toString();

    QString defaultProfileFileName = pickDefaultProfileName();
    QString profileFileName = getProfileName(defaultProfileFileName);
    QString profileFilePath = model->absoluteProfilesPath() + "/" + profileFileName;

    if(profileFileName.isEmpty() || profileFileName == selectedProfileFilePath)
        return;

    if(QFileInfo(profileFilePath).exists()) {
        auto answer = QMessageBox::question(this, "Warning", QString("Profile %1 already exists. Do you want to overwrite it?").arg(profileFileName));
        if(answer == QMessageBox::Yes) {
            QFile file(profileFilePath);
            if(!file.remove()) {
                QMessageBox::warning(this, "Warning", QString("Failed to delete profile %1").arg(profileFileName));
            } else {
                if(QFile::rename(selectedProfileFilePath, profileFilePath)) {
                    model->removeRow(selectedRow);
                    Q_EMIT profileUpdated(profileFilePath);
                } else {
                    QMessageBox::warning(this, "Warning", QString("Failed to rename profile %1 to %2").arg(selectedProfileFilePath).arg(profileFileName));
                }
            }
        }
    } else {
        if(QFile::rename(selectedProfileFilePath, profileFilePath)) {
            model->removeRow(selectedRow);
            model->append(profileFilePath, profileFileName, false);
            Q_EMIT profileUpdated(profileFilePath);
        } else {
            QMessageBox::warning(this, "Warning", QString("Failed to rename profile %1 to %2").arg(selectedProfileFilePath).arg(profileFileName));
        }
    }
}

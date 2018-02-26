#ifndef MANAGEBARCHARTCONDITIONS_H
#define MANAGEBARCHARTCONDITIONS_H

#include "barchartprofilesmodel.h"

#include <QDialog>

namespace Ui {
class ManageBarchartConditions;
}

class ManageBarchartConditions : public QDialog
{
    Q_OBJECT

public:
    explicit ManageBarchartConditions(BarchartProfilesModel* model, const QModelIndex& selected, QWidget *parent = 0);
    ~ManageBarchartConditions();

    QString pickDefaultProfileName();
    QString getProfileName(const QString& defaultProfileName);

Q_SIGNALS:
    void profileUpdated(const QString& profileName);
    void newProfile(const QString& profileName);

private Q_SLOTS:
    void on_add_pushButton_clicked();

    void on_copy_pushButton_clicked();

    void on_remove_pushButton_clicked();

    void on_rename_pushButton_clicked();

private:
    Ui::ManageBarchartConditions *ui;
};

#endif // MANAGEBARCHARTCONDITIONS_H

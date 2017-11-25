#ifndef MANAGEBOOLEANCONDITIONS_H
#define MANAGEBOOLEANCONDITIONS_H

#include "booleanprofilesmodel.h"

#include <QDialog>

namespace Ui {
class ManageBooleanConditions;
}

class ManageBooleanConditions : public QDialog
{
    Q_OBJECT

public:
    explicit ManageBooleanConditions(BooleanProfilesModel* model, QWidget *parent = 0);
    ~ManageBooleanConditions();

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

    void on_openLocation_pushButton_clicked();

private:
    Ui::ManageBooleanConditions *ui;
};

#endif // MANAGEBOOLEANCONDITIONS_H

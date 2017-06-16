#ifndef COMMENTSEDITOR_H
#define COMMENTSEDITOR_H

#include <QDialog>
#include <QDialogButtonBox>

namespace Ui {
class CommentsEditor;
}

class CommentsEditor : public QDialog
{
    Q_OBJECT

public:
    explicit CommentsEditor(QWidget *parent = 0);
    ~CommentsEditor();

    void setLabelText(const QString& labelText);
    void setTextValue(const QString& textValue);
    QString textValue() const;

    QDialogButtonBox* buttons() const;

private Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton* button);

private:
    Ui::CommentsEditor *ui;
};

#endif // COMMENTSEDITOR_H

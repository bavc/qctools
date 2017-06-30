#include "CommentsEditor.h"
#include "ui_CommentsEditor.h"
#include <QPushButton>

CommentsEditor::CommentsEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommentsEditor)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Discard)->setText("Delete");
    ui->buttonBox->button(QDialogButtonBox::Discard)->setVisible(false);

    ui->plainTextEdit->setFocus();
}

CommentsEditor::~CommentsEditor()
{
    delete ui;
}

void CommentsEditor::setLabelText(const QString &labelText)
{
    ui->label->setText(labelText);
}

void CommentsEditor::setTextValue(const QString &textValue)
{
    ui->plainTextEdit->setPlainText(textValue);
}

QString CommentsEditor::textValue() const
{
    return ui->plainTextEdit->toPlainText();
}

QDialogButtonBox *CommentsEditor::buttons() const
{
    return ui->buttonBox;
}

void CommentsEditor::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Return && event->modifiers() == Qt::AltModifier)
    {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(ui->buttonBox, &e);
    }

    QDialog::keyPressEvent(event);
}

void CommentsEditor::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button == ui->buttonBox->button(QDialogButtonBox::Discard))
        done(QDialogButtonBox::DestructiveRole);
}

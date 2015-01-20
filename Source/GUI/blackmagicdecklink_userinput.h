#ifndef BLACKMAGIC_DECKLINK_USERINPUT_H
#define BLACKMAGIC_DECKLINK_USERINPUT_H

#include <QDialog>

namespace Ui {
class BlackmagicDeckLink_UserInput;
}

class BlackmagicDeckLink_UserInput : public QDialog
{
    Q_OBJECT

public:
    explicit BlackmagicDeckLink_UserInput(QWidget *parent = 0);
    ~BlackmagicDeckLink_UserInput();

    int                         TC_in;
    int                         TC_out;
    QString                     Encoding_FileName;

private:
    Ui::BlackmagicDeckLink_UserInput *ui;

private Q_SLOTS:

    void on_accepted();
    void on_Record_GroupBox_toggled(bool on);
};

#endif // BLACKMAGIC_DECKLINK_USERINPUT_H

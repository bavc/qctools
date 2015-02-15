#ifndef BLACKMAGIC_DECKLINK_USERINPUT_H
#define BLACKMAGIC_DECKLINK_USERINPUT_H

#include "Core/BlackmagicDeckLink_Glue.h"

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

    BlackmagicDeckLink_Glue*    Card;
    QString                     Encoding_FileName;
    QString                     Encoding_Format;

    // Callbacks
    void                        TimeCode_IsAvailable();

private:
    Ui::BlackmagicDeckLink_UserInput *ui;

    bool                        In_InModification;
    bool                        Out_InModification;

private Q_SLOTS:

    void accept();
    void on_Record_Group_toggled(bool on);
    void on_CardsList_Value_currentIndexChanged(int Pos);
    void on_Deck_Source_Value_currentIndexChanged(int Pos);
    void on_In_Now_Label_toggled(bool checked);
    void on_In_Timecode_Label_toggled(bool checked);
    void on_In_Timecode_HH_valueChanged(int i);
    void on_In_Timecode_MM_valueChanged(int i);
    void on_In_Timecode_SS_valueChanged(int i);
    void on_In_Timecode_FF_valueChanged(int i);
    void on_In_Timecode_XX_valueChanged();
    void on_Out_FrameCount_Label_toggled(bool checked);
    void on_Out_FrameCount_Value_valueChanged(int i);
    void on_Out_DurationTC_Label_toggled(bool checked);
    void on_Out_DurationTC_HH_valueChanged(int i);
    void on_Out_DurationTC_MM_valueChanged(int i);
    void on_Out_DurationTC_SS_valueChanged(int i);
    void on_Out_DurationTC_FF_valueChanged(int i);
    void on_Out_DurationTC_XX_valueChanged();
    void on_Out_DurationTS_Label_toggled(bool checked);
    void on_Out_DurationTS_HH_valueChanged(int i);
    void on_Out_DurationTS_MM_valueChanged(int i);
    void on_Out_DurationTS_SS_valueChanged(int i);
    void on_Out_DurationTS_mmm_valueChanged(int i);
    void on_Out_DurationTS_XX_valueChanged();
    void on_Out_Timecode_Label_toggled(bool checked);
    void on_Out_Timecode_HH_valueChanged(int i);
    void on_Out_Timecode_MM_valueChanged(int i);
    void on_Out_Timecode_SS_valueChanged(int i);
    void on_Out_Timecode_FF_valueChanged(int i);
    void on_Out_Timecode_XX_valueChanged();
    void on_Record_DirectoryName_Dialog_pressed();
};

#endif // BLACKMAGIC_DECKLINK_USERINPUT_H

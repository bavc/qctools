#include "blackmagicdecklink_userinput.h"
#include "ui_blackmagicdecklink_userinput.h"

#include <QStandardPaths>

BlackmagicDeckLink_UserInput::BlackmagicDeckLink_UserInput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlackmagicDeckLink_UserInput)
{
    ui->setupUi(this);

    ui->In_1->setValue(1);
    ui->In_2->setValue(14);
    ui->Out_1->setValue(1);
    ui->Out_2->setValue(14);
    ui->Out_3->setValue(15);

    connect( this, SIGNAL( accepted() ), this, SLOT( on_accepted() ) );
    connect( ui->Record_GroupBox, SIGNAL( toggled(bool) ), this, SLOT( on_Record_GroupBox_toggled(bool) ) );
}

BlackmagicDeckLink_UserInput::~BlackmagicDeckLink_UserInput()
{
    delete ui;
}

void BlackmagicDeckLink_UserInput::on_accepted()
{
    TC_in=0;
    TC_in+=(ui->In_1->value()/10)<<28;
    TC_in+=(ui->In_1->value()%10)<<24;
    TC_in+=(ui->In_2->value()/10)<<20;
    TC_in+=(ui->In_2->value()%10)<<16;
    TC_in+=(ui->In_3->value()/10)<<12;
    TC_in+=(ui->In_3->value()%10)<< 8;
    TC_in+=(ui->In_4->value()/10)<< 4;
    TC_in+=(ui->In_4->value()%10)<< 0;
    TC_out=0;
    TC_out+=(ui->Out_1->value()/10)<<28;
    TC_out+=(ui->Out_1->value()%10)<<24;
    TC_out+=(ui->Out_2->value()/10)<<20;
    TC_out+=(ui->Out_2->value()%10)<<16;
    TC_out+=(ui->Out_3->value()/10)<<12;
    TC_out+=(ui->Out_3->value()%10)<< 8;
    TC_out+=(ui->Out_4->value()/10)<< 4;
    TC_out+=(ui->Out_4->value()%10)<< 0;

    if (ui->Record_GroupBox->isChecked())
        Encoding_FileName=ui->Encoding_FileName_Line->text();
}

void BlackmagicDeckLink_UserInput::on_Record_GroupBox_toggled(bool on)
{
    if (on && ui->Encoding_FileName_Line->text().isEmpty())
        ui->Encoding_FileName_Line->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)+"/QCTools_Capture.mov");
}

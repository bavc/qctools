#include "blackmagicdecklink_userinput.h"
#include "ui_blackmagicdecklink_userinput.h"

#include <QStandardPaths>
#include <QThread>

BlackmagicDeckLink_UserInput::BlackmagicDeckLink_UserInput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlackmagicDeckLink_UserInput),
    FrameCount(0),
    Card(NULL)
{
    ui->setupUi(this);

    ui->In_1->setValue(1);
    ui->In_2->setValue(14);
    ui->Out_1->setValue(1);
    ui->Out_2->setValue(14);
    ui->Out_3->setValue(15);

    connect( this, SIGNAL( accepted() ), this, SLOT( on_accepted() ) );
    connect( ui->Record_GroupBox, SIGNAL( toggled(bool) ), this, SLOT( on_Record_GroupBox_toggled(bool) ) );
    connect( ui->CardsList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CardsList_currentIndexChanged(int)));

    // Deck menu
    std::vector<std::string> List=BlackmagicDeckLink_Glue::CardsList();
    for (size_t Pos = 0; Pos<List.size(); Pos++)
        ui->CardsList->addItem(List[Pos].c_str());
    on_CardsList_currentIndexChanged(0);
}

BlackmagicDeckLink_UserInput::~BlackmagicDeckLink_UserInput()
{
    delete ui;
}

void BlackmagicDeckLink_UserInput::on_accepted()
{
    if (!Card)
        return;

    Card->TC_in=0;
    Card->TC_in+=(ui->In_1->value()/10)<<28;
    Card->TC_in+=(ui->In_1->value()%10)<<24;
    Card->TC_in+=(ui->In_2->value()/10)<<20;
    Card->TC_in+=(ui->In_2->value()%10)<<16;
    Card->TC_in+=(ui->In_3->value()/10)<<12;
    Card->TC_in+=(ui->In_3->value()%10)<< 8;
    Card->TC_in+=(ui->In_4->value()/10)<< 4;
    Card->TC_in+=(ui->In_4->value()%10)<< 0;
    Card->TC_out=0;
    Card->TC_out+=(ui->Out_1->value()/10)<<28;
    Card->TC_out+=(ui->Out_1->value()%10)<<24;
    Card->TC_out+=(ui->Out_2->value()/10)<<20;
    Card->TC_out+=(ui->Out_2->value()%10)<<16;
    Card->TC_out+=(ui->Out_3->value()/10)<<12;
    Card->TC_out+=(ui->Out_3->value()%10)<< 8;
    Card->TC_out+=(ui->Out_4->value()/10)<< 4;
    Card->TC_out+=(ui->Out_4->value()%10)<< 0;

    if (ui->Record_GroupBox->isChecked())
        Encoding_FileName=ui->Encoding_FileName_Line->text();

    int FrameCount_In, FrameCount_Out;
    GET_FRAME_COUNT(FrameCount_In, Card->TC_in, 30, 1);
    GET_FRAME_COUNT(FrameCount_Out, Card->TC_out, 30, 1);
    FrameCount=FrameCount_Out-FrameCount_In;
}

void BlackmagicDeckLink_UserInput::on_Record_GroupBox_toggled(bool on)
{
    if (on && ui->Encoding_FileName_Line->text().isEmpty())
        ui->Encoding_FileName_Line->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)+"/QCTools_Capture.mov");
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_CardsList_currentIndexChanged(int Pos)
{
    delete Card;
    Card=new BlackmagicDeckLink_Glue(Pos);
    int TC;
    while ((TC=Card->CurrentTimecode())==-1)
        thread()->yieldCurrentThread();
    ui->In_1->setValue(((TC>>28)&0xF)*10+((TC>>24)&0xF));
    ui->In_2->setValue(((TC>>20)&0xF)*10+((TC>>16)&0xF));
    ui->In_3->setValue(((TC>>12)&0xF)*10+((TC>>8)&0xF));
    ui->In_4->setValue(((TC>>4)&0xF)*10+((TC)&0xF));
    ui->Out_1->setValue(((TC>>28)&0xF)*10+((TC>>24)&0xF));
    ui->Out_2->setValue(((TC>>20)&0xF)*10+((TC>>16)&0xF));
    ui->Out_3->setValue(((TC>>12)&0xF)*10+((TC>>8)&0xF)+10); //TODO: better handling of timecodes
    ui->Out_4->setValue(((TC>>4)&0xF)*10+((TC)&0xF));
}

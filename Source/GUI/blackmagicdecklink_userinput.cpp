#include "blackmagicdecklink_userinput.h"
#include "ui_blackmagicdecklink_userinput.h"

#include <QStandardPaths>
#include <QThread>

void __stdcall BlackmagicDeckLink_UserInput_TimeCodeCallback(void* Private)
{
    BlackmagicDeckLink_UserInput* CallBack = (BlackmagicDeckLink_UserInput*)Private;
    CallBack->TimeCode_IsAvailable();
}

BlackmagicDeckLink_UserInput::BlackmagicDeckLink_UserInput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlackmagicDeckLink_UserInput),
    TC_Now(-1),
    Card(NULL)
{
    ui->setupUi(this);

    ui->In_1->setVisible(false);
    ui->In_2->setVisible(false);
    ui->In_3->setVisible(false);
    ui->In_4->setVisible(false);
    ui->Out_1->setVisible(false);
    ui->Out_2->setVisible(false);
    ui->Out_3->setVisible(false);
    ui->Out_4->setVisible(false);

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

    if (ui->In_Timecode->isChecked())
    {
        Card->Config_In.TC_in=0;
        Card->Config_In.TC_in+=(ui->In_1->value()/10)<<28;
        Card->Config_In.TC_in+=(ui->In_1->value()%10)<<24;
        Card->Config_In.TC_in+=(ui->In_2->value()/10)<<20;
        Card->Config_In.TC_in+=(ui->In_2->value()%10)<<16;
        Card->Config_In.TC_in+=(ui->In_3->value()/10)<<12;
        Card->Config_In.TC_in+=(ui->In_3->value()%10)<< 8;
        Card->Config_In.TC_in+=(ui->In_4->value()/10)<< 4;
        Card->Config_In.TC_in+=(ui->In_4->value()%10)<< 0;
    }
    else if (ui->Out_Timecode->isChecked())
    {
        Card->Config_In.TC_in=TC_Now;
    }
    else
    {
        Card->Config_In.TC_in=-1;
    }

    if (ui->Out_Timecode->isChecked())
    {
        Card->Config_In.TC_out=0;
        Card->Config_In.TC_out+=(ui->Out_1->value()/10)<<28;
        Card->Config_In.TC_out+=(ui->Out_1->value()%10)<<24;
        Card->Config_In.TC_out+=(ui->Out_2->value()/10)<<20;
        Card->Config_In.TC_out+=(ui->Out_2->value()%10)<<16;
        Card->Config_In.TC_out+=(ui->Out_3->value()/10)<<12;
        Card->Config_In.TC_out+=(ui->Out_3->value()%10)<< 8;
        Card->Config_In.TC_out+=(ui->Out_4->value()/10)<< 4;
        Card->Config_In.TC_out+=(ui->Out_4->value()%10)<< 0;
        Card->Config_In.FrameCount=-1;
    }
    else if (ui->In_Timecode->isChecked())
    {
        Card->Config_In.TC_out=-0x23595929;
        Card->Config_In.FrameCount = ui->Out_Frames->text().toInt();
    }
    else
    {
        Card->Config_In.TC_out=-1;
        Card->Config_In.FrameCount = ui->Out_Frames->text().toInt();
    }

    if (ui->Record_GroupBox->isChecked())
        Encoding_FileName=ui->Encoding_FileName_Line->text();

    if (Card->Config_In.FrameCount==-1)
    {
        int FrameCount_In, FrameCount_Out;
        GET_FRAME_COUNT(FrameCount_In, Card->Config_In.TC_in, 30, 1);
        GET_FRAME_COUNT(FrameCount_Out, Card->Config_In.TC_out, 30, 1);
        Card->Config_In.FrameCount=FrameCount_Out-FrameCount_In;
    }
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

    Card->Config_In.TimeCodeIsAvailable_Callback=BlackmagicDeckLink_UserInput_TimeCodeCallback;
    Card->Config_In.TimeCodeIsAvailable_Private=this;

    Card->CurrentTimecode(); // TimeCode_IsAvailable() will be called when available
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::TimeCode_IsAvailable()
{
    TC_Now=Card->Config_Out.TC_current;
    
    ui->In_1->setValue(((Card->Config_Out.TC_current>>28)&0xF)*10+((Card->Config_Out.TC_current>>24)&0xF));
    ui->In_2->setValue(((Card->Config_Out.TC_current>>20)&0xF)*10+((Card->Config_Out.TC_current>>16)&0xF));
    ui->In_3->setValue(((Card->Config_Out.TC_current>>12)&0xF)*10+((Card->Config_Out.TC_current>>8)&0xF));
    ui->In_4->setValue(((Card->Config_Out.TC_current>>4)&0xF)*10+((Card->Config_Out.TC_current)&0xF));
    ui->Out_1->setValue(((Card->Config_Out.TC_current>>28)&0xF)*10+((Card->Config_Out.TC_current>>24)&0xF));
    ui->Out_2->setValue(((Card->Config_Out.TC_current>>20)&0xF)*10+((Card->Config_Out.TC_current>>16)&0xF));
    ui->Out_3->setValue(((Card->Config_Out.TC_current>>12)&0xF)*10+((Card->Config_Out.TC_current>>8)&0xF)+10); //TODO: better handling of timecodes
    ui->Out_4->setValue(((Card->Config_Out.TC_current>>4)&0xF)*10+((Card->Config_Out.TC_current)&0xF));

    ui->In_Timecode->setText("Specific timecode:");
    ui->In_Timecode->setEnabled(true);
    ui->Out_Timecode->setText("Specific timecode:");
    ui->Out_Timecode->setEnabled(true);

    ui->In_1->setVisible(true);
    ui->In_2->setVisible(true);
    ui->In_3->setVisible(true);
    ui->In_4->setVisible(true);
    ui->Out_1->setVisible(true);
    ui->Out_2->setVisible(true);
    ui->Out_3->setVisible(true);
    ui->Out_4->setVisible(true);
}

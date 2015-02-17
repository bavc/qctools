#include "blackmagicdecklink_userinput.h"
#include "ui_blackmagicdecklink_userinput.h"

#include "Core/TimeCode.h"

#if QT_VERSION >= 0x050000
    #include <QStandardPaths>
#endif
#include <QThread>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

void __stdcall BlackmagicDeckLink_UserInput_TimeCodeCallback(void* Private)
{
    BlackmagicDeckLink_UserInput* CallBack = (BlackmagicDeckLink_UserInput*)Private;
    CallBack->TimeCode_IsAvailable();
}

BlackmagicDeckLink_UserInput::BlackmagicDeckLink_UserInput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlackmagicDeckLink_UserInput),
    Card(NULL),
    In_InModification(false),
    Out_InModification(false)
{
    ui->setupUi(this);

    ui->In_Timecode_HH->setVisible(false);
    ui->In_Timecode_MM->setVisible(false);
    ui->In_Timecode_SS->setVisible(false);
    ui->In_Timecode_FF->setVisible(false);
    ui->Out_Timecode_HH->setVisible(false);
    ui->Out_Timecode_MM->setVisible(false);
    ui->Out_Timecode_SS->setVisible(false);
    ui->Out_Timecode_FF->setVisible(false);
    if (ui->Record_Group->isChecked())
        on_Record_Group_toggled(true);

    // Deck menu
    std::vector<std::string> List=BlackmagicDeckLink_Glue::CardsList();
    for (size_t Pos = 0; Pos<List.size(); Pos++)
        ui->CardsList_Value->addItem(List[Pos].c_str());

    // Not yet supported
    ui->Deck_Source_Label->setVisible(false);
    ui->Deck_Source_Value->setVisible(false);
    ui->In_FrameRate_Label->setVisible(false);
    ui->In_FrameRate_Value->setVisible(false);
    ui->In_DropFrame_Label->setVisible(false);
    ui->In_DropFrame_Value->setVisible(false);
}

BlackmagicDeckLink_UserInput::~BlackmagicDeckLink_UserInput()
{
    delete ui;
}

void BlackmagicDeckLink_UserInput::accept()
{
    if (ui->Record_Group->isChecked() && QFile::exists(ui->Record_DirectoryName_Value->text() + "/" + ui->Record_FileName_Value->text()))
    {
        // Warning to user
        QMessageBox MessageBox(QMessageBox::Warning, "File already exists", ui->Record_DirectoryName_Value->text() + "/" + ui->Record_FileName_Value->text() + " already exists, are you sure you want to overwrite it?", QMessageBox::Yes|QMessageBox::No, this);
        if (MessageBox.exec() != QMessageBox::Yes)
            return;
    }

    if (!Card)
        return;

    if (ui->In_Timecode_Label->isChecked())
    {
        Card->Config_In.TC_in=0;
        Card->Config_In.TC_in+=(ui->In_Timecode_HH->value()/10)<<28;
        Card->Config_In.TC_in+=(ui->In_Timecode_HH->value()%10)<<24;
        Card->Config_In.TC_in+=(ui->In_Timecode_MM->value()/10)<<20;
        Card->Config_In.TC_in+=(ui->In_Timecode_MM->value()%10)<<16;
        Card->Config_In.TC_in+=(ui->In_Timecode_SS->value()/10)<<12;
        Card->Config_In.TC_in+=(ui->In_Timecode_SS->value()%10)<< 8;
        Card->Config_In.TC_in+=(ui->In_Timecode_FF->value()/10)<< 4;
        Card->Config_In.TC_in+=(ui->In_Timecode_FF->value()%10)<< 0;

        Card->Config_In.TC_out=0;
        Card->Config_In.TC_out+=(ui->Out_Timecode_HH->value()/10)<<28;
        Card->Config_In.TC_out+=(ui->Out_Timecode_HH->value()%10)<<24;
        Card->Config_In.TC_out+=(ui->Out_Timecode_MM->value()/10)<<20;
        Card->Config_In.TC_out+=(ui->Out_Timecode_MM->value()%10)<<16;
        Card->Config_In.TC_out+=(ui->Out_Timecode_SS->value()/10)<<12;
        Card->Config_In.TC_out+=(ui->Out_Timecode_SS->value()%10)<< 8;
        Card->Config_In.TC_out+=(ui->Out_Timecode_FF->value()/10)<< 4;
        Card->Config_In.TC_out+=(ui->Out_Timecode_FF->value()%10)<< 0;

        Card->Config_In.FrameCount=-1;
    }
    else
    {
        Card->Config_In.TC_in=-1;

        Card->Config_In.TC_out=-0x23595929;

        Card->Config_In.FrameCount = ui->Out_FrameCount_Value->text().toInt();
    }

    if (ui->Record_Group->isChecked())
    {
        Encoding_FileName=ui->Record_DirectoryName_Value->text()+"/"+ui->Record_FileName_Value->text();
        Encoding_Format = "MOV";
    }

    if (Card->Config_In.FrameCount==-1)
    {
        int FrameCount_In, FrameCount_Out;
        GET_FRAME_COUNT(FrameCount_In, Card->Config_In.TC_in, 30, 1);
        GET_FRAME_COUNT(FrameCount_Out, Card->Config_In.TC_out, 30, 1);
        Card->Config_In.FrameCount=FrameCount_Out-FrameCount_In;
    }

    switch (ui->Deck_VideoBitDepth_Value->currentIndex())
    {
        case 1 : Card->Config_In.VideoBitDepth=10; break;
        default: Card->Config_In.VideoBitDepth=8;
    }

    switch (ui->Record_VideoFormat_Value->currentIndex())
    {
        case 1 : Card->Config_In.VideoCompression=true; break;
        default: Card->Config_In.VideoCompression=false;
    }

    switch (ui->Deck_AudioBitDepth_Value->currentIndex())
    {
        case 1 : Card->Config_In.AudioBitDepth=32; Card->Config_In.AudioTargetBitDepth=24; break;
        case 2 : Card->Config_In.AudioBitDepth=32; Card->Config_In.AudioTargetBitDepth=32; break;
        default: Card->Config_In.AudioBitDepth=16; Card->Config_In.AudioTargetBitDepth=16;
    }

    switch (ui->Deck_AudioChannelsCount_Value->currentIndex())
    {
        case 1 : Card->Config_In.ChannelsCount=8; break;
        case 2 : Card->Config_In.ChannelsCount=16; break;
        default: Card->Config_In.ChannelsCount=2;
    }

    Card->Config_In.TimeCodeIsAvailable_Callback=NULL;
    Card->Config_In.TimeCodeIsAvailable_Private=NULL;

    QDialog::accept();
}

void BlackmagicDeckLink_UserInput::on_Record_Group_toggled(bool on)
{
    if (on && ui->Record_FileName_Value->text().isEmpty())
        #if QT_VERSION >= 0x050000
            ui->Record_DirectoryName_Value->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        #else
            ui->Record_DirectoryName_Value->setText("~/");
        #endif
    if (on && ui->Record_FileName_Value->text().isEmpty())
        ui->Record_FileName_Value->setText("QCTools_Capture.mov");
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_CardsList_Value_currentIndexChanged(int Pos)
{
    delete Card;
    Card=new BlackmagicDeckLink_Glue(Pos);

    // Configuring Deck_Source_Value
    for (size_t Pos=0; Pos<ui->Deck_Source_Value->count()-1; Pos++)
    {
        bool Enabled=Card->Config_Out.VideoInputConnections&(1<<Pos);
        qobject_cast<QStandardItemModel *>(ui->Deck_Source_Value->model())->item( Pos )->setEnabled( Enabled );
    }
    ui->Deck_Source_Value->setCurrentIndex(ui->Deck_Source_Value->count()-1); // "Default"

    // TimeCodeIsAvailable callback
    Card->Config_In.TimeCodeIsAvailable_Callback=BlackmagicDeckLink_UserInput_TimeCodeCallback;
    Card->Config_In.TimeCodeIsAvailable_Private=this;
    Card->CurrentTimecode(); // TimeCode_IsAvailable() will be called when available
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Deck_Source_Value_currentIndexChanged(int Pos)
{
    Card->Config_In.VideoInputConnection=(Pos==ui->Deck_Source_Value->count()-1)?-1:Pos;
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Now_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->In_Timecode_HH->setEnabled(false);
    ui->In_Timecode_MM->setEnabled(false);
    ui->In_Timecode_SS->setEnabled(false);
    ui->In_Timecode_FF->setEnabled(false);

    if (Card->Config_Out.TC_current != -1)
    {
        ui->In_Timecode_HH->setValue(((Card->Config_Out.TC_current>>28)&0xF)*10+((Card->Config_Out.TC_current>>24)&0xF));
        ui->In_Timecode_MM->setValue(((Card->Config_Out.TC_current>>20)&0xF)*10+((Card->Config_Out.TC_current>>16)&0xF));
        ui->In_Timecode_SS->setValue(((Card->Config_Out.TC_current>>12)&0xF)*10+((Card->Config_Out.TC_current>>8 )&0xF));
        ui->In_Timecode_FF->setValue(((Card->Config_Out.TC_current>> 4)&0xF)*10+((Card->Config_Out.TC_current    )&0xF));
    }
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->In_Timecode_HH->setEnabled(true);
    ui->In_Timecode_MM->setEnabled(true);
    ui->In_Timecode_SS->setEnabled(true);
    ui->In_Timecode_FF->setEnabled(true);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_HH_valueChanged(int i)
{
    on_In_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_MM_valueChanged(int i)
{
    on_In_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_SS_valueChanged(int i)
{
    on_In_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_FF_valueChanged(int i)
{
    on_In_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_In_Timecode_XX_valueChanged()
{
    if (In_InModification)
        return;
    In_InModification=true;

    on_Out_FrameCount_Value_valueChanged(ui->Out_FrameCount_Value->value()); // Force update

    In_InModification=false;
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_FrameCount_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->Out_FrameCount_Value->setEnabled(true);

    ui->Out_DurationTC_HH->setEnabled(false);
    ui->Out_DurationTC_MM->setEnabled(false);
    ui->Out_DurationTC_SS->setEnabled(false);
    ui->Out_DurationTC_FF->setEnabled(false);

    ui->Out_DurationTS_HH->setEnabled(false);
    ui->Out_DurationTS_MM->setEnabled(false);
    ui->Out_DurationTS_SS->setEnabled(false);
    ui->Out_DurationTS_mmm->setEnabled(false);

    ui->Out_Timecode_HH->setEnabled(false);
    ui->Out_Timecode_MM->setEnabled(false);
    ui->Out_Timecode_SS->setEnabled(false);
    ui->Out_Timecode_FF->setEnabled(false);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_FrameCount_Value_valueChanged(int i)
{
    if (Out_InModification)
        return;
    Out_InModification=true;
    
    TimeCode Out_DurationTC(i, 30, true);

    ui->Out_DurationTC_HH->setValue(Out_DurationTC.Hours);
    ui->Out_DurationTC_MM->setValue(Out_DurationTC.Minutes);
    ui->Out_DurationTC_SS->setValue(Out_DurationTC.Seconds);
    ui->Out_DurationTC_FF->setValue(Out_DurationTC.Frames);

    ui->Out_DurationTS_HH->setValue(Out_DurationTC.Hours);
    ui->Out_DurationTS_MM->setValue(Out_DurationTC.Minutes);
    ui->Out_DurationTS_SS->setValue(Out_DurationTC.Seconds);
    ui->Out_DurationTS_mmm->setValue(((i*1000)/30)%1000);

    TimeCode In_TimeCode(ui->In_Timecode_HH->value(), ui->In_Timecode_MM->value(), ui->In_Timecode_SS->value(), ui->In_Timecode_FF->value(), 30, true);
    i+=In_TimeCode.ToFrames();
    TimeCode Out_TimeCode(i, 30, true);

    ui->Out_Timecode_HH->setValue(Out_TimeCode.Hours);
    ui->Out_Timecode_MM->setValue(Out_TimeCode.Minutes);
    ui->Out_Timecode_SS->setValue(Out_TimeCode.Seconds);
    ui->Out_Timecode_FF->setValue(Out_TimeCode.Frames);

    Out_InModification=false;
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->Out_FrameCount_Value->setEnabled(false);

    ui->Out_DurationTC_HH->setEnabled(true);
    ui->Out_DurationTC_MM->setEnabled(true);
    ui->Out_DurationTC_SS->setEnabled(true);
    ui->Out_DurationTC_FF->setEnabled(true);

    ui->Out_DurationTS_HH->setEnabled(false);
    ui->Out_DurationTS_MM->setEnabled(false);
    ui->Out_DurationTS_SS->setEnabled(false);
    ui->Out_DurationTS_mmm->setEnabled(false);

    ui->Out_Timecode_HH->setEnabled(false);
    ui->Out_Timecode_MM->setEnabled(false);
    ui->Out_Timecode_SS->setEnabled(false);
    ui->Out_Timecode_FF->setEnabled(false);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_HH_valueChanged(int i)
{
    on_Out_DurationTC_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_MM_valueChanged(int i)
{
    on_Out_DurationTC_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_SS_valueChanged(int i)
{
    on_Out_DurationTC_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_FF_valueChanged(int i)
{
    on_Out_DurationTC_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTC_XX_valueChanged()
{
    if (Out_InModification)
        return;
    Out_InModification=true;
    
    TimeCode TC(ui->Out_DurationTC_HH->value(), ui->Out_DurationTC_MM->value(), ui->Out_DurationTC_SS->value(), ui->Out_DurationTC_FF->value(), 30, true);

    ui->Out_FrameCount_Value->setValue(TC.ToFrames());

    Out_InModification=false;
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->Out_FrameCount_Value->setEnabled(false);

    ui->Out_DurationTC_HH->setEnabled(false);
    ui->Out_DurationTC_MM->setEnabled(false);
    ui->Out_DurationTC_SS->setEnabled(false);
    ui->Out_DurationTC_FF->setEnabled(false);

    ui->Out_DurationTS_HH->setEnabled(true);
    ui->Out_DurationTS_MM->setEnabled(true);
    ui->Out_DurationTS_SS->setEnabled(true);
    ui->Out_DurationTS_mmm->setEnabled(true);

    ui->Out_Timecode_HH->setEnabled(false);
    ui->Out_Timecode_MM->setEnabled(false);
    ui->Out_Timecode_SS->setEnabled(false);
    ui->Out_Timecode_FF->setEnabled(false);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_HH_valueChanged(int i)
{
    on_Out_DurationTS_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_MM_valueChanged(int i)
{
    on_Out_DurationTS_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_SS_valueChanged(int i)
{
    on_Out_DurationTS_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_mmm_valueChanged(int i)
{
    on_Out_DurationTS_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_DurationTS_XX_valueChanged()
{
    if (Out_InModification)
        return;
    Out_InModification=true;
    
    int i=ui->Out_DurationTS_HH->value()*60*60*30+ui->Out_DurationTS_MM->value()*60*30+ui->Out_DurationTS_SS->value()*30+ui->Out_DurationTS_mmm->value()*30/1000;

    ui->Out_FrameCount_Value->setValue(i);

    Out_InModification=false;
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_Label_toggled(bool checked)
{
    if (!checked)
        return;

    ui->Out_FrameCount_Value->setEnabled(false);

    ui->Out_DurationTC_HH->setEnabled(false);
    ui->Out_DurationTC_MM->setEnabled(false);
    ui->Out_DurationTC_SS->setEnabled(false);
    ui->Out_DurationTC_FF->setEnabled(false);

    ui->Out_DurationTS_HH->setEnabled(false);
    ui->Out_DurationTS_MM->setEnabled(false);
    ui->Out_DurationTS_SS->setEnabled(false);
    ui->Out_DurationTS_mmm->setEnabled(false);

    ui->Out_Timecode_HH->setEnabled(true);
    ui->Out_Timecode_MM->setEnabled(true);
    ui->Out_Timecode_SS->setEnabled(true);
    ui->Out_Timecode_FF->setEnabled(true);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_HH_valueChanged(int i)
{
    on_Out_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_MM_valueChanged(int i)
{
    on_Out_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_SS_valueChanged(int i)
{
    on_Out_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_FF_valueChanged(int i)
{
    on_Out_Timecode_XX_valueChanged();
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Out_Timecode_XX_valueChanged()
{
    if (Out_InModification)
        return;
    Out_InModification=true;
    
    TimeCode In_TC(ui->In_Timecode_HH->value(), ui->In_Timecode_MM->value(), ui->In_Timecode_SS->value(), ui->In_Timecode_FF->value(), 30, true);
    TimeCode Out_TC(ui->Out_Timecode_HH->value(), ui->Out_Timecode_MM->value(), ui->Out_Timecode_SS->value(), ui->Out_Timecode_FF->value(), 30, true);

    if (Out_TC.ToFrames()>In_TC.ToFrames())
    {
        ui->Out_FrameCount_Value->setValue(Out_TC.ToFrames()-In_TC.ToFrames());
        Out_InModification=false;
    }
    else
    {
        // Invalid timecode, we force it to 1 In timecode + 1
        Out_InModification=false;
        if (ui->Out_FrameCount_Value->value()==1)
            on_Out_FrameCount_Value_valueChanged(1); // it is not called because value has not changed
        else
            ui->Out_FrameCount_Value->setValue(1);
    }
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::on_Record_DirectoryName_Dialog_pressed()
{
    QString Dir=QFileDialog::getExistingDirectory(this, "Choose a recording directory", ui->Record_DirectoryName_Value->text());
    if (Dir.isEmpty())
        return;

    ui->Record_DirectoryName_Value->setText(Dir);
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_UserInput::TimeCode_IsAvailable()
{
    In_InModification=true;
    ui->In_Timecode_HH->setValue(((Card->Config_Out.TC_current>>28)&0xF)*10+((Card->Config_Out.TC_current>>24)&0xF));
    ui->In_Timecode_MM->setValue(((Card->Config_Out.TC_current>>20)&0xF)*10+((Card->Config_Out.TC_current>>16)&0xF));
    ui->In_Timecode_SS->setValue(((Card->Config_Out.TC_current>>12)&0xF)*10+((Card->Config_Out.TC_current>>8)&0xF));
    ui->In_Timecode_FF->setValue(((Card->Config_Out.TC_current>>4)&0xF)*10+((Card->Config_Out.TC_current)&0xF));
    In_InModification=false;
    on_In_Timecode_XX_valueChanged();

    ui->In_Timecode_Label->setText("Timecode");
    ui->In_Timecode_Label->setEnabled(true);
    ui->Out_Timecode_Label->setText("Timecode");
    ui->Out_Timecode_Label->setEnabled(true);

    ui->In_Timecode_HH->setVisible(true);
    ui->In_Timecode_MM->setVisible(true);
    ui->In_Timecode_SS->setVisible(true);
    ui->In_Timecode_FF->setVisible(true);
    ui->Out_Timecode_HH->setVisible(true);
    ui->Out_Timecode_MM->setVisible(true);
    ui->Out_Timecode_SS->setVisible(true);
    ui->Out_Timecode_FF->setVisible(true);
}

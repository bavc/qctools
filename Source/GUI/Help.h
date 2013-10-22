//---------------------------------------------------------------------------
#ifndef HelpH
#define HelpH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QDialog>
class QTabWidget;
class QPushButton;
//---------------------------------------------------------------------------

//***************************************************************************
// GUI_Main
//***************************************************************************

class Help : public QDialog
{
    Q_OBJECT

public:
    Help (QWidget * parent);

    // Direct access
    void show_HowToUse();
    void show_FilterDescriptions();
    void show_License();

private:
    //GUI
    QTabWidget*     Central;
    QPushButton*    Close;
};

#endif

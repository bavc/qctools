/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

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
    // Constructor/Destructor
    Help (QWidget * parent);

    // Actions
    void GettingStarted();
    void HowToUseThisTool();
    void FilterDescriptions();
    void PlaybackFilters();
    void About();

private:
    //GUI
    QTabWidget*     Central;
    QPushButton*    Close;
};

#endif

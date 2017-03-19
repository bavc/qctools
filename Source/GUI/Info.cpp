/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "GUI/Info.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/FileInformation.h"
#include "Core/Core.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QLabel>
#include <QGridLayout>
#include "Core/CommonStats.h"
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

//---------------------------------------------------------------------------
Info::Info(QWidget *parent, const struct per_group* plotGroup, const struct per_item* plotItem, size_t CountOfGroups_, size_t CountOfItems_, FileInformation* FileInformationData_, style Style):
    QWidget(parent),
    // File information
    FileInfoData(FileInformationData_),
    m_plotGroup( plotGroup ),
    m_plotItem( plotItem ),
    CountOfGroups( CountOfGroups_ ),
    CountOfItems( CountOfItems_ )
{
    // Info
    QGridLayout* Layout=new QGridLayout();
    Layout->setSpacing(0);
    Layout->setMargin(0);
    Layout->setContentsMargins(0,0,0,0);

    QFont Font=QFont();
    #ifdef _WIN32
        Font.setPointSize(6);
    #else //_WIN32
        Font.setPointSize(8);
    #endif //_WIN32

    // Values
    Values=new QLabel*[CountOfItems];
    size_t X=0;
    size_t Y=0;
    for (size_t Pos=0; Pos<CountOfItems; Pos++)
    {
        Values[Pos]=new QLabel(this);
        Values[Pos]->setText(m_plotItem[Pos].Name+QString("= "));
        Values[Pos]->setFont(Font);
        Layout->addWidget(Values[Pos], X, Y);

        Y++;
        if (Style==Style_Columns || m_plotItem[Y].NewLine)
        {
            X++;
            Y=0;
        }
    }

    setLayout(Layout);

    // Info
    Frames_Pos=-1;
    ShouldUpate=false;
}

//---------------------------------------------------------------------------
Info::~Info()
{
    delete[] Values;
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
void Info::Update()
{
    if ((!ShouldUpate && Frames_Pos==FileInfoData->Frames_Pos_Get())
     || ( ShouldUpate && false)) // ToDo: try to optimize
        return;
    Frames_Pos=FileInfoData->Frames_Pos_Get();
    ShouldUpate=false;

    if (Frames_Pos<FileInfoData->ReferenceStat()->x_Current)
        for (size_t Pos=0; Pos<CountOfItems; Pos++)
        {
            if (m_plotItem[Pos].DigitsAfterComma==0)
                Values[Pos]->setText(m_plotItem[Pos].Name+QString("= ")+QString::number((int)FileInfoData->ReferenceStat()->y[Pos][Frames_Pos]));
            else
            {
                QString Value=QString::number(FileInfoData->ReferenceStat()->y[Pos][Frames_Pos], 'f', m_plotItem[Pos].DigitsAfterComma);
                int Point=Value.indexOf('.');
                if (Point==-1)
                {
                    Value+='.';
                    Point=Value.size();
                }
                else
                    Point++;
                while (Value.size()-Point<m_plotItem[Pos].DigitsAfterComma)
                    Value+='0'; //Adding precision information
                Values[Pos]->setText(m_plotItem[Pos].Name+QString("= ")+Value);
            }
        }
    else
    {
        for (size_t Pos=0; Pos<CountOfItems; Pos++)
            Values[Pos]->setText(m_plotItem[Pos].Name+QString("= "));
        ShouldUpate=true;
    }
}

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Info_H
#define GUI_Info_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <QWidget>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
class QLabel;
class FileInformation;
//---------------------------------------------------------------------------

//***************************************************************************
// Class
//***************************************************************************

class Info : public QWidget
{
    Q_OBJECT

public:
    // Constructor/Destructor
    enum style
    {
        Style_Columns,
        Style_Grid,
    };
    explicit Info(QWidget *parent, const struct per_group* plotGroup, const struct per_item* plotItem, size_t CountOfGroups, size_t CountOfItems, FileInformation* FileInfoData, style Style);
    ~Info();

    // Commands
    void                        Update                      ();

protected:
    // File information
    FileInformation*                   FileInfoData;
    bool                        ShouldUpate;
    int                         Frames_Pos;

    // Widgets
    QLabel**                    Values;

    // Arrays
    const struct per_group*    m_plotGroup; 
    const struct per_item*     m_plotItem; 
    size_t                          CountOfGroups;
    size_t                          CountOfItems;
};

#endif // GUI_Info_H

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plot_Scale_Widget_H
#define GUI_Plot_Scale_Widget_H
//---------------------------------------------------------------------------

#include <qwt_scale_widget.h>
#include <qwt_scale_div.h>

//***************************************************************************
// Class
//***************************************************************************

class PlotScaleWidget: public QwtScaleWidget
{
	Q_OBJECT

public:
    PlotScaleWidget( QWidget* parent = NULL );
	virtual ~PlotScaleWidget();

    void setScale( double from, double to );

    void setFormat( int format );
    int format() const;

    QwtScaleDiv scaleDiv() const;
    QwtInterval interval() const;
};

#endif // GUI_Plot_Scale_Widget_H

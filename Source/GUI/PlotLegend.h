/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef GUI_Plot_Legend_H
#define GUI_Plot_Legend_H
//---------------------------------------------------------------------------

#include <qwt_legend.h>

//***************************************************************************
// Class
//***************************************************************************

class PlotLegend: public QwtLegend
{
    Q_OBJECT
public:
    PlotLegend( QWidget *parent = NULL );
    virtual ~PlotLegend();

protected:
    virtual QWidget *createWidget( const QwtLegendData &data ) const;
};

#endif // GUI_Plot_Legend_H

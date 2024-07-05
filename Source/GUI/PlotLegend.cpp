/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/PlotLegend.h"
#include <qwt_graphic.h>
#include <qwt_legend_label.h>
#include <qwt_dyngrid_layout.h>
#include <QScrollArea>
#include <QLayout>

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
PlotLegend::PlotLegend( QWidget *parent ):
        QwtLegend( parent )
{
    setMinimumHeight( 1 );
    setMaxColumns( 1 );
    setContentsMargins( 0, 0, 0, 0 );

    QwtDynGridLayout* layout = qobject_cast<QwtDynGridLayout*> (contentsWidget()->layout());
    layout->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    layout->setSpacing(0);
    layout->setExpandingDirections(Qt::Horizontal);

    QScrollArea *scrollArea = findChild<QScrollArea *>();
    if ( scrollArea )
    {
        scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    }

#if 1
    QFont fnt = font();
    if ( fnt.pointSize() > 8 )
    {
        fnt.setPointSize( 8 );
        setFont( fnt );
    }
#endif
}

PlotLegend::~PlotLegend()
{
}

QWidget *PlotLegend::createWidget( const QwtLegendData &data ) const
{
    QWidget *w = QwtLegend::createWidget( data );
    auto icon = data.icon();
    if(!icon.isNull())
        w->setMaximumHeight(10);

    QwtLegendLabel *label = dynamic_cast<QwtLegendLabel *>( w );
    if ( label )
    {
        label->setMargin( 0 );
        label->setContentsMargins(0, 0, 0, 0);
    }

    return w;
}

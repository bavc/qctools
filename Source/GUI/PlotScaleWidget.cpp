/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------

#include "GUI/PlotScaleWidget.h"
#include "GUI/Plots.h"
#include <qwt_scale_engine.h>

class PlotScaleDraw: public QwtScaleDraw
{
public:
    PlotScaleDraw():
        m_format( Plots::AxisTime )
    {
    }

    void setFormat( int format )
    {
        if ( format != m_format )
        {
            m_format = format;
            invalidateCache();
        }
    }

    int format() const
    {
        return m_format;
    }

    virtual QwtText label( double value ) const
    {
        if ( m_format == Plots::AxisTime )
        {
            const int h = static_cast<int>( value / 3600 );
            const int m = static_cast<int>( value / 60 );
            const int s = static_cast<int>( value );

            QString label;

            if ( scaleDiv().interval().width() > 10.0 )
            {
                label.sprintf( "%02d:%02d:%02d",
                    h, m - h * 60, s - m * 60 );
            }
            else
            {
                const int ms = qRound( ( value - s ) * 1000.0 );
                label.sprintf( "%02d:%02d:%02d.%03d",
                    h, m - h * 60, s - m * 60, ms);
            }

            return label;
        }

        return QwtScaleDraw::label( value );
    }

private:
    int m_format;
};

PlotScaleWidget::PlotScaleWidget( QWidget* parent ):
    QwtScaleWidget( QwtScaleDraw::BottomScale, parent )
{
    setScaleDraw( new PlotScaleDraw() );
}

PlotScaleWidget::~PlotScaleWidget()
{
}

void PlotScaleWidget::setScale( double from, double to )
{
    QwtLinearScaleEngine se;
    setScaleDiv( se.divideScale( from, to, 5, 8 ) );
}

void PlotScaleWidget::setFormat( int format )
{
    PlotScaleDraw* sd = dynamic_cast<PlotScaleDraw*>( scaleDraw() );
    if ( sd )
        sd->setFormat( format );
}

int PlotScaleWidget::format() const
{
    const PlotScaleDraw* sd = dynamic_cast<const PlotScaleDraw*>( scaleDraw() );
    return sd ? sd->format() : 0;
}

QwtScaleDiv PlotScaleWidget::scaleDiv() const
{
    return scaleDraw()->scaleDiv();
}

QwtInterval PlotScaleWidget::interval() const
{
    return scaleDraw()->scaleDiv().interval();
}

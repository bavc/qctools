#include "doublespinboxwithslider.h"
#include <math.h>

//---------------------------------------------------------------------------
DoubleSpinBoxWithSlider::DoubleSpinBoxWithSlider(int Min_, int Max_, int Divisor_, int Current, const char* Name, size_t Pos_, bool IsBitSlice_, bool IsFilter_, bool IsPeak_, bool IsMode_, bool IsScale_, bool IsColorspace_, bool IsDmode_, bool IsSystem_, QWidget *parent) :
    Divisor(Divisor_),
    Min(Min_),
    Max(Max_),
    Pos(Pos_),
    IsBitSlice(IsBitSlice_),
    IsFilter(IsFilter_),
    IsPeak(IsPeak_),
    IsMode(IsMode_),
    IsScale(IsScale_),
    IsColorspace(IsColorspace_),
    IsDmode(IsDmode_),
    IsSystem(IsSystem_),
    QDoubleSpinBox(parent)
{
    Popup=NULL;
    Slider=NULL;

    setMinimum(((double)Min)/Divisor);
    setMaximum(((double)Max)/Divisor);
    setValue(((double)Current)/Divisor);
    setToolTip(Name);
    if (Divisor<=1)
    {
        setDecimals(0);
        setSingleStep(1);
    }
    else if (Divisor<=10)
    {
        setDecimals(1);
        setSingleStep(0.1);
    }
    else if (Divisor<=100)
    {
        setDecimals(2);
        setSingleStep(0.01);
    }

    if (IsBitSlice)
        setPrefix("Bit ");

    QFont Font;
    #ifdef _WIN32
    #else //_WIN32
        Font.setPointSize(Font.pointSize()*3/4);
    #endif //_WIN32
    setFont(Font);

    setFocusPolicy(Qt::NoFocus);

    //Popup=new QWidget((QWidget*)parent(), Qt::Popup | Qt::Window);
    //Popup=new QWidget((QWidget*)parent(), Qt::FramelessWindowHint);
    //Popup->setGeometry(((QWidget*)parent())->geometry().x()+x()+width()-(255+30), ((QWidget*)parent())->geometry().y()+y()+height(), 255+30, height());
    //Popup->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
    //Popup->setWindowModality(Qt::NonModal);
    //Popup->setFocusPolicy(Qt::NoFocus);
    //QLayout* Layout=new QGridLayout();
    //Layout->setContentsMargins(0, 0, 0, 0);
    //Layout->setSpacing(0);
    Slider=new QSlider(Qt::Horizontal, parentWidget());
    Slider->setFocusPolicy(Qt::NoFocus);
    Slider->setMinimum(Min);
    Slider->setMaximum(Max);
    Slider->setToolTip(toolTip());
    int slider_width = 255 + 30;
    // Assure that the initial position is always inside the window
    int initial_x = std::max(x() + width() - slider_width, 5);
    Slider->setGeometry(initial_x, y() + height(), slider_width, height());
    connect(Slider, SIGNAL(valueChanged(int)), this, SLOT(on_sliderMoved(int)));
    connect(Slider, SIGNAL(sliderMoved(int)), this, SLOT(on_sliderMoved(int)));

    Slider->setFocusPolicy(Qt::NoFocus);
    //Layout->addWidget(Slider);
    //Popup->setFocusPolicy(Qt::NoFocus);
    //Popup->setLayout(Layout);
    connect(this, SIGNAL(valueChanged(double)), this, SLOT(on_valueChanged(double)));

    Slider->hide();
}

//---------------------------------------------------------------------------
DoubleSpinBoxWithSlider::~DoubleSpinBoxWithSlider()
{
    //delete Popup; //Popup=NULL;
    delete Slider; //Slider=NULL;
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::enterEvent (QEvent* event)
{
    if (Slider==NULL)
    {
        Slider->show();
    }

    Q_EMIT entered(this);

    if (!Slider->isVisible())
    {
        on_valueChanged(value());
        Slider->show();
        ((QWidget*)parent())->repaint();
    }
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::ChangeMax(int Max_)
{
    Max=Max_;
    setMaximum(Max);
    if (Slider)
        Slider->setMaximum(Max);
}

void DoubleSpinBoxWithSlider::applyValue(double value, bool notify)
{
    if (IsBitSlice)
    {
        if (value<1)
            setPrefix(QString());
        else
            setPrefix("Bit ");
    }

    if (Slider)
    {
        double Value=value*Divisor;
        int ValueInt=(int)Value;
        if(Value-0.5>=ValueInt)
            ValueInt++;

        int oldValue = Slider->value();
        if(oldValue != ValueInt)
        {
            blockSignals(true);

            Slider->blockSignals(true);
            Slider->setValue(ValueInt);
            Slider->blockSignals(false);

            setValue(value);
            blockSignals(false);

            Q_EMIT valueChanged(value);
        }

        if(notify)
            Q_EMIT controlValueChanged(Value);
    }
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::leaveEvent (QEvent* event)
{
    //Popup->hide();
    //((QWidget*)parent())->repaint();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::keyPressEvent (QKeyEvent* event)
{
    if (event->key()==Qt::Key_Up || event->key()==Qt::Key_Down)
    {
        QDoubleSpinBox::keyPressEvent(event);
    }

    event->ignore();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::moveEvent (QMoveEvent* event)
{
    if (Slider)
        //Popup->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
        Slider->setGeometry(x()+width()-(255+30), y()+height(), 255+30, height());
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::hidePopup ()
{
    //if (Popup)
    //    Popup->hide();
    if (Slider)
        Slider->hide();
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::on_valueChanged (double value)
{
    if (IsBitSlice)
    {
        if (value<1)
            setPrefix(QString());
        else
            setPrefix("Bit ");
    }

    if (Slider)
    {
        double Value=value*Divisor;
        int ValueInt=(int)Value;
        if(Value-0.5>=ValueInt)
            ValueInt++;

        Slider->setValue(ValueInt);
        Q_EMIT controlValueChanged(Value);
    }
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::on_sliderMoved (int value)
{
    setValue(((double)value)/Divisor);
}

//---------------------------------------------------------------------------
void DoubleSpinBoxWithSlider::showEvent (QShowEvent* event)
{
    if (IsBitSlice)
    {
        if (value()<1)
            setPrefix(QString());
        else
            setPrefix("Bit ");
    }

    QDoubleSpinBox::showEvent(event);
}

//---------------------------------------------------------------------------
QString DoubleSpinBoxWithSlider::textFromValue (double value) const
{
    if (IsBitSlice && value==0)
        return "All";
    else if (IsBitSlice && value==-1)
        return "None";
    else if (IsFilter && value==0)
        return "lowpass";
    else if (IsFilter && value==1)
        return "flat";
    else if (IsFilter && value==2)
        return "aflat";
    else if (IsFilter && value==3)
        return "chroma";
    else if (IsFilter && value==4)
        return "color";
    else if (IsFilter && value==5)
        return "acolor";
    else if (IsFilter && value==6)
        return "xflat";
    else if (IsPeak && value==0)
        return "none";
    else if (IsPeak && value==1)
        return "instant";
    else if (IsPeak && value==2)
        return "peak";
    else if (IsPeak && value==3)
        return "peak+instant";
    else if (IsMode && value==0)
        return "gray";
    else if (IsMode && value==1)
        return "color";
    else if (IsMode && value==2)
        return "color2";
    else if (IsMode && value==3)
        return "color3";
    else if (IsMode && value==4)
        return "color4";
    else if (IsMode && value==5)
        return "color5";
    else if (IsScale && value==0)
        return "digital";
    else if (IsScale && value==1)
        return "millivolts";
    else if (IsScale && value==2)
        return "ire";
    else if (IsColorspace && value==0)
        return "auto";
    else if (IsColorspace && value==1)
        return "601";
    else if (IsColorspace && value==2)
        return "709";
    else if (IsDmode && value==0)
        return "mono";
    else if (IsDmode && value==1)
        return "color";
    else if (IsDmode && value==2)
        return "color2";
    else if (IsSystem && value==0)
        return "NTSC 1953 Y'I'O' (ITU-R BT.470 System M)";
    else if (IsSystem && value==1)
        return "EBU Y'U'V' (PAL/SECAM) (ITU-R BT.470 System B, G)";
    else if (IsSystem && value==2)
        return "SMPTE-C RGB";
    else if (IsSystem && value==3)
        return "SMPTE-240M Y'PbPr";
    else if (IsSystem && value==4)
        return "Apple RGB";
    else if (IsSystem && value==5)
        return "Adobe Wide Gamut RGB";
    else if (IsSystem && value==6)
        return "CIE 1931 RGB";
    else if (IsSystem && value==7)
        return "ITU.BT-709 Y'CbCr";
    else if (IsSystem && value==8)
        return "ITU-R.BT-2020";
    else
        return QDoubleSpinBox::textFromValue(value);
}

//---------------------------------------------------------------------------
double DoubleSpinBoxWithSlider::valueFromText (const QString& text) const
{
    if (IsBitSlice && text=="All")
        return -1;
    else if (IsBitSlice && text=="None")
        return 0;
    else
        return QDoubleSpinBox::valueFromText(text);
}

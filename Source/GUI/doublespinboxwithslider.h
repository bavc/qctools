#ifndef DOUBLESPINBOXWITHSLIDER_H
#define DOUBLESPINBOXWITHSLIDER_H

#include <QDoubleSpinBox>
#include <QSlider>
#include <QKeyEvent>

class DoubleSpinBoxWithSlider : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit DoubleSpinBoxWithSlider (QWidget *parent, int Min, int Max, int Divisor, int Current, const char* Name, bool IsBitSlice, bool IsFilter, bool IsPeak, bool IsMode, bool IsScale, bool IsColorspace, bool IsDmode, bool IsSystem);
    ~DoubleSpinBoxWithSlider();

    bool IsBitSlice;
    bool IsFilter;
    bool IsPeak;
    bool IsMode;
    bool IsScale;
    bool IsColorspace;
    bool IsDmode;
    bool IsSystem;
    void ChangeMax(int Max);

    void applyValue(double value, bool notify);
    void hidePopup ();

protected:
    void enterEvent (QEvent* event);
    void leaveEvent (QEvent* event);
    void keyPressEvent (QKeyEvent* event);
    void moveEvent (QMoveEvent * event);

    void showEvent (QShowEvent* event);
    QString textFromValue (double value) const;
    double  valueFromText (const QString& text) const;

private:
    QWidget*                    Popup;
    QSlider*                    Slider;
    int                         Min;
    int                         Max;
    int                         Divisor;

public Q_SLOTS:
    void on_valueChanged(double);
    void on_sliderMoved(int);

Q_SIGNALS:
    void controlValueChanged(double);
    void entered(DoubleSpinBoxWithSlider* control);
};

#endif // DOUBLESPINBOXWITHSLIDER_H

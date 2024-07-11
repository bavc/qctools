#ifndef YMINMAXSELECTOR_H
#define YMINMAXSELECTOR_H

#include <QWidget>

namespace Ui {
class YMinMaxSelector;
}

class Plot;
class YMinMaxSelector : public QWidget
{
    Q_OBJECT

public:
    explicit YMinMaxSelector(QWidget *parent = nullptr);
    ~YMinMaxSelector();

    bool isMinMaxFromThePlot() const;
    bool isFormula() const;
    bool isCustom() const;

    void enableFormulaMinMax(bool value);
    void setPlot(Plot* plot);
    Plot* getPlot() const;

private Q_SLOTS:
    void enableCustomMinMax(bool value);
    void on_apply_pushButton_clicked();
    void on_min_doubleSpinBox_valueChanged(double arg1);
    void on_max_doubleSpinBox_valueChanged(double arg1);
    void on_minMaxOfThePlot_radioButton_clicked();
    void on_minMaxSystemProvided_radioButton_clicked();
    void on_customMinMax_radioButton_clicked();

    void on_reset_pushButton_clicked();

private:
    void updateApplyButton();
    void updateMinMaxStyling();

    Plot* m_plot { nullptr };
    QPalette m_palette;
    QPalette m_redPalette;
    Ui::YMinMaxSelector *ui;
};

#endif // YMINMAXSELECTOR_H

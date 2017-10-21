#ifndef BOOLEANCHARTCONDITIONEDITOR_H
#define BOOLEANCHARTCONDITIONEDITOR_H

#include <QWidget>
#include "Plot.h"

namespace Ui {
class BooleanChartConditionEditor;
}

class BooleanChartConditionInput;
class BooleanChartConditionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit BooleanChartConditionEditor(QWidget *parent = 0);
    ~BooleanChartConditionEditor();

    BooleanChartConditionInput* getCondition(int index) const;
    int conditionsCount() const;

    void setLabel(const QString& label);
    void setDefaultColor(const QColor& color);
    void setConditions(const PlotSeriesData::Conditions& value);

private Q_SLOTS:
    void addCondition();
    void removeCondition();

private:
    QColor m_defaultColor;
    Ui::BooleanChartConditionEditor *ui;
};

#endif // BOOLEANCHARTCONDITIONEDITOR_H

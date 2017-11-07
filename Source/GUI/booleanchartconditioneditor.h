#ifndef BOOLEANCHARTCONDITIONEDITOR_H
#define BOOLEANCHARTCONDITIONEDITOR_H

#include <QLabel>
#include <QWidget>
#include "Plot.h"

namespace Ui {
class BooleanChartConditionEditor;
}

class QCompleter;
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
    QLabel* takeLabel();

    void setDefaultColor(const QColor& color);
    void setConditions(const PlotSeriesData::Conditions& value);

    QCompleter* makeCompleter(QJSEngine& engine);

public Q_SLOTS:
    void onConditionsUpdated();

private Q_SLOTS:
    void addCondition();
    void removeCondition();

private:
    QColor m_defaultColor;
    Ui::BooleanChartConditionEditor *ui;
};

#endif // BOOLEANCHARTCONDITIONEDITOR_H

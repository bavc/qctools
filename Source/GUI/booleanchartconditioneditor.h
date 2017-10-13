#ifndef BOOLEANCHARTCONDITIONEDITOR_H
#define BOOLEANCHARTCONDITIONEDITOR_H

#include <QTimer>
#include <QWidget>
#include "Plot.h"

namespace Ui {
class BooleanChartConditionEditor;
}

class QJSEngine;
class BooleanChartConditionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit BooleanChartConditionEditor(QWidget *parent = 0);
    ~BooleanChartConditionEditor();

    void setLabel(const QString& label);
    void setColor(const QColor& color);

    void setCondition(const PlotSeriesData::Condition& value);
    QString getCondition() const;

private Q_SLOTS:
    void on_condition_lineEdit_textEdited(const QString &arg1);

private:
    Ui::BooleanChartConditionEditor *ui;
    QTimer m_validationTimer;
    const PlotSeriesData::Condition* m_condition;

    QColor m_defaultTextColor;
    QColor m_validatedTextColor;
    QColor m_errorTextColor;
};

#endif // BOOLEANCHARTCONDITIONEDITOR_H

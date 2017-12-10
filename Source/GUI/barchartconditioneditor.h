#ifndef BarchartConditionEditor_H
#define BarchartConditionEditor_H

#include <QLabel>
#include <QWidget>
#include "Plot.h"

namespace Ui {
class BarchartConditionEditor;
}

class QCompleter;
class BarchartConditionInput;
class BarchartConditionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit BarchartConditionEditor(QWidget *parent = 0);
    ~BarchartConditionEditor();

    BarchartConditionInput* getCondition(int index) const;
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
    Ui::BarchartConditionEditor *ui;
};

#endif // BarchartConditionEditor_H

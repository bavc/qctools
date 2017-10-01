#ifndef BOOLEANCHARTCONDITIONEDITOR_H
#define BOOLEANCHARTCONDITIONEDITOR_H

#include <QWidget>

namespace Ui {
class BooleanChartConditionEditor;
}

class BooleanChartConditionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit BooleanChartConditionEditor(QWidget *parent = 0);
    ~BooleanChartConditionEditor();

    void setLabel(const QString& label);
    void setColor(const QColor& color);

    void setLessThan(const QString& value);
    QString getLessThan() const;

    void setMoreThan(const QString& value);
    QString getMoreThan() const;

private:
    Ui::BooleanChartConditionEditor *ui;
};

#endif // BOOLEANCHARTCONDITIONEDITOR_H

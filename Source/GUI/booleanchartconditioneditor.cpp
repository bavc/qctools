#include "booleanchartconditioneditor.h"
#include "ui_booleanchartconditioneditor.h"

BooleanChartConditionEditor::BooleanChartConditionEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BooleanChartConditionEditor)
{
    ui->setupUi(this);
}

BooleanChartConditionEditor::~BooleanChartConditionEditor()
{
    delete ui;
}

void BooleanChartConditionEditor::setLabel(const QString &label)
{
    ui->label->setText(label);
}

void BooleanChartConditionEditor::setColor(const QColor &color)
{
    QPalette palette = ui->frame->palette();
    palette.setColor( backgroundRole(), color);

    ui->frame->setPalette(palette);
}

void BooleanChartConditionEditor::setLessThan(const QString &value)
{
    ui->lessThan_lineEdit->setText(value);
}

QString BooleanChartConditionEditor::getLessThan() const
{
    return ui->lessThan_lineEdit->text();
}

void BooleanChartConditionEditor::setMoreThan(const QString &value)
{
    ui->moreThan_lineEdit->setText(value);
}

QString BooleanChartConditionEditor::getMoreThan() const
{
    return ui->moreThan_lineEdit->text();
}

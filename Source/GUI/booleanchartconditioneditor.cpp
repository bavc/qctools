#include "booleanchartconditioneditor.h"
#include "ui_booleanchartconditioneditor.h"

BooleanChartConditionEditor::BooleanChartConditionEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BooleanChartConditionEditor)
{
    ui->setupUi(this);

    m_defaultTextColor = ui->condition_lineEdit->palette().color(QPalette::Text);
    m_validatedTextColor = QColor(Qt::darkGreen);
    m_errorTextColor = QColor(Qt::red);

    m_validationTimer.setSingleShot(true);
    m_validationTimer.setInterval(500);

    connect(&m_validationTimer, &QTimer::timeout, [&] {
        if(!m_condition)
            return;

        QColor color;
        if(ui->condition_lineEdit->text().isEmpty())
        {
            color = m_defaultTextColor;
            ui->condition_lineEdit->setToolTip("No condition");
        }
        else
        {
            auto result = m_condition->makeConditionFunction(ui->condition_lineEdit->text());
            if(result.isError() || !result.isCallable()) {
                color = m_errorTextColor;
                ui->condition_lineEdit->setToolTip("Error: " + result.toString());
            } else {
                auto callResult = result.call(QJSValueList() << 0);
                if(callResult.isError() && !result.isBool()) {
                    color = m_errorTextColor;
                    ui->condition_lineEdit->setToolTip("Error: " + callResult.toString());
                } else {
                    color = m_validatedTextColor;
                    ui->condition_lineEdit->setToolTip("Success");
                }
            }
        }

        auto palette = ui->condition_lineEdit->palette();
        palette.setColor(QPalette::Text, color);

        ui->condition_lineEdit->setPalette(palette);
    });
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

void BooleanChartConditionEditor::setCondition(const PlotSeriesData::Condition &value)
{
    ui->condition_lineEdit->setText(value.m_conditionString);
    m_condition = &value;
    m_validationTimer.start(0);
}

QString BooleanChartConditionEditor::getCondition() const
{
    return ui->condition_lineEdit->text();
}

void BooleanChartConditionEditor::on_condition_lineEdit_textEdited(const QString &arg1)
{
    m_validationTimer.start();
}

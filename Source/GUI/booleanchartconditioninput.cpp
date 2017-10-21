#include "GUI/booleanchartconditioninput.h"
#include "ui_booleanchartconditioninput.h"
#include <QColorDialog>
#include <QJSValueList>
#include <cassert>

BooleanChartConditionInput::BooleanChartConditionInput(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BooleanChartConditionInput),
    m_engine(nullptr)
{
    ui->setupUi(this);

    ui->add_toolButton->setMaximumWidth(ui->add_toolButton->height());
    ui->add_toolButton->setMaximumHeight(ui->add_toolButton->height());

    ui->remove_toolButton->setMaximumWidth(ui->remove_toolButton->height());
    ui->remove_toolButton->setMaximumHeight(ui->remove_toolButton->height());

    m_defaultTextColor = ui->condition_lineEdit->palette().color(QPalette::Text);
    m_validatedTextColor = QColor(Qt::darkGreen);
    m_errorTextColor = QColor(Qt::red);

    m_validationTimer.setSingleShot(true);
    m_validationTimer.setInterval(500);

    connect(&m_validationTimer, &QTimer::timeout, [&] {
            assert(m_engine);

            QColor color;
            if(ui->condition_lineEdit->text().isEmpty())
            {
                color = m_defaultTextColor;
                ui->condition_lineEdit->setToolTip("No condition");
            }
            else
            {
                auto result = PlotSeriesData::Condition::makeConditionFunction(m_engine, ui->condition_lineEdit->text());
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

BooleanChartConditionInput::~BooleanChartConditionInput()
{
    delete ui;
}

void BooleanChartConditionInput::setColor(const QColor &color)
{
    m_color = color;
    ui->color_toolButton->setStyleSheet("background: " + color.name() + ";");
}

QColor BooleanChartConditionInput::getColor() const
{
    return m_color;
}

void BooleanChartConditionInput::setRemoveButtonEnabled(bool value)
{
    ui->remove_toolButton->setEnabled(value);
}

void BooleanChartConditionInput::setCondition(const QString &value)
{
    ui->condition_lineEdit->setText(value);
    m_validationTimer.start(0);
}

QString BooleanChartConditionInput::getCondition() const
{
    return ui->condition_lineEdit->text();
}

void BooleanChartConditionInput::setJsEngine(QJSEngine *engine)
{
    m_engine = engine;
}

QJSEngine *BooleanChartConditionInput::getJsEngine() const
{
    return m_engine;
}

void BooleanChartConditionInput::setCompleter(QCompleter *completer)
{
    ui->condition_lineEdit->setCompleter(completer);
}

QCompleter *BooleanChartConditionInput::getCompleter() const
{
    return ui->condition_lineEdit->completer();
}

void BooleanChartConditionInput::on_remove_toolButton_clicked()
{
    Q_EMIT removeButtonClicked();
}

void BooleanChartConditionInput::on_add_toolButton_clicked()
{
    Q_EMIT addButtonClicked();
}

void BooleanChartConditionInput::on_color_toolButton_clicked()
{
    auto color = QColorDialog::getColor(getColor(), this, "Pick the color for condition");
    if(color.isValid())
        setColor(color);
}

void BooleanChartConditionInput::on_condition_lineEdit_textEdited(const QString &arg1)
{
    m_validationTimer.start();
}

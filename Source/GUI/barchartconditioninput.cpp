#include "GUI/barchartconditioninput.h"
#include "ui_barchartconditioninput.h"
#include <QColorDialog>
#include <QJSValueList>
#include <QList>
#include <QStringBuilder>
#include <cassert>

BarchartConditionInput::BarchartConditionInput(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BarchartConditionInput),
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
                ui->condition_lineEdit->setToolTip("No condition\n\n" + getTooltipHelp());
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
                        ui->condition_lineEdit->setToolTip("Success\n\n" + getTooltipHelp());
                    }
                }
            }

            auto palette = ui->condition_lineEdit->palette();
            palette.setColor(QPalette::Text, color);

            ui->condition_lineEdit->setPalette(palette);
        });

}

BarchartConditionInput::~BarchartConditionInput()
{
    delete ui;
}

void BarchartConditionInput::setColor(const QColor &color)
{
    m_color = color;
    ui->color_toolButton->setStyleSheet("background: " + color.name() + ";");
}

QColor BarchartConditionInput::getColor() const
{
    return m_color;
}

void BarchartConditionInput::setRemoveButtonEnabled(bool value)
{
    ui->remove_toolButton->setEnabled(value);
}

void BarchartConditionInput::setName(const QString &name)
{
    ui->label_lineEdit->setText(name);
}

QString BarchartConditionInput::getName() const
{
    return ui->label_lineEdit->text();
}

void BarchartConditionInput::setEliminateSpikes(bool eliminateSpikes)
{
    ui->eliminateSpikes_checkBox->setChecked(eliminateSpikes);
}

bool BarchartConditionInput::getEliminateSpikes() const
{
    return ui->eliminateSpikes_checkBox->isChecked();
}

void BarchartConditionInput::setCondition(const QString &value)
{
    ui->condition_lineEdit->setText(value);
    m_validationTimer.start(0);
}

QString BarchartConditionInput::getCondition() const
{
    return ui->condition_lineEdit->text();
}

void BarchartConditionInput::setJsEngine(QJSEngine *engine)
{
    m_engine = engine;
}

QJSEngine *BarchartConditionInput::getJsEngine() const
{
    return m_engine;
}

void BarchartConditionInput::setCompleter(QCompleter *completer)
{
    Q_ASSERT(m_engine);
    ui->condition_lineEdit->setCompleter(completer);
    m_autocompletion = m_engine->property("autocomplete").value<QList<QPair<QString, QString>>>();
}

QCompleter *BarchartConditionInput::getCompleter() const
{
    return ui->condition_lineEdit->completer();
}

QString BarchartConditionInput::getTooltipHelp() const
{
    QString help("help: \n");

    for(auto item : m_autocompletion)
    {
        help.append(QString("%1 - %2\n").arg(item.first).arg(item.second));
    }

    return help;
}

void BarchartConditionInput::on_remove_toolButton_clicked()
{
    Q_EMIT removeButtonClicked();
}

void BarchartConditionInput::on_add_toolButton_clicked()
{
    Q_EMIT addButtonClicked();
}

void BarchartConditionInput::on_color_toolButton_clicked()
{
    auto color = QColorDialog::getColor(getColor(), this, "Pick the color for condition");
    if(color.isValid())
        setColor(color);
}

void BarchartConditionInput::on_condition_lineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);

    m_validationTimer.start();
}

void BarchartConditionInput::on_condition_lineEdit_editingFinished()
{
    m_validationTimer.start();
}

void BarchartConditionInput::on_condition_lineEdit_cursorPositionChanged(int arg1, int arg2)
{
    Q_UNUSED(arg1);
    Q_UNUSED(arg2);

    m_validationTimer.start();
}

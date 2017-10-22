#include "GUI/booleanchartconditioninput.h"
#include "GUI/booleanchartconditioneditor.h"
#include "ui_booleanchartconditioneditor.h"
#include <QCompleter>
#include <QJSValueIterator>
#include <QStandardItemModel>
#include <cassert>

BooleanChartConditionEditor::BooleanChartConditionEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BooleanChartConditionEditor)
{
    ui->setupUi(this);

    getCondition(0)->setRemoveButtonEnabled(false);
    connect(getCondition(0), SIGNAL(addButtonClicked()), this, SLOT(addCondition()));
}

BooleanChartConditionEditor::~BooleanChartConditionEditor()
{
    delete ui;
}

BooleanChartConditionInput *BooleanChartConditionEditor::getCondition(int index) const
{
    return static_cast<BooleanChartConditionInput*> (ui->verticalLayout->itemAt(index)->widget());
}

int BooleanChartConditionEditor::conditionsCount() const
{
    return ui->verticalLayout->count();
}

void BooleanChartConditionEditor::setLabel(const QString &label)
{
    ui->label->setText(label);
}

void BooleanChartConditionEditor::setDefaultColor(const QColor &color)
{
    m_defaultColor = color;
    getCondition(0)->setColor(m_defaultColor);
}

void BooleanChartConditionEditor::setConditions(const PlotSeriesData::Conditions &value)
{
    class CompleterModel : public QStandardItemModel {
    public:
        typedef QPair<QString, QString> Word;
        typedef QList<Word> Words;
        QVariant data(const QModelIndex &index, int role) const
        {
            if(role == Qt::DisplayRole)
                return QStandardItemModel::data(index, Qt::UserRole + 1).toString() + " - " + QStandardItemModel::data(index, Qt::UserRole + 2).toString();

            if(role == Qt::EditRole)
                return QStandardItemModel::data(index, Qt::UserRole + 1);

            return QStandardItemModel::data(index, role);
        }

        void setWords(const Words& words) {
            for(auto& word : words) {
                auto data = new QStandardItem;
                data->setData(word.first, Qt::UserRole + 1);
                data->setData(word.second, Qt::UserRole + 2);
                appendRow(data);
            }
        }
    };

    CompleterModel* model = new CompleterModel();

    CompleterModel::Words words = value.engine.property("autocomplete").value<CompleterModel::Words>();
    model->setWords(words);

    QCompleter *completer = new QCompleter(model, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    if(value.items.size() == 0)
    {
        while(conditionsCount() != 1)
            removeCondition();

        auto condition = getCondition(0);
        assert(condition);

        condition->setCompleter(completer);
        condition->setJsEngine(&value.engine);
        condition->setColor(m_defaultColor);
        condition->setCondition(QString());
    }
    else
    {
        if(conditionsCount() != value.items.size())
        {
            if(conditionsCount() > value.items.size())
            {
                while(conditionsCount() != value.items.size())
                    removeCondition();
            }
            else
            {
                while(conditionsCount() != value.items.size())
                    addCondition();
            }
        }

        for(auto i = 0; i < conditionsCount(); ++i)
        {
            auto condition = getCondition(i);
            assert(condition);

            condition->setCompleter(completer);
            condition->setJsEngine(&value.engine);
            condition->setColor(value.items[i].m_color);
            condition->setCondition(value.items[i].m_conditionString);
        }
    }
}

void BooleanChartConditionEditor::addCondition()
{
    int index = ui->verticalLayout->indexOf(static_cast<QWidget*>(sender()));

    auto input = new BooleanChartConditionInput();
    input->setCompleter(getCondition(0)->getCompleter());
    input->setJsEngine(getCondition(0)->getJsEngine());
    input->setColor(m_defaultColor);
    ui->verticalLayout->insertWidget(index + 1, input);

    connect(input, SIGNAL(addButtonClicked()), this, SLOT(addCondition()));
    connect(input, SIGNAL(removeButtonClicked()), this, SLOT(removeCondition()));
}

void BooleanChartConditionEditor::removeCondition()
{
    auto widget = sender() != nullptr ? static_cast<QWidget*>(sender()) :
                                        ui->verticalLayout->itemAt(ui->verticalLayout->count() - 1)->widget();

    widget->deleteLater();
    ui->verticalLayout->removeWidget(widget);
}

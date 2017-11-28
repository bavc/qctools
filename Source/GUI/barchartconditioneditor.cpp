#include "GUI/barchartconditioninput.h"
#include "GUI/barchartconditioneditor.h"
#include "ui_barchartconditioneditor.h"
#include <QCompleter>
#include <QJSValueIterator>
#include <QStandardItemModel>

BarchartConditionEditor::BarchartConditionEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BarchartConditionEditor)
{
    ui->setupUi(this);

    getCondition(0)->setRemoveButtonEnabled(false);
    connect(getCondition(0), SIGNAL(addButtonClicked()), this, SLOT(addCondition()));
}

BarchartConditionEditor::~BarchartConditionEditor()
{
    delete ui;
}

BarchartConditionInput *BarchartConditionEditor::getCondition(int index) const
{
    return static_cast<BarchartConditionInput*> (ui->verticalLayout->itemAt(index)->widget());
}

int BarchartConditionEditor::conditionsCount() const
{
    return ui->verticalLayout->count();
}

void BarchartConditionEditor::setLabel(const QString &label)
{
    ui->groupBox->setTitle(label);
}

void BarchartConditionEditor::setDefaultColor(const QColor &color)
{
    m_defaultColor = color;
    getCondition(0)->setColor(m_defaultColor);
}

QCompleter* BarchartConditionEditor::makeCompleter(QJSEngine& engine)
{
    class CompleterModel : public QStandardItemModel {
    public:
        typedef QPair<QString, QString> Word;
        typedef QList<Word> Words;

        CompleterModel(QObject* parent = nullptr) : QStandardItemModel(parent) {
        }

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

    QCompleter *completer = new QCompleter(this);
    CompleterModel* model = new CompleterModel(completer);

    CompleterModel::Words words = engine.property("autocomplete").value<CompleterModel::Words>();
    model->setWords(words);

    completer->setModel(model);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    return completer;
}

void BarchartConditionEditor::setConditions(const PlotSeriesData::Conditions &value)
{
    auto condition = getCondition(0);
    Q_ASSERT(condition);
    condition->setJsEngine(&value.m_engine);

    auto completer = makeCompleter(value.m_engine);

    if(value.m_items.size() == 0)
    {
        while(conditionsCount() != 1)
            removeCondition();

        condition->setCompleter(completer);
        condition->setColor(m_defaultColor);
        condition->setName(QString());
        condition->setCondition(QString());
    }
    else
    {
        if(conditionsCount() != value.m_items.size())
        {
            if(conditionsCount() > value.m_items.size())
            {
                while(conditionsCount() != value.m_items.size())
                    removeCondition();
            }
            else
            {
                while(conditionsCount() != value.m_items.size())
                    addCondition();
            }
        }

        for(auto i = 0; i < conditionsCount(); ++i)
        {
            auto condition = getCondition(i);
            Q_ASSERT(condition);

            condition->setJsEngine(&value.m_engine);
            condition->setCompleter(completer);
            condition->setColor(value.m_items[i].m_color);
            condition->setName(value.m_items[i].m_name);
            condition->setCondition(value.m_items[i].m_conditionString);
        }
    }
}

void BarchartConditionEditor::onConditionsUpdated()
{
    Q_ASSERT(getCondition(0));
    if(!getCondition(0)->getJsEngine())
        return;

    auto completer = makeCompleter(*getCondition(0)->getJsEngine());
    for(auto i = 0; i < conditionsCount(); ++i)
    {
        getCondition(i)->setCompleter(completer);
    }
}

void BarchartConditionEditor::addCondition()
{
    int index = ui->verticalLayout->indexOf(static_cast<QWidget*>(sender()));
    if(index == -1) {
        index = ui->verticalLayout->count() - 1;
    }

    auto input = new BarchartConditionInput();
    input->setJsEngine(getCondition(0)->getJsEngine());
    input->setCompleter(getCondition(0)->getCompleter());
    input->setColor(m_defaultColor);
    ui->verticalLayout->insertWidget(index + 1, input);

    connect(input, SIGNAL(addButtonClicked()), this, SLOT(addCondition()));
    connect(input, SIGNAL(removeButtonClicked()), this, SLOT(removeCondition()));
}

void BarchartConditionEditor::removeCondition()
{
    auto widget = sender() != nullptr ? static_cast<QWidget*>(sender()) :
                                        ui->verticalLayout->itemAt(ui->verticalLayout->count() - 1)->widget();

    widget->deleteLater();
    ui->verticalLayout->removeWidget(widget);
}

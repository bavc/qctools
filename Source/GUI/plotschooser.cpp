#include "plotschooser.h"
#include "ui_plotschooser.h"

#include "Core/Core.h"
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QHeaderView>
#include <QSet>
#include <QVariantList>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QMimeData>

constexpr int NAME_COLUMN = 1;
constexpr int ICON_COLUMN = 0;
constexpr int IconSize = 20;

class NoSelectionDelegate : public QStyledItemDelegate
{
    void paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
    {
        QStyleOptionViewItem itemOption(option);

        if (itemOption.state & QStyle::State_HasFocus)
        {
            itemOption.state ^= QStyle::State_HasFocus;
        }

        QStyledItemDelegate::paint(painter, itemOption, index);
    }
};

struct Row {
    QString name;
    quint64 type;
    quint64 group;
    QString description;
    Qt::CheckState checked = { Qt::Unchecked };
};

constexpr int TypeRole = Qt::UserRole + 1;
constexpr int GroupRole = Qt::UserRole + 2;

class TableModel : public QAbstractTableModel
{
public:

    QList<Row> m_data;

    TableModel(PlotsChooser* plotsChooser) : m_plotsChooser(plotsChooser) {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        Q_UNUSED(parent);
        return m_data.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        Q_UNUSED(parent);
        return 2;
    }

    QVariant data(const QModelIndex &i, int r) const
    {
        auto& row = m_data[i.row()];

        if(r == GroupRole)
            return row.group;
        else if(r == TypeRole)
            return row.type;
        if(r == Qt::ToolTipRole)
            return row.description;

        if(i.column() == NAME_COLUMN)
        {
            if(r == Qt::CheckStateRole)
                return row.checked;
            else if(r == Qt::DisplayRole)
                return row.name;
        }
        else if(i.column() == ICON_COLUMN && (r == Qt::DecorationRole || r == Qt::SizeHintRole))
        {
            static auto graphsIcon = QPixmap(":/icon/signalserver_success.png").scaled(IconSize, IconSize);
            static auto commentsIcon = QPixmap(":/icon/signalserver_not_uploaded.png").scaled(IconSize, IconSize);
            static auto panelsIcon = QPixmap(":/icon/signalserver_error.png").scaled(IconSize, IconSize);

            auto type = row.type;
            QPixmap pixmap;

            if(type < Type_Max)
                pixmap = graphsIcon;
            else if(type == Type_Comments)
                pixmap = commentsIcon;
            else if(type == Type_Panels)
                pixmap = panelsIcon;

            if(r == Qt::DecorationRole)
                return pixmap;
            else if(r == Qt::SizeHintRole)
                return pixmap.size();
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int r) const
    {
        Q_UNUSED(section);
        Q_UNUSED(orientation);
        Q_UNUSED(r);

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

        if(!index.isValid()) {
            defaultFlags |= Qt::ItemIsDropEnabled;
        } else {
            defaultFlags |= Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;

            if(index.column() == NAME_COLUMN)
                defaultFlags |= Qt::ItemIsUserCheckable;
        }

        return defaultFlags;
    }

    Qt::DropActions supportedDropActions() const
    {
        return Qt::MoveAction | Qt::CopyAction;
    }

    bool setData(const QModelIndex &i, const QVariant &v, int r)
    {
        auto & row = m_data[i.row()];

        if(i.column() == NAME_COLUMN) {
            if(r == Qt::DisplayRole) {
                row.name = v.toString();
                return true;
            } else if(r == Qt::CheckStateRole) {
                row.checked = (Qt::CheckState) v.toInt();
                Q_EMIT m_plotsChooser->selected(row.checked == Qt::Checked, row.group, row.type);
                return true;
            }
        }

        if(r == TypeRole) {
            row.type = v.toULongLong();
        } else if(r == GroupRole) {
            row.group = v.toULongLong();
        } else if(r == Qt::ToolTipRole) {
            row.description = v.toString();
        }

        return true;
    }

    bool setItemData(const QModelIndex &i, const QMap<int, QVariant> &roles)
    {
        auto& row = m_data[i.row()];

        row.name = roles[Qt::DisplayRole].toString();
        row.checked = (Qt::CheckState) roles[Qt::CheckStateRole].toInt();
        row.description = roles[Qt::ToolTipRole].toString();
        row.type = roles[TypeRole].toULongLong();
        row.group = roles[GroupRole].toULongLong();

        return true;
    }

    QStringList mimeTypes() const override {
        QStringList defaultMimeTypes = QAbstractItemModel::mimeTypes();
        defaultMimeTypes << "group" << "type";
        return defaultMimeTypes;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override {
        auto defaultMimeData = QAbstractItemModel::mimeData(indexes);
        for(auto index : indexes) {
            auto group = data(index, GroupRole).toULongLong();
            auto type = data(index, TypeRole).toULongLong();
            defaultMimeData->setData("group", QByteArray().setNum(group));
            defaultMimeData->setData("type", QByteArray().setNum(type));
        }
        return defaultMimeData;
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
    {
        qDebug() << "dropMimeData: " << "action" << action << "row" << row << "column" << column;

        Q_UNUSED(parent);
        Q_UNUSED(column);

        if(row == -1) {
            row = rowCount(QModelIndex());
        }

        auto dropSuccess = QAbstractTableModel::dropMimeData(data, action, row, 0, parent);

        if(dropSuccess)
        {
            if(data->hasFormat("group")) {
                auto groupByteArray = data->data("group");
                quint64 group = groupByteArray.toULongLong();
                setData(index(row, 0), group, GroupRole);
            }

            if(data->hasFormat("type")) {
                auto typeByteArray = data->data("type");
                quint64 type = typeByteArray.toULongLong();
                setData(index(row, 0), type, TypeRole);
            }

            QTimer::singleShot(0, [&]() {
                Q_EMIT m_plotsChooser->orderChanged();
            });
        }

        qDebug() << "dropMimeData finished: " << "action" << action << "row" << row << "column" << column;
        return dropSuccess;
    }

    bool insertRows(int row, int count, const QModelIndex &parent)
    {
        qDebug() << "insertRows: " << "from" << row << "count" << count;

        Q_UNUSED(parent);

        beginInsertRows(parent, row, row + count - 1);
        for(int i = 0; i<count; ++i) {
            m_data.insert(row, Row());
        }
        endInsertRows();

        qDebug() << "insertRows finished: count = " << m_data.size();
        // m_plotsChooser->getFilterSelectorsOrder();
        return true;
    }

    bool removeRows(int row, int count, const QModelIndex &parent)
    {
        qDebug() << "removeRows: " << "from" << row << "count" << count;

        Q_UNUSED(parent);

        beginRemoveRows(parent, row, row + count - 1);
        for(int i = 0; i<count; ++i) {
            m_data.removeAt(row);
        }
        endRemoveRows();

        qDebug() << "removeRows finished: count = " << m_data.size();
        // m_plotsChooser->getFilterSelectorsOrder();
        return true;
    }

    void beginModelReset() {
        QAbstractTableModel::beginResetModel();
    }

    void endModelReset() {
        QAbstractItemModel::endResetModel();
    }

    bool moveRows(const QModelIndex &srcParent, int srcRow, int count,
                              const QModelIndex &dstParent, int dstChild)
    {
        qDebug() << "moveRows: " << "from" << srcRow << "count" << count << "to" << dstChild;

        Q_UNUSED(srcParent);
        Q_UNUSED(dstParent);

        // beginMoveRows(srcParent, srcRow, srcRow + count - 1, dstParent, dstChild);
        for(int i = 0; i<count; ++i) {
            m_data.insert(dstChild + i, m_data[srcRow]);
            int removeIndex = dstChild > srcRow ? srcRow : srcRow+1;
            m_data.removeAt(removeIndex);
        }
        // endMoveRows();

        qDebug() << "moveRows finished: count = " << m_data.size();
        // m_plotsChooser->getFilterSelectorsOrder();
        return true;
    }

private:
    PlotsChooser* m_plotsChooser;
};

class FilteringModel : public QSortFilterProxyModel {
private:
    std::function<bool (quint64, quint64)> m_filterCriteria;
    PlotsChooser* m_plotsChooser;
public:
    FilteringModel(PlotsChooser* plotsChooser) : m_plotsChooser(plotsChooser) {
    }

    void setFilterCriteria(const std::function<bool (quint64, quint64)> &filterCriteria) {
        m_filterCriteria = filterCriteria;
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
        Q_UNUSED(source_parent);

        if(!m_filterCriteria)
            return true;

        auto type = sourceModel()->data(sourceModel()->index(source_row, NAME_COLUMN), TypeRole).toULongLong();
        auto group = sourceModel()->data(sourceModel()->index(source_row, NAME_COLUMN), GroupRole).toULongLong();

        auto result = m_filterCriteria(type, group);
        qDebug() << "filterAcceptsRow => type: " << type << "group: " << group << "result: " << result;
        return result;
    }
};

void setupView(QTableView &t)
{
    t.verticalHeader()->hide();
    t.horizontalHeader()->hide();
    t.horizontalHeader()->setStretchLastSection(true);

    t.setSelectionBehavior(QAbstractItemView::SelectRows);
    t.setSelectionMode(QAbstractItemView::SingleSelection);

    t.setDragEnabled(true);
    t.setDefaultDropAction(Qt::MoveAction);
    t.setDragDropMode(QTableView::InternalMove);
    t.setDropIndicatorShown(true);
    t.setAcceptDrops(true);
    t.setDragDropOverwriteMode(false);

    t.verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t.verticalHeader()->setDefaultSectionSize(20);
    t.setItemDelegate(new NoSelectionDelegate());

    auto p = t.palette();
    p.setColor(QPalette::Highlight, t.palette().color(QPalette::Base));
    p.setColor(QPalette::HighlightedText, t.palette().color(QPalette::Text));
    t.setPalette(p);
};

PlotsChooser::PlotsChooser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotsChooser)
{
    ui->setupUi(this);

    m_sourceModel = new TableModel(this);

    m_filteringModel = new FilteringModel(this);
    m_filteringModel->setSourceModel(m_sourceModel);

    ui->tableView->setModel(m_filteringModel);
    setupView(*ui->tableView);
    setWindowFlag(Qt::Tool);
}

PlotsChooser::~PlotsChooser()
{
    delete ui;
}

void PlotsChooser::selectFilters(const QStringList &selectedFilters)
{
    QSet<QString> selectedFiltersSet = QSet<QString>::fromList(selectedFilters);
    auto model = static_cast<FilteringModel*> (ui->tableView->model());
    // auto model = sfpModel->sourceModel();

    for(auto i = 0; i < model->rowCount(); ++i)
    {
        auto name = model->data(model->index(i, NAME_COLUMN)).toString();
        auto filterIsSelected = selectedFilters.contains(name);

        model->setData(model->index(i, NAME_COLUMN), filterIsSelected ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
}

void PlotsChooser::add(const QString &name, quint64 type, quint64 group, const QString &description, bool selected)
{
    auto model = m_sourceModel;
    // auto model = static_cast<FilteringModel*> (ui->tableView->model());
    // auto model = sfpModel->sourceModel();
    model->insertRows(model->rowCount(), 1, QModelIndex());

    model->setData(model->index(model->rowCount() - 1, NAME_COLUMN), name, Qt::DisplayRole);
    model->setData(model->index(model->rowCount() - 1, NAME_COLUMN), description, Qt::ToolTipRole);
    model->setData(model->index(model->rowCount() - 1, NAME_COLUMN), type, TypeRole);
    model->setData(model->index(model->rowCount() - 1, NAME_COLUMN), group, GroupRole);
    model->setData(model->index(model->rowCount() - 1, NAME_COLUMN), (selected ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);

    qDebug() << "added filter entry: " << "\n"
             << "\tname = " << name << "\n"
             << "\tchecked = " << false << "\n"
             << "\tdescription = " << description << "\n"
             << "\ttype = " << type << "\n"
             << "\tgroup = " << group << "\n"
             << "\tselected = " << selected << "\n";
}

void PlotsChooser::remove(const QString &name)
{
    for(int row = 0; row < m_sourceModel->rowCount(); ++row) {
        auto rowName = m_sourceModel->data(m_sourceModel->index(row, NAME_COLUMN), Qt::DisplayRole).toString();
        if(rowName == name)
            m_sourceModel->removeRows(row, 1, QModelIndex());
    }
}

void PlotsChooser::setFilterCriteria(const std::function<bool (quint64, quint64)> &filterCriteria)
{
    m_filteringModel->setFilterCriteria(filterCriteria);
    m_filteringModel->invalidate();
}

QList<QString> PlotsChooser::getAvailableFilters(const std::function<bool (quint64)> &criteria)
{
    QList<QString> filterNames;

    for(int row = 0; row < m_sourceModel->rowCount(); ++row) {
        auto modelGroup = m_sourceModel->data(m_sourceModel->index(row, NAME_COLUMN), GroupRole).toULongLong();
        auto modelType = m_sourceModel->data(m_sourceModel->index(row, NAME_COLUMN), TypeRole).toULongLong();

        if(criteria(modelType))
            filterNames.append(m_sourceModel->data(m_sourceModel->index(row, NAME_COLUMN), Qt::DisplayRole).toString());
    }

    return filterNames;
}

void PlotsChooser::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    ui->tableView->resizeColumnToContents(0);
    ui->tableView->resizeColumnToContents(1);

    QRect rect = ui->tableView->geometry();
    rect.setWidth(2 + ui->tableView->verticalHeader()->width() + ui->tableView->columnWidth(0) + ui->tableView->columnWidth(1));

    auto height = ui->tableView->horizontalHeader()->height() + 2;
    for(int row = 0; row < ui->tableView->model()->rowCount(); ++row)
        height += ui->tableView->rowHeight(row);

    rect.setHeight(height);
    ui->tableView->setGeometry(rect);

    auto minSize = ui->tableView->minimumSize();
    ui->tableView->setMinimumSize(rect.size());
    adjustSize();
    ui->tableView->setMinimumSize(minSize);
}

QStringList PlotsChooser::getSelectedFilters(QMap<QString, std::tuple<quint64, quint64>>* map) const
{
    QStringList selectedFilters;
    auto model = ui->tableView->model();
    for(auto i = 0; i < model->rowCount(); ++i) {
        auto modelIndex = model->index(i, NAME_COLUMN);
        auto value = model->data(modelIndex, Qt::CheckStateRole).toBool();
        if(value) {
            auto selectedFilter = model->data(model->index(i, NAME_COLUMN)).toString();
            selectedFilters << selectedFilter;

            if(map)
            {
                auto group = model->data(modelIndex, GroupRole).toULongLong();
                auto type = model->data(modelIndex, TypeRole).toULongLong();

                map->insert(selectedFilter, std::tuple<quint64, quint64>(group, type));
            }
        }
    }

    return selectedFilters;
}

QList<std::tuple<quint64, quint64> > PlotsChooser::getFilterSelectorsOrder(int start, int end)
{
    QList<std::tuple<quint64, quint64>> filtersInfo;
    if(end == -1)
        end = ui->tableView->model()->rowCount() - 1;

    qDebug() << "getFilterSelectorsOrder: ";

    for(int i = start; i <= end; ++i)
    {
        auto model = ui->tableView->model();
        auto group = model->data(model->index(i, NAME_COLUMN), GroupRole).toULongLong();
        auto type = model->data(model->index(i, NAME_COLUMN), TypeRole).toULongLong();

        qDebug() << "\tgroup = " << group << ", type = " << type;

        filtersInfo.push_back(std::make_tuple(group, type));
    }

    qDebug() << "getFilterSelectorsOrder: size(): " << filtersInfo.size();

    return filtersInfo;
}

void PlotsChooser::changeOrder(QList<std::tuple<quint64, quint64>> filtersInfo)
{
    auto model = m_sourceModel;
    auto newOrder = std::vector<int>();

    for(std::tuple<quint64, quint64> groupAndType : filtersInfo)
    {
        quint64 group = std::get<0>(groupAndType);
        quint64 type = std::get<1>(groupAndType);

        auto rowsCount = model->rowCount();
        for(auto row = 0; row < rowsCount; ++row)
        {
            auto modelGroup = model->data(model->index(row, NAME_COLUMN), GroupRole).toULongLong();
            auto modelType = model->data(model->index(row, NAME_COLUMN), TypeRole).toULongLong();

            if(modelGroup == group && modelType == type) {
                newOrder.push_back(row);
                qDebug() << "new order: " << "group: " << group << "type: " << type << "row: " << row;
            }
        }
    };

    model->beginModelReset();
    for(auto i = 0; i < newOrder.size(); ++i) {
        auto newIndex = i;
        auto oldIndex = newOrder[i];

        if(newIndex != oldIndex) {
            model->moveRow(QModelIndex(), oldIndex, QModelIndex(), newIndex);
        }
    }
    model->endModelReset();
}

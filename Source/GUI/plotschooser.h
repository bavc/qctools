#ifndef PLOTSCHOOSER_H
#define PLOTSCHOOSER_H

#include <QWidget>
#include <QList>
#include <tuple>
#include <unordered_set>
#include <QSortFilterProxyModel>

namespace Ui {
class PlotsChooser;
}

class FilteringModel;
class TableModel;
class PlotsChooser : public QWidget
{
    Q_OBJECT

public:
    explicit PlotsChooser(QWidget *parent = nullptr);
    ~PlotsChooser();

    void selectFilters(const QStringList& selectedFilters);
    QStringList getSelectedFilters(QMap<QString, std::tuple<quint64, quint64>>* filtersMap = nullptr) const;

    QList<std::tuple<quint64, quint64>> getFilterSelectorsOrder(int start = 0, int end = -1);

    void changeOrder(QList<std::tuple<quint64, quint64>> filtersInfo);
    void add(const QString& name, quint64 type, quint64 group, const QString& descriptoin, bool selected = false);
    void remove(const QString& name);

    void setFilterCriteria(const std::function<bool(quint64 type, quint64 group)> & filterCriteria);
    QList<QString> getAvailableFilters(const std::function<bool(quint64 type)> & criteria);

protected:
    void showEvent(QShowEvent* e);

Q_SIGNALS:
    void orderChanged();
    void selected(bool checked, quint64 group, quint64 type);

private:
    FilteringModel* m_filteringModel;
    TableModel* m_sourceModel;

    Ui::PlotsChooser *ui;
};

#endif // PLOTSCHOOSER_H

#ifndef FILTERSELECTOR_H
#define FILTERSELECTOR_H

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>
#include <QPointer>
#include "doublespinboxwithslider.h"
#include "filters.h"

class FileInformation;
class FilterSelector : public QFrame
{
    Q_OBJECT
public:
    FilterSelector(QWidget* parent = nullptr, const std::function<bool(const char*)>& nameFilter = {});

    // Content
    struct options
    {
        QCheckBox*              EnableCheckbox;
        QCheckBox*              Checks[Args_Max];
        QLabel*                 Sliders_Label[Args_Max];
        DoubleSpinBoxWithSlider* Sliders_SpinBox[Args_Max];
        QRadioButton*           Radios[Args_Max][4];
        QButtonGroup*           Radios_Group[Args_Max];
        QPushButton*            ColorButton[Args_Max];
        int                     ColorValue[Args_Max];
        QComboBox*              FiltersList;
        QLabel*                 FiltersList_Fake;
    };

    options& getOptions();

    int getPhysicalFilterIndex(int displayFilterIndex);
    void setCurrentFilter(int index);
    void enableCurrentFilter(bool enable);

    struct previous_values
    {
        int                     Values[Args_Max];

        previous_values()
        {
            for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
                Values[OptionPos]=-2;
        }
    };

    void setFileInformation(FileInformation* fileInformation);
    void setCurrentIndex(int index);
    void selectCurrentFilter(int index);

    QString getFilter();

Q_SIGNALS:
    void filterChanged(const QString& filterString);

private Q_SLOTS:
    void on_FiltersOptions_click();
    void hideOthersOnEntering(DoubleSpinBoxWithSlider* doubleSpinBox, DoubleSpinBoxWithSlider** others);
    void on_FiltersList_currentIndexChanged(int Pos);

protected:
    void FiltersList_currentIndexChanged(int FilterPos, QGridLayout *Layout0);

    void on_FiltersList_currentOptionChanged(int filterIndex);
    std::string FiltersList_currentOptionChanged(int Picture_Current);

protected Q_SLOTS:
    void on_FiltersSpinBox1_click();
    void on_FiltersOptions1_toggle(bool checked);

    void on_Color1_click(bool checked);
private:
    options m_filterOptions;
    std::vector<previous_values> m_previousValues;

    FileInformation* FileInfoData;
    QGridLayout* Layout;

    int m_currentFilterIndex;
};

#endif // FILTERSELECTOR_H

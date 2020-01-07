#include "filterselector.h"

#include "Core/FileInformation.h"
#include "Core/FFmpeg_Glue.h"
#include <QButtonGroup>
#include <QColorDialog>
#include <QGridLayout>
#include <string>
#include <cmath>

FilterSelector::FilterSelector(QWidget *parent, const std::function<bool(const char*)>& nameFilter) : QFrame(parent), FileInfoData(nullptr)
{
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        m_filterOptions.Checks[OptionPos]=nullptr;
        m_filterOptions.Sliders_Label[OptionPos]=nullptr;
        m_filterOptions.Sliders_SpinBox[OptionPos]=nullptr;
        for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
            m_filterOptions.Radios[OptionPos][OptionPos2]=nullptr;
        m_filterOptions.Radios_Group[OptionPos]=nullptr;
        m_filterOptions.ColorButton[OptionPos]=nullptr;
    }
    m_filterOptions.FiltersList_Fake=nullptr;

    m_filterOptions.EnableCheckbox = new QCheckBox(this);
    connect(m_filterOptions.EnableCheckbox, &QCheckBox::toggled, [&](bool toggled) {
       on_FiltersOptions_click();
    });

    m_filterOptions.FiltersList = new QComboBox(this);

    QFont Font=QFont();
#ifdef _WIN32
#else //_WIN32
    Font.setPointSize(Font.pointSize()*3/4);
#endif //_WIN32

    m_filterOptions.FiltersList->setFont(Font);

    typedef QPair<QString, int> FilterInfo;
    typedef QList<FilterInfo> FiltersGroup;
    typedef QVector<FiltersGroup> FiltersGroups;

    struct Sort
    {
        static bool filterInfoLessThan(const FilterInfo& i1, const FilterInfo& i2)
        {
            return i1.first < i2.first;
        }
    };

    FiltersGroups filtersGroups;

    for (size_t FilterPos=0; FilterPos<FiltersListDefault_Count; FilterPos++)
    {
        const char* filterName = Filters[FilterPos].Name;
        if (strcmp(filterName, "(Separator)") && strcmp(filterName, "(End)"))
        {
            if(filtersGroups.empty())
                filtersGroups.push_back(FiltersGroup());

            filtersGroups.back().append(FilterInfo(filterName, FilterPos));
        }
        else if (strcmp(filterName, "(End)"))
        {
            filtersGroups.push_back(FiltersGroup());
        }
    }

    for(int i = 0; i < filtersGroups.length(); ++i)
    {
        FiltersGroup & filterGroup = filtersGroups[i];
        qSort(filterGroup.begin(), filterGroup.end(), Sort::filterInfoLessThan);

        for(FiltersGroup::const_iterator it = filterGroup.cbegin(); it != filterGroup.cend(); ++it)
        {
            auto first = it->first;
            auto second = it->second;

            auto filterNameString = first.toStdString();
            const char* filterName = filterNameString.c_str();
            if(nameFilter && !nameFilter(filterName))
                continue;

            m_filterOptions.FiltersList->addItem(first, second);
        }

        if(i != (filtersGroups.length() - 1))
            m_filterOptions.FiltersList->insertSeparator(FiltersListDefault_Count);
    };

    m_filterOptions.FiltersList->setMinimumWidth(m_filterOptions.FiltersList->minimumSizeHint().width());
    m_filterOptions.FiltersList->setMaximumWidth(m_filterOptions.FiltersList->minimumSizeHint().width());
    m_filterOptions.FiltersList->setMaxVisibleItems(25);

    connect(m_filterOptions.FiltersList, SIGNAL(currentIndexChanged(int)), this, SLOT(on_FiltersList_currentIndexChanged(int)));

    Layout = new QGridLayout();
    Layout->setMargin(0);
    Layout->setContentsMargins(0, 0, 0, 0);
    setLayout(Layout);

    setFrameStyle(QFrame::Box);

    auto minimumSize = QSize(m_filterOptions.FiltersList->minimumWidth(), m_filterOptions.FiltersList->height());
    setMinimumSize(minimumSize);
}

FilterSelector::options &FilterSelector::getOptions()
{
    return m_filterOptions;
}

int FilterSelector::getPhysicalFilterIndex(int displayFilterIndex)
{
    return m_filterOptions.FiltersList->itemData(displayFilterIndex).toInt();
}

void FilterSelector::setCurrentFilter(int filterIndex)
{
    QGridLayout* Layout0=new QGridLayout();
    Layout0->setContentsMargins(0, 0, 0, 0);
    Layout0->setSpacing(8);

    auto hbox = new QHBoxLayout();

    hbox->addWidget(m_filterOptions.EnableCheckbox);
    hbox->addWidget(m_filterOptions.FiltersList);

    Layout0->addLayout(hbox, 0, 0, Qt::AlignLeft);
    FiltersList_currentIndexChanged(filterIndex, Layout0);

    m_filterOptions.FiltersList_Fake=new QLabel(" ");
    Layout0->addWidget(m_filterOptions.FiltersList_Fake, 1, 0, Qt::AlignLeft);
    Layout0->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum), 0, 14);

    Layout->addLayout(Layout0, 0, 0, 1, 1, Qt::AlignLeft|Qt::AlignTop);

    m_currentFilterIndex = filterIndex;
    on_FiltersList_currentOptionChanged(filterIndex);
}

void FilterSelector::enableCurrentFilter(bool enable)
{
    QSignalBlocker blocker(this);
    m_filterOptions.EnableCheckbox->setChecked(enable);
}

void FilterSelector::setFileInformation(FileInformation *fileInformation)
{
    FileInfoData = fileInformation;
}

void FilterSelector::setCurrentIndex(int index)
{
    for(int displayFilterIndex = 0; displayFilterIndex < getOptions().FiltersList->count(); ++displayFilterIndex)
    {
        auto physicalFilterIndex = getPhysicalFilterIndex(displayFilterIndex);

        if(physicalFilterIndex == index)
            getOptions().FiltersList->setCurrentIndex(displayFilterIndex);
    }

    m_currentFilterIndex = index;
}

void FilterSelector::selectCurrentFilter(int index)
{
    QSignalBlocker blocker(this);
    m_filterOptions.FiltersList->setCurrentIndex(index);
}

void FilterSelector::on_FiltersOptions_click()
{
    on_FiltersList_currentOptionChanged(m_currentFilterIndex);
}

void FilterSelector::hideOthersOnEntering(DoubleSpinBoxWithSlider *doubleSpinBox, DoubleSpinBoxWithSlider **others)
{
    connect(doubleSpinBox, &DoubleSpinBoxWithSlider::entered, [others](DoubleSpinBoxWithSlider* control) {
        for (size_t Pos=0; Pos<Args_Max; Pos++)
            if (others[Pos] && others[Pos] != control)
                others[Pos]->hidePopup();
    });
}

void FilterSelector::on_FiltersList_currentOptionChanged(int filterIndex)
{
    if(m_filterOptions.EnableCheckbox->isChecked())
    {
        std::string Modified_String = FiltersList_currentOptionChanged(filterIndex);
        Q_EMIT filterChanged(QString::fromStdString(Modified_String));
    } else
    {
        Q_EMIT filterChanged(QString(""));
    }
}

void FilterSelector::on_FiltersList_currentIndexChanged(int Pos)
{
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    if(combo)
    {
        if(!combo->itemData(Pos).isNull())
            Pos = combo->itemData(Pos).toInt();

        setCurrentFilter(Pos);
    }
}

void FilterSelector::on_Color1_click(bool checked)
{
    QObject* Sender=sender();
    size_t OptionPos=0;
    while (Sender!= m_filterOptions.ColorButton[OptionPos])
        OptionPos++;

    QColor Color=QColorDialog::getColor(QColor(m_filterOptions.ColorValue[OptionPos]), this);
    if (Color.isValid())
    {
        m_filterOptions.ColorValue[OptionPos]=Color.rgb()&0x00FFFFFF;
        on_FiltersList_currentOptionChanged(m_currentFilterIndex);
    }
    hide();
    show();
}

void FilterSelector::on_FiltersSpinBox1_click()
{
    on_FiltersList_currentOptionChanged(m_currentFilterIndex);
}

void FilterSelector::on_FiltersOptions1_toggle(bool checked)
{
    if (checked)
        on_FiltersList_currentOptionChanged(m_currentFilterIndex);
}

std::string FilterSelector::FiltersList_currentOptionChanged(int Picture_Current)
{
    size_t Value_Pos=0;
    bool Modified=false;
    double WithSliders[Args_Max];
    WithSliders[0]=-2;
    WithSliders[1]=-2;
    WithSliders[2]=-2;
    WithSliders[3]=-2;
    string WithRadios[Args_Max];
    string Modified_String;
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        switch (Filters[Picture_Current].Args[OptionPos].Type)
        {
        case Args_Type_Toggle:
            Value_Pos<<=1;
            Value_Pos|=(m_filterOptions.Checks[OptionPos]->isChecked()?1:0);
            m_previousValues[Picture_Current].Values[OptionPos]=m_filterOptions.Checks[OptionPos]->isChecked()?1:0;

            // Special case: "Line", max is source width or height
            if (string(Filters[Picture_Current].Args[OptionPos].Name)=="Columns")
                for (size_t OptionPos2=0; OptionPos2<Args_Max; OptionPos2++)
                    if (Filters[Picture_Current].Args[OptionPos2].Type!=Args_Type_None && string(Filters[Picture_Current].Args[OptionPos2].Name)=="Line")
                    {
                        m_filterOptions.Sliders_SpinBox[OptionPos2]->ChangeMax(m_filterOptions.Checks[OptionPos]->isChecked()?FileInfoData->Glue->Width_Get():FileInfoData->Glue->Height_Get());
                        // Infinite loop in that case
                        //m_filterOptions.Sliders_SpinBox[OptionPos2]->setValue(m_filterOptions.Sliders_SpinBox[OptionPos2]->maximum()/2);
                    }

            // Special case: RGB
            if (string(Filters[Picture_Current].Name)=="Histogram" && m_filterOptions.Checks[1])
            {
                if (m_filterOptions.Checks[1]->isChecked() && m_filterOptions.Radios[2][0]->text()!="R") //RGB
                {
                    m_filterOptions.Radios[2][0]->setText("R");
                    m_filterOptions.Radios[2][1]->setText("G");
                    m_filterOptions.Radios[2][2]->setText("B");
                    m_filterOptions.Radios[2][3]->setChecked(true);
                }
                if (!m_filterOptions.Checks[1]->isChecked() && m_filterOptions.Radios[2][0]->text()!="Y") //YUV
                {
                    m_filterOptions.Radios[2][0]->setText("Y");
                    m_filterOptions.Radios[2][1]->setText("U");
                    m_filterOptions.Radios[2][2]->setText("V");
                    m_filterOptions.Radios[2][3]->setChecked(true);
                }
            }

            break;
        case Args_Type_Slider:
        {
            Modified = true;
            double value = m_filterOptions.Sliders_SpinBox[OptionPos]->value();
            double divisor = Filters[Picture_Current].Args[OptionPos].Divisor;
            WithSliders[OptionPos] = value;
            m_previousValues[Picture_Current].Values[OptionPos] = value * divisor;
        }
            break;
        case Args_Type_Win_Func:
        case Args_Type_Wave_Mode:
            Modified=true;
            for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
            {
                if (m_filterOptions.Radios[OptionPos][OptionPos2] && m_filterOptions.Radios[OptionPos][OptionPos2]->isChecked())
                {
                    WithRadios[OptionPos]=m_filterOptions.Radios[OptionPos][OptionPos2]->text().toUtf8().data();
                    size_t X_Pos=WithRadios[OptionPos].find('x');
                    if (X_Pos!=string::npos)
                        WithRadios[OptionPos].resize(X_Pos);
                    m_previousValues[Picture_Current].Values[OptionPos]=OptionPos2;
                    break;
                }
            }
            break;
        case Args_Type_YuvA:
            Value_Pos<<=1;
            Value_Pos|=m_filterOptions.Radios[OptionPos][3]->isChecked()?1:0; // 3 = pos of "all"
            //No break
        case Args_Type_Yuv:
            Modified=true;
            for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
            {
                if (m_filterOptions.Radios[OptionPos][OptionPos2] && m_filterOptions.Radios[OptionPos][OptionPos2]->isChecked())
                {
                    if (string(Filters[Picture_Current].Name)=="Extract Planes Equalized" || string(Filters[Picture_Current].Name)=="Bit Plane Noise" || string(Filters[Picture_Current].Name)=="Value Highlight" || string(Filters[Picture_Current].Name)=="Field Difference" || string(Filters[Picture_Current].Name)=="Temporal Difference" || string(Filters[Picture_Current].Name)=="Bit Plane (10 slices)")
                    {
                        switch (OptionPos2)
                        {
                        case 0: WithRadios[OptionPos]="y"; break;
                        case 1: WithRadios[OptionPos]="u"; break;
                        case 2: WithRadios[OptionPos]="v"; break;
                        default:;
                        }
                    }
                    else
                    {
                        switch (OptionPos2)
                        {
                        case 0: WithRadios[OptionPos]="1"; break;
                        case 1: WithRadios[OptionPos]="2"; break;
                        case 2: WithRadios[OptionPos]="4"; break;
                        case 3: WithRadios[OptionPos]="7"; break; //Special case: remove plane
                        default:;
                        }
                    }
                    m_previousValues[Picture_Current].Values[OptionPos]=OptionPos2;
                    break;
                }
            }
            break;
        case Args_Type_Ranges:
            Value_Pos<<=1;
            Value_Pos|=m_filterOptions.Radios[OptionPos][1]->isChecked()?1:0;
            //No break
        case Args_Type_ClrPck:
            Modified=true;
            WithSliders[OptionPos]=m_filterOptions.ColorValue[OptionPos];
            m_previousValues[Picture_Current].Values[OptionPos]=m_filterOptions.ColorValue[OptionPos];
            break ;
        case Args_Type_ColorMatrix:
            Modified=true;
            for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
            {
                if (m_filterOptions.Radios[OptionPos][OptionPos2] && m_filterOptions.Radios[OptionPos][OptionPos2]->isChecked())
                {
                    switch (OptionPos2)
                    {
                    case 0: WithRadios[OptionPos]="bt601"; break;
                    case 1: WithRadios[OptionPos]="bt709"; break;
                    case 2: WithRadios[OptionPos]="smpte240m"; break;
                    case 3: WithRadios[OptionPos]="fcc"; break;
                    default:;
                    }
                    m_previousValues[Picture_Current].Values[OptionPos]=OptionPos2;
                    break;
                }
            }
            break;
        case Args_Type_SampleRange:
            Modified=true;
            for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
            {
                if (m_filterOptions.Radios[OptionPos][OptionPos2] && m_filterOptions.Radios[OptionPos][OptionPos2]->isChecked())
                {
                    switch (OptionPos2)
                    {
                    case 0: WithRadios[OptionPos]="auto"; break;
                    case 1: WithRadios[OptionPos]="full"; break;
                    case 2: WithRadios[OptionPos]="tv"; break;
                    default:;
                    }
                    m_previousValues[Picture_Current].Values[OptionPos]=OptionPos2;
                    break;
                }
            }
            break;
        case Args_Type_LogLin:
            Modified=true;
            for (size_t OptionPos2=0; OptionPos2<(Filters[Picture_Current].Args[OptionPos].Type?4:3); OptionPos2++)
            {
                if (m_filterOptions.Radios[OptionPos][OptionPos2] && m_filterOptions.Radios[OptionPos][OptionPos2]->isChecked())
                {
                    switch (OptionPos2)
                    {
                    case 0: WithRadios[OptionPos]="linear"; break;
                    case 1: WithRadios[OptionPos]="logarithmic"; break;
                    default:;
                    }
                    m_previousValues[Picture_Current].Values[OptionPos]=OptionPos2;
                    break;
                }
            }
            break;
        default:                ;
        }
    }

    Modified_String=Filters[Picture_Current].Formula[Value_Pos];
    if (Modified)
    {
        for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
            switch (Filters[Picture_Current].Args[OptionPos].Type)
            {
            case Args_Type_Slider:
            {
                char ToFind1[3];
                ToFind1[0]='$';
                ToFind1[1]='1'+OptionPos;
                ToFind1[2]='\0';
                char ToFind2[5];
                ToFind2[0]='$';
                ToFind2[1]='{';
                ToFind2[2]='1'+OptionPos;
                ToFind2[3]='}';
                ToFind2[4]='\0';
                for (;;)
                {
                    size_t InsertPos=Modified_String.find(ToFind2);
                    size_t BytesCount=4;
                    if (InsertPos==string::npos)
                    {
                        InsertPos=Modified_String.find(ToFind1);
                        if (InsertPos!=string::npos)
                            BytesCount=2;
                    }
                    if (InsertPos==string::npos)
                        break;
                    Modified_String.erase(InsertPos, BytesCount);
                    Modified_String.insert(InsertPos, QString::number(WithSliders[OptionPos]).toUtf8());
                }
            }
                break;
            case Args_Type_Win_Func:
            case Args_Type_Wave_Mode:
            case Args_Type_Yuv:
            case Args_Type_YuvA:
            case Args_Type_ClrPck:
            case Args_Type_ColorMatrix:
            case Args_Type_SampleRange:
            case Args_Type_LogLin:
            {
                char ToFind1[3];
                ToFind1[0]='$';
                ToFind1[1]='1'+OptionPos;
                ToFind1[2]='\0';
                char ToFind2[5];
                ToFind2[0]='$';
                ToFind2[1]='{';
                ToFind2[2]='1'+OptionPos;
                ToFind2[3]='}';
                ToFind2[4]='\0';
                for (;;)
                {
                    size_t InsertPos=Modified_String.find(ToFind2);
                    size_t BytesCount=4;
                    if (InsertPos==string::npos)
                    {
                        InsertPos=Modified_String.find(ToFind1);
                        if (InsertPos!=string::npos)
                            BytesCount=2;
                    }
                    if (InsertPos==string::npos)
                    {
                        // Special case RGB
                        for (;;)
                        {
                            char ToFind3[4];
                            ToFind3[0]='$';
                            ToFind3[1]='{';
                            ToFind3[2]='1'+OptionPos;
                            ToFind3[3]='\0';
                            InsertPos=Modified_String.find(ToFind3);
                            if (InsertPos==string::npos)
                                break;

                            if (InsertPos+6<Modified_String.size() && (Modified_String[InsertPos+3]=='R' || Modified_String[InsertPos+3]=='G' ||Modified_String[InsertPos+3]=='B') && Modified_String[InsertPos+4]=='}')
                            {
                                int Value=(int)WithSliders[OptionPos];
                                switch(Modified_String[InsertPos+3])
                                {
                                case 'R' : Value>>=16;            ; break;
                                case 'G' : Value>>= 8; Value&=0xFF; break;
                                case 'B' :             Value&=0xFF; break;
                                default  : ;
                                }

                                Modified_String.erase(InsertPos, 5);
                                Modified_String.insert(InsertPos, QString::number(Value).toUtf8());
                            }
                        }
                        break;
                    }
                    Modified_String.erase(InsertPos, BytesCount);
                    if (Filters[Picture_Current].Args[OptionPos].Type==Args_Type_ClrPck)
                        Modified_String.insert(InsertPos, ("0x"+QString::number(m_filterOptions.ColorValue[OptionPos], 16)).toUtf8().data());
                    else
                        Modified_String.insert(InsertPos, WithRadios[OptionPos]);
                }

                // Special case: removing fake "extractplanes=all,"
                size_t RemovePos=Modified_String.find("crop=iw:256:0:all,");
                if (RemovePos==string::npos)
                    RemovePos=Modified_String.find("extractplanes=all,");
                if (RemovePos!=string::npos)
                    Modified_String.erase(RemovePos, 18);
            }
                break;
            default:                ;
            }
    }

    // Variables
    QString str = QString::fromStdString(Modified_String);

    Modified_String = str.toStdString();

    return Modified_String;
}

//---------------------------------------------------------------------------
void FilterSelector::FiltersList_currentIndexChanged(int FilterPos, QGridLayout* Layout0)
{
    // Options
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        delete m_filterOptions.Checks[OptionPos]; m_filterOptions.Checks[OptionPos]=NULL;
        delete m_filterOptions.Sliders_Label[OptionPos]; m_filterOptions.Sliders_Label[OptionPos]=NULL;
        delete m_filterOptions.Sliders_SpinBox[OptionPos]; m_filterOptions.Sliders_SpinBox[OptionPos]=NULL;
        for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
        {
            delete m_filterOptions.Radios[OptionPos][OptionPos2]; m_filterOptions.Radios[OptionPos][OptionPos2]=NULL;
        }
        delete m_filterOptions.Radios_Group[OptionPos]; m_filterOptions.Radios_Group[OptionPos]=NULL;
        delete m_filterOptions.ColorButton[OptionPos]; m_filterOptions.ColorButton[OptionPos]=NULL;
    }
    delete m_filterOptions.FiltersList_Fake; m_filterOptions.FiltersList_Fake=NULL;

    if(FilterPos == -1)
        return;

    Layout0->setContentsMargins(0, 0, 0, 0);
    QFont Font=QFont();
#ifdef _WIN32
#else //_WIN32
    Font.setPointSize(Font.pointSize()*3/4);
#endif //_WIN32
    size_t Widget_XPox=1;
    for (size_t OptionPos=0; OptionPos<Args_Max; OptionPos++)
    {
        if (FilterPos>=m_previousValues.size())
            m_previousValues.resize(FilterPos+1);
        if (m_previousValues[FilterPos].Values[OptionPos]==-2)
        {
            m_previousValues[FilterPos].Values[OptionPos]=Filters[FilterPos].Args[OptionPos].Default;

            //Special case : default for Line Select is Height/2
            if (Filters[FilterPos].Args[OptionPos].Name && std::string(Filters[FilterPos].Args[OptionPos].Name)=="Line")
                m_previousValues[FilterPos].Values[OptionPos]=FileInfoData->Glue->Height_Get()/2;
            //Special case : default for Color Matrix is Width/2
            if (Filters[FilterPos].Args[OptionPos].Name && std::string(Filters[FilterPos].Args[OptionPos].Name)=="Reveal")
                m_previousValues[FilterPos].Values[OptionPos]=FileInfoData->Glue->Width_Get()/2;
        }

        switch (Filters[FilterPos].Args[OptionPos].Type)
        {
        case Args_Type_Toggle:
        {
            m_filterOptions.Checks[OptionPos]=new QCheckBox(Filters[FilterPos].Args[OptionPos].Name);
            m_filterOptions.Checks[OptionPos]->setFont(Font);
            m_filterOptions.Checks[OptionPos]->setChecked(m_previousValues[FilterPos].Values[OptionPos]);
            connect(m_filterOptions.Checks[OptionPos], SIGNAL(stateChanged(int)), this, SLOT(on_FiltersOptions_click()));
            Layout0->addWidget(m_filterOptions.Checks[OptionPos], 0, Widget_XPox);
            Widget_XPox++;
        }
            break;
        case Args_Type_Slider:
        {
            // Special case: "Line", max is source width or height
            int Max;
            QString MaxTemp(Filters[FilterPos].Args[OptionPos].Name);
            if(strcmp(Filters[FilterPos].Name, "Limiter") == 0)
            {
                if (MaxTemp == "Min" || MaxTemp == "Max")
                {
                    int BitsPerRawSample = FileInfoData->Glue->BitsPerRawSample_Get();
                    if (BitsPerRawSample == 0) {
                        BitsPerRawSample = 8; //Workaround when BitsPerRawSample is unknown, we hope it is 8-bit.
                    }
                    Max = pow(2, BitsPerRawSample) - 1;
                    if (Filters[FilterPos].Args[OptionPos].Name && std::string(Filters[FilterPos].Args[OptionPos].Name)=="Max")
                        m_previousValues[FilterPos].Values[OptionPos]=pow(2, BitsPerRawSample) - 1;
                }
                else
                  Max=Filters[FilterPos].Args[OptionPos].Max;
            } else
                if (MaxTemp == "Line")
                {
                    bool SelectWidth = false;
                    for (size_t OptionPos2 = 0; OptionPos2 < Args_Max; OptionPos2++)
                        if (Filters[FilterPos].Args[OptionPos2].Type != Args_Type_None && string(Filters[FilterPos].Args[OptionPos2].Name) == "Vertical")
                            SelectWidth = Filters[FilterPos].Args[OptionPos2].Default ? true : false;
                    Max = SelectWidth ? FileInfoData->Glue->Width_Get() : FileInfoData->Glue->Height_Get();
                }
                else if (MaxTemp == "x" || MaxTemp == "x offset" || MaxTemp == "Reveal" || MaxTemp == "w")
                    Max = FileInfoData->Glue->Width_Get();
                else if (MaxTemp == "y" || MaxTemp == "s" || MaxTemp == "h")
                    Max = FileInfoData->Glue->Height_Get();
                else if (MaxTemp.contains("bit position", Qt::CaseInsensitive) && FileInfoData->Glue->BitsPerRawSample_Get() != 0)
                    Max = FileInfoData->Glue->BitsPerRawSample_Get();
                else
                    Max=Filters[FilterPos].Args[OptionPos].Max;

            m_filterOptions.Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
            m_filterOptions.Sliders_SpinBox[OptionPos]=new DoubleSpinBoxWithSlider(this, Filters[FilterPos].Args[OptionPos].Min, Max, Filters[FilterPos].Args[OptionPos].Divisor, m_previousValues[FilterPos].Values[OptionPos], Filters[FilterPos].Args[OptionPos].Name, QString(Filters[FilterPos].Args[OptionPos].Name).contains(" bit position"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Filter"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Peak"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Mode"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Scale"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("Colorspace"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("DataMode"), QString(Filters[FilterPos].Args[OptionPos].Name).contains("System") || QString(Filters[FilterPos].Args[OptionPos].Name).contains("Gamut"));
            hideOthersOnEntering(m_filterOptions.Sliders_SpinBox[OptionPos], m_filterOptions.Sliders_SpinBox);

            connect(m_filterOptions.Sliders_SpinBox[OptionPos], SIGNAL(valueChanged(double)), this, SLOT(on_FiltersSpinBox1_click()));
            m_filterOptions.Sliders_Label[OptionPos]->setFont(Font);
            if (m_filterOptions.Sliders_SpinBox[OptionPos])
            {
                Layout0->addWidget(m_filterOptions.Sliders_Label[OptionPos], 0, Widget_XPox, 1, 1, Qt::AlignRight);
                Layout0->addWidget(m_filterOptions.Sliders_SpinBox[OptionPos], 0, Widget_XPox+1);
            }
            else
                Layout0->addWidget(m_filterOptions.Sliders_Label[OptionPos], 1, Widget_XPox, 1, 2);
            Widget_XPox+=2;
        }
            break;
        case Args_Type_Win_Func:
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("none"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("hann"); break;
                case 2: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("hamming"); break;
                case 3: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("blackman"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=4;
            break;
        case Args_Type_Wave_Mode:
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("point"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("line"); break;
                case 2: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("p2p"); break;
                case 3: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("cline"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=4;
            break;
        case Args_Type_Yuv:
        case Args_Type_YuvA:
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<(Filters[FilterPos].Args[OptionPos].Type==Args_Type_Yuv?3:4); OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("Y"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("U"); break;
                case 2: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("V"); break;
                case 3: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("A"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=Filters[FilterPos].Args[OptionPos].Type==Args_Type_Yuv?3:4;
            break;
        case Args_Type_Ranges:
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<2; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("above white"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("below black"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=2;
            break;
        case Args_Type_ColorMatrix:
            m_filterOptions.Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
            Layout0->addWidget(m_filterOptions.Sliders_Label[OptionPos], 0, Widget_XPox);
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<4; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("bt601"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("bt709"); break;
                case 2: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("smpte240m"); break;
                case 3: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("fcc"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=5;
            break;
        case Args_Type_SampleRange:
            m_filterOptions.Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
            Layout0->addWidget(m_filterOptions.Sliders_Label[OptionPos], 0, Widget_XPox);
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<3; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("auto"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("full"); break;
                case 2: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("broadcast"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox+=4;
            break;
        case Args_Type_ClrPck:
        {
            m_filterOptions.ColorValue[OptionPos]=m_previousValues[FilterPos].Values[OptionPos];
            m_filterOptions.ColorButton[OptionPos]=new QPushButton("Color");
            m_filterOptions.ColorButton[OptionPos]->setFont(Font);
            m_filterOptions.ColorButton[OptionPos]->setMaximumHeight(m_filterOptions.FiltersList->height());
            connect(m_filterOptions.ColorButton[OptionPos], SIGNAL(clicked (bool)), this, SLOT(on_Color1_click(bool)));
            Layout0->addWidget(m_filterOptions.ColorButton[OptionPos], 0, Widget_XPox);
            Widget_XPox++;
        }
            break;
        case Args_Type_LogLin:
            //m_filterOptions.Sliders_Label[OptionPos]=new QLabel(Filters[FilterPos].Args[OptionPos].Name+QString(": "));
            Layout0->addWidget(m_filterOptions.Sliders_Label[OptionPos], 0, Widget_XPox);
            m_filterOptions.Radios_Group[OptionPos]=new QButtonGroup();
            for (size_t OptionPos2=0; OptionPos2<2; OptionPos2++)
            {
                m_filterOptions.Radios[OptionPos][OptionPos2]=new QRadioButton();
                m_filterOptions.Radios[OptionPos][OptionPos2]->setFont(Font);
                switch (OptionPos2)
                {
                case 0: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("linear"); break;
                case 1: m_filterOptions.Radios[OptionPos][OptionPos2]->setText("log"); break;
                default:;
                }
                if (OptionPos2==m_previousValues[FilterPos].Values[OptionPos])
                    m_filterOptions.Radios[OptionPos][OptionPos2]->setChecked(true);
                connect(m_filterOptions.Radios[OptionPos][OptionPos2], SIGNAL(toggled(bool)), this, SLOT(on_FiltersOptions1_toggle(bool)));
                Layout0->addWidget(m_filterOptions.Radios[OptionPos][OptionPos2], 0, Widget_XPox+1+OptionPos2);
                m_filterOptions.Radios_Group[OptionPos]->addButton(m_filterOptions.Radios[OptionPos][OptionPos2]);
            }
            Widget_XPox++;
            break;
        default:                ;
        }
    }
}


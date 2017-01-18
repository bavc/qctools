/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>
#include <QStandardPaths>
#include <QMetaType>
#include <QDebug>
//---------------------------------------------------------------------------

typedef QPair<int, int> GroupAndType;
Q_DECLARE_METATYPE(GroupAndType)

typedef QList<GroupAndType> FilterSelectorsOrder;
Q_DECLARE_METATYPE(FilterSelectorsOrder)

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);

    //Configuration
    connect(this, SIGNAL(accepted()), this, SLOT(OnAccepted()));
    connect(this, SIGNAL(rejected()), this, SLOT(OnRejected()));

    //Loading preferences
    QCoreApplication::setOrganizationName("MediaArea");
    QCoreApplication::setOrganizationDomain("mediaarea.net");
    QCoreApplication::setApplicationName("QCTools");

    Load();

    qRegisterMetaType<GroupAndType>("GroupAndType");
    qRegisterMetaType<FilterSelectorsOrder>("FilterSelectorsOrder");
    qRegisterMetaTypeStreamOperators<FilterSelectorsOrder>("FilterSelectorsOrder");
}

//---------------------------------------------------------------------------
Preferences::~Preferences()
{
    delete ui;
}

QDataStream &operator<<(QDataStream &out, const FilterSelectorsOrder &order) {

    qDebug() << "serializing total " << order.length() << ": \n";

    FilterSelectorsOrder::const_iterator item;
    for(item = order.begin(); item != order.end(); ++item) {
        qDebug() << "g: " << item->first << ", t: " << item->second;
    }

    FilterSelectorsOrder::const_iterator filterInfo;
    for(filterInfo = order.begin(); filterInfo != order.end(); ++filterInfo) {
        out << filterInfo->first << filterInfo->second;
    }

    return out;
}
QDataStream &operator>>(QDataStream &in, FilterSelectorsOrder &order) {
    while(!in.atEnd())
    {
        int group;
        int type;
        in >> group;
        in >> type;

        QPair<int, int> entry = qMakePair(group, type);
        if(!order.contains(entry))
            order.push_back(entry);
    }

    qDebug() << "deserialized: total " << order.length() << "\n";

    FilterSelectorsOrder::iterator item;
    for(item = order.begin(); item != order.end(); ++item) {
        qDebug() << "g: " << item->first << ", t: " << item->second;
    }

    return in;
}

QList<QPair<int, int> > Preferences::loadFilterSelectorsOrder()
{
    QSettings Settings;

    QList<QPair<int, int> > order = Settings.value("filterSelectorsOrder", QVariant::fromValue(FilterSelectorsOrder())).value<FilterSelectorsOrder>();

    return order;
}

void Preferences::saveFilterSelectorsOrder(const QList<QPair<int, int> > &order)
{
    QSettings Settings;

    Settings.setValue("filterSelectorsOrder", QVariant::fromValue(order));
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void Preferences::Load()
{
    QSettings Settings;
    ActiveFilters=activefilters(Settings.value("ActiveFilters", (1<<ActiveFilter_Video_signalstats)|(1<<ActiveFilter_Video_Psnr)|(1<<ActiveFilter_Audio_astats)).toInt());
    ActiveAllTracks=activealltracks(Settings.value("ActiveAllTracks", 0).toInt());

    ui->Filters_Video_signalstats->setChecked(ActiveFilters[ActiveFilter_Video_signalstats]);
    ui->Filters_Video_cropdetect->setChecked(ActiveFilters[ActiveFilter_Video_cropdetect]);
    ui->Filters_Video_Psnr->setChecked(ActiveFilters[ActiveFilter_Video_Psnr]);
    ui->Filters_Video_Ssim->setChecked(ActiveFilters[ActiveFilter_Video_Ssim]);
    ui->Filters_Video_Idet->setChecked(ActiveFilters[ActiveFilter_Video_Idet]);
    ui->Filters_Audio_EbuR128->setChecked(ActiveFilters[ActiveFilter_Audio_EbuR128]);
    ui->Filters_Audio_aphasemeter->setChecked(ActiveFilters[ActiveFilter_Audio_aphasemeter]);
    ui->Filters_Audio_astats->setChecked(ActiveFilters[ActiveFilter_Audio_astats]);

    ui->Tracks_Video_First->setChecked(!ActiveAllTracks[Type_Video]);
    ui->Tracks_Video_All->setChecked(ActiveAllTracks[Type_Video]);
    ui->Tracks_Audio_First->setChecked(!ActiveAllTracks[Type_Audio]);
    ui->Tracks_Audio_All->setChecked(ActiveAllTracks[Type_Audio]);
}

//---------------------------------------------------------------------------
void Preferences::Save()
{
    QSettings Settings;
    Settings.setValue("ActiveFilters", (uint)ActiveFilters.to_ulong());
    Settings.setValue("ActiveAllTracks", (uint)ActiveAllTracks.to_ulong());
    Settings.sync();
}

//***************************************************************************
// Slots
//***************************************************************************

//---------------------------------------------------------------------------
void Preferences::OnAccepted()
{
    ActiveFilters.reset();
    if (ui->Filters_Video_signalstats->isChecked())
        ActiveFilters.set(ActiveFilter_Video_signalstats);
    if (ui->Filters_Video_cropdetect->isChecked())
        ActiveFilters.set(ActiveFilter_Video_cropdetect);
    if (ui->Filters_Video_Psnr->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Psnr);
    if (ui->Filters_Video_Ssim->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Ssim);
    if (ui->Filters_Video_Idet->isChecked())
        ActiveFilters.set(ActiveFilter_Video_Idet);
    if (ui->Filters_Audio_EbuR128->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_EbuR128);
    if (ui->Filters_Audio_aphasemeter->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_aphasemeter);
    if (ui->Filters_Audio_astats->isChecked())
        ActiveFilters.set(ActiveFilter_Audio_astats);

    ActiveAllTracks.reset();
    if (ui->Tracks_Video_All->isChecked())
        ActiveAllTracks.set(Type_Video);
    if (ui->Tracks_Audio_All->isChecked())
        ActiveAllTracks.set(Type_Audio);

    Save();
}

//---------------------------------------------------------------------------
void Preferences::OnRejected()
{
    Load();
}


/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Core_H
#define Core_H

#include <cstddef>
#include <bitset>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE

extern const char* Version;

#define NOT_AVAILABLE "N/A"
bool isNotAvailable(const char* value);

enum activefilter
{
    ActiveFilter_Video_signalstats,
    ActiveFilter_Video_cropdetect,
    ActiveFilter_Video_Psnr,
    ActiveFilter_Audio_EbuR128,
    ActiveFilter_Audio_aphasemeter,
    ActiveFilter_Audio_astats,
    ActiveFilter_Video_Ssim,
    ActiveFilter_Video_Idet,
    ActiveFilter_Video_Deflicker,
    ActiveFilter_Video_Entropy,
    ActiveFilter_Video_EntropyDiff,
    ActiveFilter_Video_blockdetect,
    ActiveFilter_Video_blurdetect,
    ActiveFilter_Max //Note: always add a new ActiveFilter element before ActiveFilter_Max, never before any other element, else preferences of people already having the tool will be shifted when preferences are read from the profile
};
typedef std::bitset<ActiveFilter_Max> activefilters;

struct per_group
{
    const   std::size_t Start; //Item
    const   std::size_t Count;
    const   char*       MinFormula;
    const   char*       MaxFormula;
    const   double      StepsCount;
    const   char*       Name;
    const   bool        CheckedByDefault;
    const   char*       Description;
    activefilter        ActiveFilterGroup;
};

struct per_item
{
    const   std::size_t Group1; //Group
    const   std::size_t Group2; //Group
    const   char*       Name;
    const   char*       FFmpeg_Name;
    const   int         DigitsAfterComma;
    const   bool        NewLine;
    const   double      DefaultLimit;
    const   double      DefaultLimit2;
    const   activefilter Filter;
    const   char*       color;
    const   int         thickness;
};

struct stream_info
{
    size_t                      CountOfGroups;
    size_t                      CountOfItems;
    const struct per_group*     PerGroup;
    const struct per_item*      PerItem;

    struct per_group*           GetPerGroup(int group) const {
        return const_cast<struct per_group*> (PerGroup) + group;
    }
};
enum Types
{
    Type_Video,
    Type_Audio,
    Type_Max
};

constexpr Types Type_Comments = Type_Max;
constexpr Types Type_Panels = (Types) (Type_Comments + 1);

typedef std::bitset<Type_Max> activealltracks;

extern const struct stream_info PerStreamType    [Type_Max];

#endif // Core_H

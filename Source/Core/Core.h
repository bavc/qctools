/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Core_H
#define Core_H

#include <cstddef>

#ifndef UNICODE
    #define UNICODE
#endif //UNICODE

extern const char* Version;

struct per_group
{
    const   std::size_t Start; //Item
    const   std::size_t Count;
    const   double      Min;
    const   double      Max;
    const   double      StepsCount;
    const   char*       Name;
    const   bool        CheckedByDefault;
    const   char*       Description;
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
};

struct stream_info
{
    size_t                      CountOfGroups;
    size_t                      CountOfItems;
    const struct per_group*     PerGroup;
    const struct per_item*      PerItem;
};
const size_t CountOfStreamTypes=2;
extern const struct stream_info PerStreamType    [CountOfStreamTypes];

#endif // Core_H

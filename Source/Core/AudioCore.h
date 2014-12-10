/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef AudioCore_H
#define AudioCore_H

#include <Core/Core.h>

const size_t AudioItem_Begin=21;

enum AudioItem
{
    //R128
    Item_R128M,
    //Item_R128S,
    //Item_R128I,
    //LRA (R128)
    //Item_LRAL,
    //Item_LRA,
    //Item_LRAH,
    //Internal
    Item_AudioMax
};

enum AudioGroup
{
    Group_R128,
    //Group_LRA,
    Group_AudioAxis,
    Group_AudioMax
};

extern const struct per_group  AudioPerGroup    [Group_AudioMax];
extern const struct per_item   AudioPerItem     [Item_AudioMax];

#endif // Core_H

/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/AudioCore.h>
#include <cfloat>

const struct per_group AudioPerGroup [Group_AudioMax]=
{
    //R128
    {
        Item_R128M,     1,    0,    0,  3,  "R.128",  true,
        "(TODO)\n",
    },
    //LRA
    //{
    //    Item_LRAL,       3,    0,    0,  3,  "LRA",  true,
    //    "(TODO)\n",
    //},
};

const struct per_item AudioPerItem [Item_AudioMax]=
{
    //Y
    { Group_R128,   Group_AudioMax,       "R128.M",         "lavfi.r128.M",             0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_R128,   Group_AudioMax,       "R128.S",         "lavfi.r128.S",             0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_R128,   Group_AudioMax,       "R128.I",         "lavfi.r128.I",             0,   true,   DBL_MAX, DBL_MAX },
    //U
    //{ Group_LRA,    Group_AudioMax,       "LRAL",           "lavfi.r128.LRA.low",       0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRA",            "lavfi.r128.LRA",           0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRAH",           "lavfi.r128.LRA.high",      0,   true,   DBL_MAX, DBL_MAX },
};

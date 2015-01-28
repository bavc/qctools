/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/Core.h>
#include <Core/VideoCore.h>
#include <Core/AudioCore.h>

//---------------------------------------------------------------------------
const char* Version="0.6.0";

//---------------------------------------------------------------------------
const struct stream_info PerStreamType    [CountOfStreamTypes] =
{
    { Group_VideoMax, Item_VideoMax, VideoPerGroup, VideoPerItem, },
    { Group_AudioMax, Item_AudioMax, AudioPerGroup, AudioPerItem, },
};

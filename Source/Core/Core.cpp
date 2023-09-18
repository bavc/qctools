/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/Core.h>
#include <Core/VideoCore.h>
#include <Core/AudioCore.h>
#include <string.h>

//---------------------------------------------------------------------------
const char* Version="1.3";

//---------------------------------------------------------------------------
const struct stream_info PerStreamType    [Type_Max] =
{
    { Group_VideoMax, Item_VideoMax, VideoPerGroup, VideoPerItem, },
    { Group_AudioMax, Item_AudioMax, AudioPerGroup, AudioPerItem, },
};

bool isNotAvailable(const char *value)
{
    return strcmp(value, NOT_AVAILABLE) == 0;
}

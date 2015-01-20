/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef BlackmagicDeckLink_Glue_H
#define BlackmagicDeckLink_Glue_H

class FFmpeg_Glue;

class BlackmagicDeckLink_Glue
{
public:
    BlackmagicDeckLink_Glue(FFmpeg_Glue* Glue, int TC_in, int TC_out);
    ~BlackmagicDeckLink_Glue();

    void                        Start();
    void                        Pause();
    void                        Stop();
    
    int                         Width_Get();
    int                         Height_Get();

    enum status
    {
        connecting,
        connected,
        seeking,
        capturing,
        captured,
        aborting,
    };
    status                      Status;

private:
    void*                       Handle;
};


//---------------------------------------------------------------------------
// Helpers

#include <cmath>

// convert a BCD timecode to a frame count (does not take into account drop frame timecodes...)
#define GET_FRAME_COUNT(result, tc_bcd, timeScale, frameDuration)    do{\
                        (result) = 0;\
                        (result) += (((uint32_t)((tc_bcd) >> 28) & 0x0F)*10 + ((uint32_t)((tc_bcd) >> 24) & 0x0F))*60;\
                        (result) += ((uint32_t)((tc_bcd) >> 20) & 0x0F)*10 + ((uint32_t)((tc_bcd) >> 16) & 0x0F);\
                        (result) *= 60;\
                        (result) += ((uint32_t)((tc_bcd) >> 12) & 0x0F)*10 + ((uint32_t)((tc_bcd) >> 8) & 0x0F);\
                        (result) *= std::ceil(double((timeScale))/(frameDuration));\
                        (result) += ((uint32_t)((tc_bcd) >> 4) & 0x0F)*10 + ((uint32_t)((tc_bcd) & 0x0F));\
                        } while(0)

#endif // BlackmagicDeckLink_Glue_H

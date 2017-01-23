/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef BlackmagicDeckLink_Glue_H
#define BlackmagicDeckLink_Glue_H

//---------------------------------------------------------------------------
#ifndef _WIN32
    #define __stdcall
#endif
//---------------------------------------------------------------------------

#include <string>
#include <vector>

class FFmpeg_Glue;
typedef void (__stdcall timecodeisavailable_callback)(void* Private);

class BlackmagicDeckLink_Glue
{
public:
    BlackmagicDeckLink_Glue(size_t CardPos);
    ~BlackmagicDeckLink_Glue();

    void                        Start();
    void                        Stop();
    
    int                         Width_Get();
    int                         Height_Get();

    void                        CurrentTimecode();

    FFmpeg_Glue*                Glue;
    enum status
    {
        instancied,
        capturing,
        aborting,
        finished,
    };
    struct config_in
    {
        int                     TC_in;
        int                     TC_out;
        int                     FrameCount;
        int                     FrameDuration;
        int                     VideoBitDepth;
        bool                    VideoCompression;
        int                     AudioBitDepth;
        int                     AudioTargetBitDepth;
        int                     ChannelsCount;
        int                     TimeScale;
        bool                    DropFrame;
        timecodeisavailable_callback* TimeCodeIsAvailable_Callback;
        void*                   TimeCodeIsAvailable_Private;
        int                     VideoInputConnection;

        config_in()
            : TC_in(-1)
            , TC_out(-1)
            , FrameCount(-1)
            , FrameDuration(0)
            , VideoBitDepth(8)
            , VideoCompression(false)
            , AudioBitDepth(16)
            , AudioTargetBitDepth(16)
            , ChannelsCount(2)
            , TimeScale(0)
            , DropFrame(true)
            , TimeCodeIsAvailable_Callback(NULL)
            , TimeCodeIsAvailable_Private(NULL)
            , VideoInputConnection(-1)
        {
        }
    };
    struct config_out
    {
        int                     VideoInputConnections;
        status                  Status;
        int                     TC_current;

        config_out()
            : VideoInputConnections(-1)
            , Status(instancied)
            , TC_current(-1)
        {
        }
    };
    config_in                   Config_In;
    config_out                  Config_Out;

    static std::vector<std::string> CardsList();

private:
    void*                       Handle;
};


//---------------------------------------------------------------------------
// Helpers

#include <cmath>

// convert a BCD timecode to a frame count (does not take into account drop frame timecodes...)
#define GET_FRAME_COUNT(result, tc_bcd, timeScale, frameDuration)    {\
                        (result) = 0;\
                        (result) += (((int)((tc_bcd) >> 28) & 0x0F)*10 + ((int)((tc_bcd) >> 24) & 0x0F))*60;\
                        (result) += ((int)((tc_bcd) >> 20) & 0x0F)*10 + ((int)((tc_bcd) >> 16) & 0x0F);\
                        (result) *= 60;\
                        (result) += ((int)((tc_bcd) >> 12) & 0x0F)*10 + ((int)((tc_bcd) >> 8) & 0x0F);\
                        (result) *= std::ceil(double((timeScale))/(frameDuration));\
                        (result) += ((int)((tc_bcd) >> 4) & 0x0F)*10 + ((int)((tc_bcd) & 0x0F));\
                        }

#endif // BlackmagicDeckLink_Glue_H

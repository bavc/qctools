/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/BlackmagicDeckLink_Glue.h"
#include "Core/BlackmagicDeckLink.h"
#if !defined (BLACKMAGICDECKLINK) && defined(_DEBUG)
    #include "Core/FFmpeg_Glue.h"
#endif
//---------------------------------------------------------------------------


#if !defined (BLACKMAGICDECKLINK) && defined(_DEBUG) // Simulation
struct Debug_Simulation
{
    FFmpeg_Glue* Glue;
    int FrameCount;
};
#endif

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BlackmagicDeckLink_Glue::BlackmagicDeckLink_Glue(FFmpeg_Glue* Glue, int TC_in, int TC_out)
    : Handle(NULL)
    , Status(connecting)
{
    #if defined(BLACKMAGICDECKLINK_YES)
        CaptureHelper* helper=new CaptureHelper(true);
        helper->Status=&Status;
        helper->Glue=Glue;
        helper->TC_in=TC_in;
        helper->TC_out=TC_out;

        Handle=helper;
    #elif defined(_DEBUG) // Simulation
        int FrameCount_In, FrameCount_Out;
        GET_FRAME_COUNT(FrameCount_In, TC_in, 30, 1);
        GET_FRAME_COUNT(FrameCount_Out, TC_out, 30, 1);
        
        Debug_Simulation* simulation=new Debug_Simulation;
        simulation->Glue=Glue;
        simulation->FrameCount=FrameCount_Out-FrameCount_In;

        Status=BlackmagicDeckLink_Glue::connected;

        Handle=simulation;
    #endif
}

//---------------------------------------------------------------------------
BlackmagicDeckLink_Glue::~BlackmagicDeckLink_Glue()
{
    if (Handle)
    {
        #if defined(BLACKMAGICDECKLINK_YES)
            delete (CaptureHelper*)Handle;
        #elif defined(_DEBUG) // Simulation
            delete (Debug_Simulation*)Handle;
        #endif
    }
}

//***************************************************************************
// Actions
//***************************************************************************

//---------------------------------------------------------------------------
void BlackmagicDeckLink_Glue::Start()
{
    if (Handle)
    {
        #if defined(BLACKMAGICDECKLINK_YES)
            ((CaptureHelper*)Handle)->startCapture();
        #elif defined(_DEBUG) // Simulation
            unsigned char FillingValue=0;
            unsigned char Data[720*486*2];

            for (int FramePos=0; FramePos<((Debug_Simulation*)Handle)->FrameCount; FramePos++)
            {
                memset(Data, FillingValue, 720*486*2);
                if (!((Debug_Simulation*)Handle)->Glue->OutputFrame(Data, 720*486*2, FramePos))
                    break;
                FillingValue++;
            }

            Status=BlackmagicDeckLink_Glue::captured;

            ((Debug_Simulation*)Handle)->Glue->CloseOutput();
        #endif
    }
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_Glue::Pause()
{
    if (Handle)
    {
        #if defined(BLACKMAGICDECKLINK_YES)
            ((CaptureHelper*)Handle)->pauseCapture();
        #endif
    }
}

//---------------------------------------------------------------------------
bool BlackmagicDeckLink_Glue::Stop()
{
    if (Handle)
    {
        #if defined(BLACKMAGICDECKLINK_YES)
            return ((CaptureHelper*)Handle)->stopCapture();
        #endif
    }

    return false;
}

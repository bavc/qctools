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
    FFmpeg_Glue** Glue;
    int FrameCount;
};
#endif

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
BlackmagicDeckLink_Glue::BlackmagicDeckLink_Glue(size_t CardPos)
    : Handle(NULL)
    , Glue(NULL)
{
    #if defined(BLACKMAGICDECKLINK_YES)
        CaptureHelper* helper=new CaptureHelper(CardPos, &Config_In, &Config_Out);
        helper->Glue=&Glue;

        Handle=helper;
    #elif defined(_DEBUG) // Simulation
        Debug_Simulation* simulation=new Debug_Simulation;
        simulation->Glue=&Glue;

        Config_Out.VideoInputConnections=0x3D;

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

            int FrameCount_In, FrameCount_Out;
            GET_FRAME_COUNT(FrameCount_In, Config_In.TC_in, 30, 1);
            GET_FRAME_COUNT(FrameCount_Out, Config_In.TC_out, 30, 1);
        
            Debug_Simulation* simulation=new Debug_Simulation;
            simulation->Glue=&Glue;
            int FrameCount=FrameCount_Out-FrameCount_In;

            for (int FramePos=0; FramePos<FrameCount; FramePos++)
            {
                memset(Data, FillingValue, 720*486*2);
                if (!(*((Debug_Simulation*)Handle)->Glue)->OutputFrame(Data, 720*486*2, FramePos))
                    break;
                FillingValue++;
            }

            Config_Out.Status=BlackmagicDeckLink_Glue::finished;

            (*((Debug_Simulation*)Handle)->Glue)->CloseOutput();
        #endif
    }
}

//---------------------------------------------------------------------------
bool BlackmagicDeckLink_Glue::Stop()
{
    if (Handle)
    {
        #if defined(BLACKMAGICDECKLINK_YES)
            return ((CaptureHelper*)Handle)->finishCapture();
        #endif
    }

    return false;
}

//---------------------------------------------------------------------------
std::vector<std::string> BlackmagicDeckLink_Glue::CardsList()
{
    #if defined(BLACKMAGICDECKLINK_YES)
        return DeckLinkCardsList();
    #else
        std::vector<std::string> ToReturn;
        ToReturn.push_back("Fake Card 1");
        ToReturn.push_back("Fake Card 2");
        return ToReturn;
    #endif
}

//---------------------------------------------------------------------------
void BlackmagicDeckLink_Glue::CurrentTimecode()
{
    #if defined(BLACKMAGICDECKLINK_YES)
        ((CaptureHelper*)Handle)->getTimeCode();
    #endif
}


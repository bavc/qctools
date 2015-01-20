/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef BlackmagicDeckLink_H
#define BlackmagicDeckLink_H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
#if defined(__APPLE__) && defined(__MACH__)
    #define BLACKMAGICDECKLINK_GLUE_MAC
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(BLACKMAGICDECKLINK_GLUE_MAC)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/BlackmagicDeckLink_Glue.h"
#include "Core/FFmpeg_Glue.h"
#include "Mac/include/DeckLinkAPI.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class CaptureHelper : public IDeckLinkDeckControlStatusCallback , public IDeckLinkInputCallback
{
private:
    IDeckLink*                  m_deckLink;
    IDeckLinkInput*             m_deckLinkInput;
    IDeckLinkDeckControl*       m_deckControl;

    // video mode
    long                        m_width;
    long                        m_height;
    BMDTimeScale                m_timeScale;
    BMDTimeValue                m_frameDuration;
    bool                        m_dropframe;
    
    bool                        setupDeck();
    bool                        setupDeckLinkInput();
    bool                        setupDeckControl();
    
    void                        cleanupDeckControl();
    void                        cleanupDeckLinkInput();
    void                        cleanupDeck();
    
public:
                                CaptureHelper(bool dropframe);
    virtual                    ~CaptureHelper();
    
    // start the capture operation. returns when the operation has completed
    void                        startCapture();
    void                        pauseCapture();
    void                        stopCapture();
    
    // IDeckLinkDeckControlStatusCallback
    virtual HRESULT             TimecodeUpdate (BMDTimecodeBCD currentTimecode);
    virtual HRESULT             VTRControlStateChanged (BMDDeckControlVTRControlState newState, BMDDeckControlError error);
    virtual HRESULT             DeckControlEventReceived (BMDDeckControlEvent event, BMDDeckControlError error);
    virtual HRESULT             DeckControlStatusChanged (BMDDeckControlStatusFlags flags, uint32_t mask);
    
    // IDeckLinkInputCallback
    virtual HRESULT             VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) {return S_OK;};
    virtual HRESULT             VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedFrame, IDeckLinkAudioInputPacket*);
    
    // IUnknown
    HRESULT                     QueryInterface (REFIID iid, LPVOID *ppv)        {return E_NOINTERFACE;}
    ULONG                       AddRef ()                                       {return 1;}
    ULONG                       Release ()                                      {return 1;}

    BlackmagicDeckLink_Glue::status* Status;
    FFmpeg_Glue*                Glue;
    int                         m_FramePos;
    int                         TC_in;
    int                         TC_current;
    int                         TC_out;
};

#endif // BLACKMAGICDECKLINK_GLUE_MAC

#endif // BlackmagicDeckLink_H

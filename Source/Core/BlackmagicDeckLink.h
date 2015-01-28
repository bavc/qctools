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
#include "Core/BlackmagicDeckLink_Glue.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(BLACKMAGICDECKLINK_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "Core/FFmpeg_Glue.h"
#if defined(_WIN32) || defined(_WIN64)
    #include "DeckLinkAPI_h.h"
    typedef unsigned long bmdl_uint32_t;
#elif defined(__APPLE__) && defined(__MACH__)
    #include "Mac/include/DeckLinkAPI.h"
    typedef uint32_t bmdl_uint32_t;
#else
    #include "Linux/include/DeckLinkAPI.h"
    typedef uint32_t bmdl_uint32_t;
#endif
#include <string>
#include <vector>
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
    
    bool                        setupDeck(size_t CardPos);
    bool                        setupDeckLinkInput();
    bool                        setupDeckControl();
    
    bool                        cleanupDeckControl();
    bool                        cleanupDeckLinkInput();
    bool                        cleanupDeck();
    
public:
                                CaptureHelper(size_t CardPos, bool dropframe);
    virtual                    ~CaptureHelper();
    
    void                        startCapture();
    void                        pauseCapture();
    bool                        stopCapture(bool force=false);

    int                         getTimeCode();
    
    // IDeckLinkDeckControlStatusCallback
    virtual HRESULT STDMETHODCALLTYPE TimecodeUpdate (BMDTimecodeBCD currentTimecode);
    virtual HRESULT STDMETHODCALLTYPE VTRControlStateChanged (BMDDeckControlVTRControlState newState, BMDDeckControlError error);
    virtual HRESULT STDMETHODCALLTYPE DeckControlEventReceived (BMDDeckControlEvent event, BMDDeckControlError error);
    virtual HRESULT STDMETHODCALLTYPE DeckControlStatusChanged (BMDDeckControlStatusFlags flags, bmdl_uint32_t mask);
    
    // IDeckLinkInputCallback
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) {return S_OK;};
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedFrame, IDeckLinkAudioInputPacket*);
    
    // IUnknown
    HRESULT STDMETHODCALLTYPE   QueryInterface (REFIID iid, LPVOID *ppv)        {return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE     AddRef ()                                       {return 1;}
    ULONG STDMETHODCALLTYPE     Release ()                                      {return 1;}

    BlackmagicDeckLink_Glue::status* Status;
    FFmpeg_Glue**               Glue;
    int                         m_FramePos;
    int*                        TC_in;
    int                         TC_current;
    int*                        TC_out;
};

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
std::vector<std::string> DeckLinkCardsList();

#endif // BLACKMAGICDECKLINK_YES

#endif // BlackmagicDeckLink_H

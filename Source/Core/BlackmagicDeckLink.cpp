/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/BlackmagicDeckLink.h"
//---------------------------------------------------------------------------


//***************************************************************************
// Deck Mac
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(BLACKMAGICDECKLINK_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
const char* BMDDeckControlError2String(BMDDeckControlError bmdDeckControlError)
{
    switch (bmdDeckControlError)
    {
        case bmdDeckControlNoError                  : return "";
        case bmdDeckControlModeError                : return "Incorrect mode";
        case bmdDeckControlMissedInPointError       : return "Missed InPoint";
        case bmdDeckControlDeckTimeoutError         : return "Deck timeout";
        case bmdDeckControlCommandFailedError       : return "Command failed";
        case bmdDeckControlDeviceAlreadyOpenedError : return "Device already opened";
        case bmdDeckControlFailedToOpenDeviceError  : return "Failed to open device";
        case bmdDeckControlInLocalModeError         : return "In local mode";
        case bmdDeckControlEndOfTapeError           : return "End of tape";
        case bmdDeckControlUserAbortError           : return "User abort";
        case bmdDeckControlNoTapeInDeckError        : return "No tape";
        case bmdDeckControlNoVideoFromCardError     : return "No video from card";
        case bmdDeckControlNoCommunicationError     : return "No communication";
        case bmdDeckControlUnknownError             : return "Unknown";
        default                                     : return "Reserved";
    }
}

//---------------------------------------------------------------------------
string BMDDeckControlStatusFlags2String(BMDDeckControlStatusFlags bmdDeckControlStatusFlags)
{
    string ToReturn;
    ToReturn+=(bmdDeckControlStatusFlags & bmdDeckControlStatusDeckConnected)   ? "Deck connected, "    : "Deck disconnected, ";
    ToReturn+=(bmdDeckControlStatusFlags & bmdDeckControlStatusRemoteMode)      ? "Remote mode, "       : "Local mode, ";
    ToReturn+=(bmdDeckControlStatusFlags & bmdDeckControlStatusRecordInhibited) ? "Record inhibited, "  : "Record allowed, ";
    ToReturn+=(bmdDeckControlStatusFlags & bmdDeckControlStatusCassetteOut)     ? "Cassette out"        : "Cassette in";
    return ToReturn;
}

//---------------------------------------------------------------------------
const char* BMDDeckControlEvent2String(BMDDeckControlEvent bmdDeckControlEvent)
{
    switch (bmdDeckControlEvent)
    {
        case bmdDeckControlAbortedEvent             : return "Abort";
        case bmdDeckControlPrepareForExportEvent    : return "Prepare for export";
        case bmdDeckControlPrepareForCaptureEvent   : return "Prepare for capture";
        case bmdDeckControlExportCompleteEvent      : return "Export complete";
        case bmdDeckControlCaptureCompleteEvent     : return "Capture complete";
        default                                     : return "Reserved";
    }
}

//---------------------------------------------------------------------------
const char* BMDDeckControlVTRControlState2String(BMDDeckControlVTRControlState bmdDeckControlVTRControlState)
{
    switch (bmdDeckControlVTRControlState)
    {
        case bmdDeckControlNotInVTRControlMode      : return "Not in VTR mode";
        case bmdDeckControlVTRControlPlaying        : return "Play";
        case bmdDeckControlVTRControlRecording      : return "Record";
        case bmdDeckControlVTRControlStill          : return "Still";
        case bmdDeckControlVTRControlShuttleForward : return "Shuttle forward";
        case bmdDeckControlVTRControlShuttleReverse : return "Shuttle reverse";
        case bmdDeckControlVTRControlJogForward     : return "Jog forward";
        case bmdDeckControlVTRControlJogReverse     : return "Jog reverse";
        case bmdDeckControlVTRControlStopped        : return "Stop";
        default                                     : return "Reserved";
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
IDeckLinkIterator* getDeckLinkIterator()
{
    #if defined(_WIN32) || defined(_WIN64)
        IDeckLinkIterator* deckLinkIter = NULL;
        CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&deckLinkIter);
        return deckLinkIter;
    #else
        return CreateDeckLinkIteratorInstance();
    #endif
}

//---------------------------------------------------------------------------
IDeckLink *getDeckLinkCard(size_t Pos=0)
{
    IDeckLinkIterator* deckLinkIter = getDeckLinkIterator();
    if (!deckLinkIter)
    {
        cout << "Could not enumerate DeckLink cards" << endl;
        return NULL;
    }
    
    // get the first decklink card
    IDeckLink* deckLink=NULL;
    for (;;)
    {
        HRESULT Result=deckLinkIter->Next(&deckLink);
        if (Result == E_FAIL)
        {
            cout << "Could not detect a DeckLink card" << endl;
            break;
        }
        if (Result == S_FALSE)
            break; // Finished

        if (!Pos)
            break;
        Pos--;
    }
        
    deckLinkIter->Release();
    return deckLink;
}

//---------------------------------------------------------------------------
std::vector<std::string> DeckLinkCardsList()
{
    std::vector<std::string> List;

    IDeckLinkIterator* deckLinkIter = getDeckLinkIterator();
    if (!deckLinkIter)
        return List; // No card
    
    // get the first decklink card
    IDeckLink* deckLink=NULL;
    for (;;)
    {
        HRESULT Result=deckLinkIter->Next(&deckLink);
        if (Result == E_FAIL)
        {
            cout << "Could not detect a DeckLink card" << endl;
            break;
        }
        if (Result == S_FALSE)
            break; // Finished

        List.push_back("DeckLink");
    }
        
    deckLinkIter->Release();
    return List;
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
CaptureHelper::CaptureHelper(size_t CardPos, bool dropframe)
    : m_deckLink(NULL)
    , m_deckLinkInput(NULL)
    , m_deckControl(NULL)
    , m_width(-1)
    , m_height(-1)
    , m_timeScale(0)
    , m_frameDuration(0)
    , m_dropframe(dropframe)
    , m_FramePos(0)
    , Glue(NULL)
    , TC_in(NULL)
    , TC_current(-1)
    , TC_out(NULL)
{
    cout << endl;
    cout << "***************************" << endl;
    cout << "*** Blackmagic DeckLink ***" << endl;
    cout << "***************************" << endl;
    cout << endl;

    // Setup DeckLink Input interface
    if (!setupDeck(CardPos))
        return;

    // Setup DeckLink Input interface
    if (!setupDeckLinkInput())
    {
        cleanupDeck();
        return;
    }
    
    // Setup DeckControl interface
    if (!setupDeckControl())
    {
        cleanupDeckLinkInput();
        cleanupDeck();
        return;
    }
}

//---------------------------------------------------------------------------
CaptureHelper::~CaptureHelper()
{
    stopCapture(true);
}

//---------------------------------------------------------------------------
bool CaptureHelper::setupDeck(size_t CardPos)
{
    cout << "*** Setup of Deck ***" << endl;

    // Find the card
    m_deckLink = getDeckLinkCard(CardPos);
    if (!m_deckLink)
        return false;

    // Increment reference count of the object
    m_deckLink->AddRef();

    // Get interfaces
    if (m_deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&m_deckLinkInput) != S_OK)
    {
        cout << "Could not obtain the DeckLink Input interface" << endl;
        return false;
    }
    if (m_deckLink->QueryInterface(IID_IDeckLinkDeckControl, (void **)&m_deckControl) != S_OK)
    {
        cout << "Could not obtain the DeckControl interface" << endl;
        return false;
    }

    cout << "OK" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::setupDeckLinkInput()
{
    cout << "*** Setup of DeckLinkInput ***" << endl;
    
    m_width = -1;
    
    // get frame scale and duration for the video mode
    IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
    if (m_deckLinkInput->GetDisplayModeIterator(&displayModeIterator) != S_OK)
    {
        cout << "Setup of DeckLinkInput error: problem with GetDisplayModeIterator" << endl;
        return false;
    }

    IDeckLinkDisplayMode* deckLinkDisplayMode = NULL;
    while (displayModeIterator->Next(&deckLinkDisplayMode) == S_OK)
    {
        if (deckLinkDisplayMode->GetDisplayMode() == bmdModeNTSC)
        {
            m_width = deckLinkDisplayMode->GetWidth();
            m_height = deckLinkDisplayMode->GetHeight();
            deckLinkDisplayMode->GetFrameRate(&m_frameDuration, &m_timeScale);
            deckLinkDisplayMode->Release();
            
            break;
        }
        
        deckLinkDisplayMode->Release();
    }
    displayModeIterator->Release();
    
    if (m_width == -1)
    {
        cout << "Setup of DeckLinkInput error: unable to find requested video mode" << endl;
        return false;
    }
    
    // set callback
    m_deckLinkInput->SetCallback(this);
    
    // enable video input
    if (m_deckLinkInput->EnableVideoInput(bmdModeNTSC, bmdFormat8BitYUV, bmdVideoInputFlagDefault) != S_OK)
    {
        cout << "Setup of DeckLinkInput error: could not enable video input" << endl;
        return false;
    }
    
    // start streaming
    if (m_deckLinkInput->StartStreams() != S_OK)
    {
        cout << "Setup of DeckLinkInput error: could not start streams" << endl;
        return false;
    }

    cout << "OK" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::setupDeckControl()
{
    cout << "*** Setup of DeckControl ***" << endl;

    // set callback, preroll and offset
    m_deckControl->SetCallback(this);
    
    // open connection to deck
    BMDDeckControlError bmdDeckControlError;
    if (m_deckControl->Open(m_timeScale, m_frameDuration, m_dropframe, &bmdDeckControlError) != S_OK)
    {
        cout << "Setup of DeckControl error: could not open (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;
        return false;
    }

    cout << "Waiting for deck anwser" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupDeckControl()
{
    if (!m_deckControl)
        return true;

    cout << "*** Cleanup of DeckControl ***" << endl;

    // Stop
    switch (*Status)
    {
        case BlackmagicDeckLink_Glue::seeking :
        case BlackmagicDeckLink_Glue::capturing :
                                                *Status=BlackmagicDeckLink_Glue::aborting;
                                                if (m_deckControl->Abort() != S_OK)
                                                    cout << "Could not abort capture" << endl;
                                                else
                                                    cout << "Aborting capture" << endl;
                                                return false;
    }

    // Close
    m_deckControl->Close(false);
    m_deckControl->SetCallback(NULL);

    m_deckControl->Release();
    m_deckControl = NULL;

    cout << "OK" << endl;

    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupDeckLinkInput()
{
    if (!m_deckLinkInput)
        return true;

    cout << "*** Cleanup of DeckLinkInput ***" << endl;

    m_deckLinkInput->StopStreams();
    m_deckLinkInput->DisableVideoInput();
    m_deckLinkInput->SetCallback(NULL);
    m_deckLinkInput->Release();
    m_deckLinkInput = NULL;

    cout << "OK" << endl;

    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupDeck()
{
    if (!m_deckLink)
        return true;

    cout << "*** Cleanup of Deck ***" << endl;
    
    // Decrement reference count of the object
    m_deckLink->Release();
    m_deckLink=NULL;

    cout << "OK" << endl;

    return true;
}

//---------------------------------------------------------------------------
void CaptureHelper::startCapture()
{
    cout << "*** Start capture ***" << endl;

    cout.setf (ios::hex, ios::basefield);
    cout.fill ('0');
    cout << "Starting capure from " << setw(2) << (((*TC_in)>>24)&0xFF) << ":" << setw(2) << (((*TC_in)>>16)&0xFF) << ":" << setw(2) << (((*TC_in)>>8)&0xFF) << ":" << setw(2) << (((*TC_in))&0xFF)
         << " to " << setw(2) << (((*TC_out)>>24)&0xFF) << ":" << setw(2) << (((*TC_out)>>16)&0xFF) << ":" << setw(2) << (((*TC_out)>>8)&0xFF) << ":" << setw(2) << (((*TC_out))&0xFF) << endl ;

    // Start capture
    *Status=BlackmagicDeckLink_Glue::seeking;
    BMDDeckControlError bmdDeckControlError;
    if (m_deckControl->StartCapture(true, (*TC_in), (*TC_out), &bmdDeckControlError) != S_OK)
        cout << "Could not start capture (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;

    cout << "Waiting for deck answer" << endl ;
}

//---------------------------------------------------------------------------
void CaptureHelper::pauseCapture()
{
    if (!m_deckControl)
        return;

    cout << "*** Pause capture ***" << endl;

    // Stop
    BMDDeckControlError bmdDeckControlError;
    if (m_deckControl->Stop(&bmdDeckControlError) != S_OK)
        cout << "Could not stop (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;

    cout << "OK" << endl;
}

//---------------------------------------------------------------------------
bool CaptureHelper::stopCapture(bool force)
{
    if (!cleanupDeckControl() && !force)
        return false;
    if (!cleanupDeckLinkInput() && !force)
        return false;
    if (!cleanupDeck() && !force)
        return false;

    if (Glue && *Glue)
        (*Glue)->CloseOutput();
    return true;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::TimecodeUpdate (BMDTimecodeBCD currentTimecode)
{
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::DeckControlEventReceived (BMDDeckControlEvent bmdDeckControlEvent, BMDDeckControlError bmdDeckControlError)
{
    cout <<"Deck control event: " << BMDDeckControlEvent2String(bmdDeckControlEvent);
    if (bmdDeckControlError != bmdDeckControlNoError)
        cout << " (error: " << BMDDeckControlError2String(bmdDeckControlError) << ")";
    cout << endl;
    
    switch (bmdDeckControlEvent)
    {
        case bmdDeckControlPrepareForCaptureEvent:
                                                    *Status=BlackmagicDeckLink_Glue::capturing;
                                                    cout << "OK" << endl;
                                                    break;
        case bmdDeckControlCaptureCompleteEvent:
                                                    *Status=BlackmagicDeckLink_Glue::captured;
                                                    break;
        default:
                                                    *Status=BlackmagicDeckLink_Glue::aborted;
                                                    break;
    }
    
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::VTRControlStateChanged (BMDDeckControlVTRControlState newState, BMDDeckControlError error)
{
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::DeckControlStatusChanged (BMDDeckControlStatusFlags bmdDeckControlStatusFlags, bmdl_uint32_t mask)
{
    cout <<"Deck control status change: " << BMDDeckControlStatusFlags2String(bmdDeckControlStatusFlags) << endl;
    
    if ((*Status==BlackmagicDeckLink_Glue::connecting)
     && (mask & bmdDeckControlStatusDeckConnected)
     && (bmdDeckControlStatusFlags & bmdDeckControlStatusDeckConnected))
    {
        cout << "OK" << endl;
        *Status=BlackmagicDeckLink_Glue::connected;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedFrame, IDeckLinkAudioInputPacket*)
{
    // check the serial timecode only when we were told the capture is about to start (bmdDeckControlPrepareForCaptureEvent)
    if (*Status!=BlackmagicDeckLink_Glue::capturing)
        return S_OK;

    IDeckLinkTimecode *timecode = NULL;
    if (arrivedFrame->GetTimecode(bmdTimecodeSerial, &timecode) != S_OK)
        return S_OK;

    // Handle the frame if time code is in [TC_in, TC_out[
    BMDTimecodeBCD tcBCD = timecode->GetBCD();
    if ( (TC_current == -1 || tcBCD != TC_current) //Ignore frames with same time code (TODO: check if it is relevant)
      && tcBCD >= *TC_in 
      && tcBCD < *TC_out )
    {
        // this frame is within the in-and out-points, do something useful with it
        uint8_t hours, minutes, seconds, frames;
        timecode->GetComponents(&hours, &minutes, &seconds, &frames);        
        cout.setf(ios::dec, ios::basefield);
        //cout << "New frame (timecode is " << setw(2) << (int)hours << ":" << setw(2) << (int)minutes << ":" <<  setw(2) << (int)seconds << ":" << setw(2) << (int)frames << ")" << endl;

        void *buffer;
        arrivedFrame->GetBytes(&buffer);
        if (Glue && *Glue)
            (*Glue)->OutputFrame((unsigned char*)buffer, 720*486*2, m_FramePos);

        TC_current = tcBCD;
        m_FramePos++;
    }
        
    timecode->Release();
    
    return S_OK;
}

#endif // defined(BLACKMAGICDECKLINK_YES)


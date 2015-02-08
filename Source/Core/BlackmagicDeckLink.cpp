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
#include <bitset>
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
// There are some differences between platforms
//***************************************************************************

#if defined(__APPLE__) && defined(__MACH__)
    typedef bool BOOL;
    typedef int64_t LONGLONG;
#elif !(defined(_WIN32) || defined(_WIN64))
    typedef bool BOOL;
    typedef int64_t LONGLONG;
#endif

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

        #if defined(__APPLE__) && defined(__MACH__)
        CFStringRef deviceNameCFString = NULL;
        if (deckLink->GetModelName(&deviceNameCFString) == S_OK)
        {
            char            deviceName[64];
            CFStringGetCString(deviceNameCFString, deviceName, sizeof(deviceName), kCFStringEncodingMacRoman);
            List.push_back(deviceName);
        }
        else
        #endif
            List.push_back("DeckLink");
    }
       
    deckLinkIter->Release();
    return List;
}

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
CaptureHelper::CaptureHelper(size_t CardPos, BlackmagicDeckLink_Glue::config_in* Config_In_, BlackmagicDeckLink_Glue::config_out* Config_Out_)
    : m_card(NULL)
    , m_input(NULL)
    , m_control(NULL)
    , m_width(-1)
    , m_height(-1)
    , m_timeScale(0)
    , m_frameDuration(0)
    , m_FramePos(0)
    , Glue(NULL)
    , Config_In(Config_In_)
    , Config_Out(Config_Out_)
    , WantTimeCode(false)
{
    cout << endl;
    cout << "***********************************" << endl;
    cout << "*** Blackmagic DeckLink Card #" << CardPos << " ***" << endl;
    cout << "***********************************" << endl;
    cout << endl;

    // Setup DeckLink Input interface
    if (!setupCard(CardPos))
        return;
}

//---------------------------------------------------------------------------
CaptureHelper::~CaptureHelper()
{
    stopCapture(true);
}

//***************************************************************************
// Deck
//***************************************************************************

//---------------------------------------------------------------------------
bool CaptureHelper::setupCard(size_t CardPos)
{
    if (m_card)
        return true;

    cout << "*** Setup of Card ***" << endl;

    // Find the card
    m_card = getDeckLinkCard(CardPos);
    if (!m_card)
    {
        cout << "Error: Could not obtain the Card interface" << endl;
        return false;
    }

    // Increment reference count of the object
    m_card->AddRef();

    cout << "OK" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupCard()
{
    if (!m_card)
        return true;

    cout << "*** Cleanup of Card ***" << endl;
    
    // Decrement reference count of the object
    m_card->Release();
    m_card=NULL;

    cout << "OK" << endl;

    return true;
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
bool CaptureHelper::setupInput()
{
    if (m_input)
        return true;

    cout << "*** Setup of Input ***" << endl;

    m_width = -1;
    
    // Get interface
    if (m_card->QueryInterface(IID_IDeckLinkInput, (void **)&m_input) != S_OK)
    {
        cout << "Error: Could not obtain the Input interface" << endl;
        return false;
    }

    // get frame scale and duration for the video mode
    IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
    if (m_input->GetDisplayModeIterator(&displayModeIterator) != S_OK)
    {
        cout << "Error: problem with GetDisplayModeIterator" << endl;
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
        cout << "Error: unable to find requested video mode" << endl;
        return false;
    }
    
    // set callback
    m_input->SetCallback(this);
    
    // enable video input
    if (m_input->EnableVideoInput(bmdModeNTSC, bmdFormat8BitYUV, bmdVideoInputFlagDefault) != S_OK)
    {
        cout << "Error: could not enable video input" << endl;
        return false;
    }
    
    // start streaming
    if (m_input->StartStreams() != S_OK)
    {
        cout << "Error: could not start streams" << endl;
        return false;
    }

    cout << "OK" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupInput()
{
    if (!m_input)
        return true;

    cout << "*** Cleanup of Input ***" << endl;

    m_input->StopStreams();
    m_input->DisableVideoInput();
    m_input->SetCallback(NULL);
    m_input->Release();
    m_input = NULL;

    cout << "OK" << endl;

    return true;
}

//***************************************************************************
// Control
//***************************************************************************

//---------------------------------------------------------------------------
bool CaptureHelper::setupControl()
{
    // We need time scale and time duration
    if (!setupInput())
        return false;
    
    if (m_control)
        return true;

    cout << "*** Setup of Control ***" << endl;

    // Get interface
    if (m_card->QueryInterface(IID_IDeckLinkDeckControl, (void **)&m_control) != S_OK)
    {
        cout << "Error: Could not obtain the Control interface" << endl;
        return false;
    }

    // set callback
    m_control->SetCallback(this);
    
    // open connection to deck
    BMDDeckControlError bmdDeckControlError;
    if (m_control->Open(m_timeScale, m_frameDuration, Config_In->DropFrame, &bmdDeckControlError) != S_OK)
    {
        cout << "Error: could not open (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;
        return false;
    }

    cout << "Waiting for deck anwser" << endl;
    return true;
}

//---------------------------------------------------------------------------
bool CaptureHelper::cleanupControl()
{
    if (!m_control)
        return true;

    cout << "*** Cleanup of DeckControl ***" << endl;

    // Stop
    switch (Config_Out->Status)
    {
        case BlackmagicDeckLink_Glue::seeking :
        case BlackmagicDeckLink_Glue::capturing :
                                                Config_Out->Status=BlackmagicDeckLink_Glue::aborting;
                                                if (m_control->Abort() != S_OK)
                                                    cout << "Could not abort capture" << endl;
                                                else
                                                    cout << "Aborting capture" << endl;
                                                return false;
    }

    // Close
    m_control->Close(false);
    m_control->SetCallback(NULL);
    m_control->Release();
    m_control = NULL;

    cout << "OK" << endl;

    return true;
}

//---------------------------------------------------------------------------
void CaptureHelper::getTimeCode()
{
    if (!setupControl())
        return; 

    readTimeCode();
}

//---------------------------------------------------------------------------
void CaptureHelper::readTimeCode()
{
    cout << "*** Timecode ***" << endl;

    BMDDeckControlError bmdDeckControlError;
    IDeckLinkTimecode *currentTimecode=NULL;
    if (m_control->GetTimecode(&currentTimecode, &bmdDeckControlError) != S_OK)
    {
        if (bmdDeckControlError==bmdDeckControlNoCommunicationError)
        {
            cout << "Waiting for deck answer" << endl;
            WantTimeCode=true;
        }
        else
            cout << "Error: " << BMDDeckControlError2String(bmdDeckControlError) << endl;
        Config_Out->TC_current=-1;
    }
    else
    {
        Config_Out->TC_current=currentTimecode->GetBCD();
        currentTimecode->Release();
        cout << "OK " << hex << Config_Out->TC_current << endl;

        if (Config_In->TimeCodeIsAvailable_Callback)
            Config_In->TimeCodeIsAvailable_Callback(Config_In->TimeCodeIsAvailable_Private);
    }
}

//---------------------------------------------------------------------------
void CaptureHelper::startCapture()
{
    if (!setupInput())
        return;
    if (!setupControl())
        return;

    cout << "*** Start capture ***" << endl;

    BMDDeckControlError bmdDeckControlError;
    if (Config_In->TC_in==-1)
    {
        if (m_control->Play(&bmdDeckControlError) != S_OK)
            cout << "Could not start capture (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;
        Config_Out->Status=BlackmagicDeckLink_Glue::capturing;
    }
    else
    {
        cout.setf (ios::hex, ios::basefield);
        cout.fill ('0');
        cout << "Starting capure from " << setw(2) << (((Config_In->TC_in)>>24)&0xFF) << ":" << setw(2) << (((Config_In->TC_in)>>16)&0xFF) << ":" << setw(2) << (((Config_In->TC_in)>>8)&0xFF) << ":" << setw(2) << (((Config_In->TC_in))&0xFF)
             << " to " << setw(2) << (((Config_In->TC_out)>>24)&0xFF) << ":" << setw(2) << (((Config_In->TC_out)>>16)&0xFF) << ":" << setw(2) << (((Config_In->TC_out)>>8)&0xFF) << ":" << setw(2) << (((Config_In->TC_out))&0xFF) << endl ;

        // Start capture
        Config_Out->Status=BlackmagicDeckLink_Glue::seeking;
        if (m_control->StartCapture(true, (Config_In->TC_in), (Config_In->TC_out), &bmdDeckControlError) != S_OK)
        {
            cout << "Could not start capture (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;
            return;
        }

        cout << "Waiting for deck answer" << endl ;
    }
}

//---------------------------------------------------------------------------
void CaptureHelper::pauseCapture()
{
    if (!m_control)
        return;

    cout << "*** Pause capture ***" << endl;

    // Stop
    BMDDeckControlError bmdDeckControlError;
    if (m_control->Stop(&bmdDeckControlError) != S_OK)
        cout << "Could not stop (" << BMDDeckControlError2String(bmdDeckControlError) << ")" << endl;

    cout << "OK" << endl;
}

//---------------------------------------------------------------------------
bool CaptureHelper::stopCapture(bool force)
{
    if (!cleanupControl() && !force)
        return false;
    if (!cleanupInput() && !force)
        return false;
    if (!cleanupCard() && !force)
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
    cout <<"*** Deck control event ***" << endl;
    cout << BMDDeckControlEvent2String(bmdDeckControlEvent);
    if (bmdDeckControlError != bmdDeckControlNoError)
        cout << " (error: " << BMDDeckControlError2String(bmdDeckControlError) << ")";
    cout << endl;
    
    switch (bmdDeckControlEvent)
    {
        case bmdDeckControlPrepareForCaptureEvent:
                                                    Config_Out->Status=BlackmagicDeckLink_Glue::capturing;
                                                    cout << "Capturing" << endl;
                                                    break;
        case bmdDeckControlCaptureCompleteEvent:
                                                    Config_Out->Status=BlackmagicDeckLink_Glue::captured;
                                                    break;
        default:
                                                    Config_Out->Status=BlackmagicDeckLink_Glue::aborted;
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
    cout <<"*** Deck control status change ***" << endl;
    cout << BMDDeckControlStatusFlags2String(bmdDeckControlStatusFlags) << endl;
    
    if ((Config_Out->Status==BlackmagicDeckLink_Glue::connecting)
     && (mask & bmdDeckControlStatusDeckConnected)
     && (bmdDeckControlStatusFlags & bmdDeckControlStatusDeckConnected))
    {
        cout << "Connected" << endl;
        Config_Out->Status=BlackmagicDeckLink_Glue::connected;

        if (WantTimeCode)
        {
            readTimeCode();
            WantTimeCode=false;
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT CaptureHelper::VideoInputFrameArrived (IDeckLinkVideoInputFrame* arrivedVideoFrame, IDeckLinkAudioInputPacket* arrivedAudioFrame)
{
    // check the serial timecode only when we were told the capture is about to start (bmdDeckControlPrepareForCaptureEvent)
    if (Config_Out->Status!=BlackmagicDeckLink_Glue::capturing)
        return S_OK;

    IDeckLinkTimecode *timecode = NULL;
    arrivedVideoFrame->GetTimecode(bmdTimecodeSerial, &timecode);

    BMDTimecodeBCD tcBCD;
    bool ShouldDecode=true;
    if ( timecode )
    {
        tcBCD= timecode->GetBCD();

        // Handle the frame if time code is in [TC_in, TC_out[
        if ((Config_Out->TC_current != -1 && tcBCD == Config_Out->TC_current) //Ignore frames with same time code (TODO: check if it is relevant)
          || tcBCD < Config_In->TC_in 
          || tcBCD >= Config_In->TC_out )
          ShouldDecode=false;

        // this frame is within the in-and out-points, do something useful with it
        uint8_t hours, minutes, seconds, frames;
        timecode->GetComponents(&hours, &minutes, &seconds, &frames);        
        cout.setf(ios::dec, ios::basefield);
        cout << "New frame (timecode is " << setw(2) << (int)hours << ":" << setw(2) << (int)minutes << ":" <<  setw(2) << (int)seconds << ":" << setw(2) << (int)frames << ")";
        if (!ShouldDecode)
            cout << ", is discarded";
        cout << endl;

        Config_Out->TC_current = tcBCD;
        timecode->Release();
    }
    else
    {
        cout << "New frame (no timecode)" << endl;
    }

    if (ShouldDecode)
    {
        void *buffer;
        arrivedVideoFrame->GetBytes(&buffer);
        if (Glue && *Glue)
            (*Glue)->OutputFrame((unsigned char*)buffer, 720*486*2, m_FramePos);

        m_FramePos++;

        if (Config_In->FrameCount != -1
         && m_FramePos >= Config_In->FrameCount)
        {
            stopCapture();
        }
    }
    
    return S_OK;
}

#endif // defined(BLACKMAGICDECKLINK_YES)


/****************************************************************************
 * Control.h                                                                *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 6, 2012                                                  *
 * Authors: Konstantin Miller <konstantin.miller@tu-berlin.de>              *
 *                                                                          *
 * This program is free software: you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.     *
 ****************************************************************************/

#ifndef CONTROL_H_
#define CONTROL_H_

#include "dashp2p.h"
#include "mpd/model.h"
#include "MpdWrapper.h"
#include "DebugAdapter.h"
#include "DashHttp.h"
#include "SegmentStorage.h"
#include "Contour.h"
#include "ControlLogic.h"
#include "ControlLogicAction.h"
#include "HttpEvent.h"

#include <vlc_common.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
using std::map;
using std::set;

namespace dashp2p {

class Control
{
protected:
    Control(){}
    virtual ~Control(){}

/* Public methods. */
public:

    /* Initialization and clean-up */
    static void init(const std::string& mpdUrl, int windowWidth, int windowHeight,
    		ControlType controlType, std::string adaptationConfiguration /*, bool withHandover*/);
    static void cleanUp();

    /* Main loop of the Control thread. */
    static void* controlThreadMain(void* params);

    /* Playback control */
    //static void prepareToPause();
    //static void pause();

    /* Callback for data downloaded by DashHttp. Don't call any DashHttp methods from this call because of internal DashHttp mutexes (might cause dead lock). */
    static void httpCb(HttpEvent* e);

    /**
     * Interface to the DASH-P2P plugin file. Called to retrieve more video data.
     * If valid pointer to memory is given, we copy video data to the buffer.
     * Otherwise, we allocate memory ourselves.
     * @param buffer Pointer to the memory. If NULL, we allocate memory ourselves.
     * @param bufferSize Size of the allocated memory.
     * @param bytesReturned Number of bytes actually returned.
     * @param secReturned Approximate number of seconds returned.
     * @return 0 if EOF, otherwise 1.
     */
    static int vlcCb(char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned);

    /* Stream related stuff */
    static dashp2p::URL& getMpdUrl() {return splittedMpdUrl;}
    static int64_t getPosition();
    static std::vector<int64_t> getSwitchingPoints(int num);
    //static unsigned getLowestBitrate() {return representations.at(0).bandwidth;}
    //static int getStartSegment() {return startSegment;}

    /* Handover related stuff */
    //static void receiveDisplayHandover(const std::string& mpdUrl, int64_t startPosition, int64_t startTime);

    /* Overlay related stuff */
    static void displayThroughputOverlay(int segNr, int64_t usec, double thrpt);

    //static void toFile (ContentIdSegment segId, string& fileName){SegmentStorage::toFile(segId,fileName);};

/* Private methods. */
protected:

    static pair<bool,bool> waitSelect(struct timeval selectTimeout);

    //static void httpConnected(const HttpEventConnected& e);
    static void httpDataReceived(HttpEventDataReceived& e);
    static void httpDataReceived_Mpd(HttpEventDataReceived& e);
    //static void httpDataReceived_MpdPeer(HttpEventDataReceived& e);
    //static void httpDataReceived_Tracker(HttpEventDataReceived& e);
    static void httpDataReceived_Segment(HttpEventDataReceived& e);
    static void httpDisconnect(const HttpEventDisconnect& e);

    static bool processAction                   (const ControlLogicAction& a);
    //static bool processActionCloseTcpConnection (const ControlLogicActionCloseTcpConnection& a);
    //static bool processActionOpenTcpConnection  (const ControlLogicActionOpenTcpConnection& a);
    static bool processActionStartDownload      (const ControlLogicActionStartDownload& a);

    /* Playback related */
    // Must be protected by mutexes!
    static void resumePlayback();

    static StreamPosition getNextPosition();

    static bool eof();

    //static void closeConnection(const TcpConnectionId& tcpConnectionId);

protected:
    //enum ControlState {ControlState_Initializing, ControlState_Playing, ControlState_PrepareToPause, ControlState_Paused, ControlState_Terminating, ControlState_Dead};
    enum ControlState {ControlState_Initializing, ControlState_Playing, ControlState_Terminating, ControlState_Dead};
    //typedef map<ReqId, pair<SegId, HttpMethod> > RequestMap; // TODO: consider removing
    //typedef map<int, DashHttp*> HttpMap;

/* Private variables. */
protected:

    static ControlState state;

    /* Main thread related stuff */
    static vlc_thread_t controlThread;
    static Mutex mutex;
    static Mutex actionsMutex;
    static int fdActions;
    static list<ControlLogicAction*> actions;
    static Mutex eventsMutex;
    static int fdEvents;
    static list<ControlLogicEvent*> events;

    /* General stream related stuff */
    static bool forcedEof;

    /* Playback related stuff */
    // last byte given to VLC
    static StreamPosition curPos;
    static CondVar playbackPausedCondvar;

    /* MPD related stuff. */
    static dashp2p::URL splittedMpdUrl;
    //static MpdWrapper* mpdWrapper;

    /* Adaptation related stuff */
    static ControlLogic* controlLogic;
    static ControlType controlType;
    static std::string adaptationConfiguration;

    /* Download related stuff */
    //static HttpMap httpMap;
    //static RequestMap requestMap;

    /* Storage related */
    //static SegmentStorage* storage;

    /* Display handover related stuff */
    //static bool withHandover;
    //static sem_t displayHandoverSema;

    /* Logging related */
    static int64_t beginUnderrun;

    /* Overlay related */
    static int64_t lastReportedBufferLevelTime;
    static int64_t lastReportedBufferLevel;
    //static int64_t lastReportedThroughputTime;

    /* Related to fetching segment sizes */
    static map<ContentIdSegment, int64_t> segmentSizes;
};

}

#endif /* CONTROL_H_ */

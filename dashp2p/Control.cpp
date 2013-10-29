/****************************************************************************
 * Control.cpp                                                              *
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

//#include "Dashp2pTypes.h"
#include "Utilities.h"
#include "Control.h"
#include "HttpRequestManager.h"
#include "TcpConnectionManager.h"
#include "SourceManager.h"
#include "DashHttp.h"
//#include "DisplayHandover.h"
#include "Statistics.h"
#include "OverlayAdapter.h"
#include "ControlLogicST.h"
//#include "ControlLogicMH.h"
//#include "ControlLogicP2P.h"
#include "ControlLogicEvent.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_playlist.h>

#include <sys/eventfd.h>
#include <ctime>
#include <string>
#include <cassert>
#include <cmath>
#include <clocale>
#include <limits>
#include <cerrno>

#define WITH_OVERLAY_STATISTICS 0

// TODO: let the buffer grow infinitely large and discard buffered content upon bandwidth increase
// TODO: do HEAD requests to replace segment size by real segmen size

namespace dashp2p {

Control::ControlState Control::state = ControlState_Initializing;

/* Main thread related stuff */
vlc_thread_t Control::controlThread;
Mutex Control::mutex;
Mutex Control::actionsMutex;
int Control::fdActions = 0;
list<ControlLogicAction*> Control::actions;
Mutex Control::eventsMutex;
int Control::fdEvents = 0;
list<ControlLogicEvent*> Control::events;

/* General stream related stuff */
bool Control::forcedEof = false;

/* Playback related stuff */
StreamPosition Control::curPos = StreamPosition();
CondVar Control::playbackPausedCondvar;
//Contour Control::contour;

/* MPD related stuff. */
dashp2p::URL Control::splittedMpdUrl;
//MpdWrapper* Control::mpdWrapper = NULL;
//DataField* Control::mpdDataField = NULL;

/* Adaptation related stuff */
ControlLogic* Control::controlLogic = NULL;
ControlType Control::controlType = ControlType_ST;
std::string Control::adaptationConfiguration = std::string();
//int Control::width = 0;
//int Control::height = 0;

/* Download related stuff */
Control::HttpMap Control::httpMap;
//Control::RequestMap Control::requestMap;

/* Storage related */
SegmentStorage* Control::storage = NULL;

/* Display handover related stuff */
//sem_t Control::displayHandoverSema;
//bool Control::withHandover = false;

/* Logging related */
int64_t Control::beginUnderrun = -1;

/* Overlay related */
int64_t Control::lastReportedBufferLevelTime = -1;
int64_t Control::lastReportedBufferLevel = -1;
//int64_t Control::lastReportedThroughputTime = -1;

/* Related to fetching segment sizes */
map<ContentIdSegment,int64_t> Control::segmentSizes;


void Control::init(
        const std::string& mpdUrl,
        int windowWidth, int windowHeight,
        ControlType controlType,
        std::string adaptationConfiguration /*, bool withHandover*/)
{
    /* Avoid decimal comma instead of dot in printf() output, as happens, e.g., with Ubuntu 12.04 set to German locale. */
    setlocale(LC_NUMERIC, "C");

    DBGMSG("Initializing Control module.");

    state = ControlState_Initializing;

    /* Initialize HttpRequestManager */
    HttpRequestManager::init();

    /* Main thread related stuff */
    //Control::controlThread;
    ThreadAdapter::mutexInit(&mutex);
    ThreadAdapter::mutexInit(&actionsMutex);
    fdActions = eventfd(0, EFD_SEMAPHORE);
    dp2p_assert(fdActions != -1);
    dp2p_assert(actions.empty());
    ThreadAdapter::mutexInit(&eventsMutex);
    fdEvents = eventfd(0, EFD_SEMAPHORE);
    dp2p_assert(fdEvents != -1);
    dp2p_assert(events.empty());

    /* General stream related stuff */
    Control::forcedEof = false;

    /* Playback related stuff */
    Control::curPos = StreamPosition(); /* last byte given to VLC */
    ThreadAdapter::condVarInit(&playbackPausedCondvar);
    //Control::contour = Contour();

    /* MPD related stuff. */
    if(mpdUrl.compare("dummy.mpd") == 0)
        Control::splittedMpdUrl = dashp2p::URL();
    else
        Control::splittedMpdUrl = dashp2p::Utilities::splitURL(mpdUrl);
    //Control::mpdWrapper = NULL;
    //Control::mpdDataField = NULL;

    /* Adaptation related stuff */
    switch(controlType) {
        case ControlType_ST:  controlLogic = new ControlLogicST(windowWidth, windowHeight, adaptationConfiguration);  break;
        //case ControlType_MH:  controlLogic = new ControlLogicMH(adaptationConfiguration);  break;
        //case ControlType_P2P: controlLogic = new ControlLogicP2P(windowWidth, windowHeight, startPosition, stopPosition, adaptationConfiguration); break;
        default: dp2p_assert(0); break;
    }
    Control::controlType = controlType;
    Control::adaptationConfiguration = adaptationConfiguration;
    //Control::width = windowWidth;
    //Control::height = windowHeight;

#if 0
    /* Download related stuff */
    if(mpdUrl.compare("dummy.mpd") != 0) {
        DBGMSG("Initializing DashHttp module(s).");
        unsigned maxPendingRequests = 0;
        if(controlType == ControlType_MH) {
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getPrimaryIf(), maxPendingRequests, callbackForDownloadedData);
            httpSecondary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getSecondaryIf(), maxPendingRequests, callbackForDownloadedData);
        } else {
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, IfData(), maxPendingRequests, callbackForDownloadedData);
            httpSecondary = NULL;
        }
        DBGMSG("DashHttp module initialized.");
    }
    dp2p_assert(Control::requestMap.empty());
#endif

    /* Storage related */
    dp2p_assert(storage == NULL);
    storage = new SegmentStorage();

    /* Display handover related stuff */
    //dp2p_assert(0 == sem_init(&displayHandoverSema, 0, 0));
    //Control::withHandover = withHandover;
    //const int displayHandoverPort = 12345;
    //DBGMSG("Initializing DisplayHandover module.");
    //DisplayHandover::init(displayHandoverPort, withHandover);

    /* Logging related */
    Control::beginUnderrun = -1;

    /* Overlay related */
    Control::lastReportedBufferLevelTime = -1;
    Control::lastReportedBufferLevel = -1;
    //lastReportedThroughputTime = -1;

    /* Related to fetching segment sizes */
    dp2p_assert(segmentSizes.size() == 0);
    //fetchingSegmentSizesCondVar
    //fetchingSegmentSizesMutex

    /* Other stuff */

    /* some initial logging */
    Statistics::recordScalarU64("referenceTime", dashp2p::Utilities::getReferenceTime());

    if(mpdUrl.compare("dummy.mpd") != 0) {
        state = ControlState_Playing;
    } else {
        dp2p_assert(state == ControlState_Initializing);
    }

    /* Start the main thread */
    dp2p_assert(0 == vlc_clone (&controlThread, Control::controlThreadMain, NULL, VLC_THREAD_PRIORITY_LOW));

    DBGMSG("Control module initialized.");
}

void Control::cleanUp()
{
    DBGMSG("Doing cleanup in Control.");

    /* Tell Control thread to terminate */
    switch(state) {
    case ControlState_Initializing: dp2p_assert(0); break;
    case ControlState_Playing: state = ControlState_Terminating; break;
    //case ControlState_Paused:state = ControlState_Terminating; break;
    case ControlState_Terminating: dp2p_assert(0); break;
    case ControlState_Dead: dp2p_assert(0); break;
    default: dp2p_assert(0); break;
    }

    /* Stop downloader thread and clean-up */
    for(HttpMap::iterator it = httpMap.begin(); it != httpMap.end(); ++it)
        delete it->second;
    httpMap.clear();
    //Control::requestMap.clear();

    /* Main thread related stuff */
    vlc_join(controlThread, NULL);
    ThreadAdapter::mutexDestroy(&mutex);
    ThreadAdapter::mutexDestroy(&actionsMutex);
    dp2p_assert(0 == close(fdActions));
    while(!actions.empty()) {
        delete actions.front();
        actions.pop_front();
    }
    ThreadAdapter::mutexDestroy(&eventsMutex);
    dp2p_assert(0 == close(fdEvents));
    while(!events.empty()) {
    	delete events.front();
    	events.pop_front();
    }

    /* General stream related stuff */
    Control::forcedEof = true;

    /* Playback related stuff */
    Control::curPos = StreamPosition();
    ThreadAdapter::condVarDestroy(&Control::playbackPausedCondvar);
    //Control::contour.clear();

    /* MPD related stuff. */
    Control::splittedMpdUrl = dashp2p::URL();
    //delete Control::mpdWrapper; Control::mpdWrapper = NULL;
    //delete mpdDataField; mpdDataField = NULL;

    /* Adaptation related stuff */
    delete controlLogic; controlLogic = NULL;
    Control::controlType = controlType;
    Control::adaptationConfiguration = std::string();
    //Control::width = 0;
    //Control::height = 0;

    /* Storage related */
    delete storage; storage = NULL;

    /* Display handover related stuff */
    //dp2p_assert(0 == sem_destroy(&displayHandoverSema));
    // Control::withHandover
    //DisplayHandover::cleanup();

    /* Logging related */
    Control::beginUnderrun = -1;

    /* Overlay related */
    Control::lastReportedBufferLevelTime = -1;
    Control::lastReportedBufferLevel = -1;
    //lastReportedThroughputTime = -1;

    /* Related to fetching segment sizes */
    segmentSizes.clear();
    //fetchingSegmentSizesCondVar
    //fetchingSegmentSizesMutex

    /* Cleanup HttpRequestManager */
    HttpRequestManager::cleanup();

    dp2p_assert(state == ControlState_Terminating);
    state = ControlState_Dead;

    DBGMSG("Clean-up in Control successfull.");
}

void* Control::controlThreadMain(void* /*params*/)
{
    /* If we are in DisplayHandover mode, wait until SET_POSITION was called */
    //if(splittedMpdUrl.hostName.empty()) {
    //    dp2p_assert(state == ControlState_Initializing);
    //    dp2p_assert(0 == sem_wait(&displayHandoverSema));
    //    dp2p_assert(!splittedMpdUrl.hostName.empty());
    //    state = ControlState_Playing;
    //} else {
        dp2p_assert(state == ControlState_Playing);
    //}

    /* Open the game by downloading the MPD file */
    list<ControlLogicAction*> newActions = controlLogic->processEvent(new ControlLogicEventStartPlayback(splittedMpdUrl));
    const int numNewActions = newActions.size();
    ThreadAdapter::mutexLock(&actionsMutex);
    actions.splice(actions.end(), newActions);
    uint64_t buf = numNewActions;
    dp2p_assert(8 == write(fdActions, &buf, 8));
    DBGMSG("Received %d new actions. Now in total: %d pending actions.", numNewActions, actions.size());
    ThreadAdapter::mutexUnlock(&actionsMutex);

    DBGMSG("Starting Control's main loop.");

    /* Main loop */
    while(state == ControlState_Playing /*|| state == ControlState_Paused*/)
    {
        /* If no actions and no events available -> wait */
    	struct timeval selectTimeout;
    	selectTimeout.tv_sec = 1;
    	/*pair<bool,bool> retSelect =*/ Control::waitSelect(selectTimeout);

        /* If there are events, process ONE event. */
        ThreadAdapter::mutexLock(&eventsMutex);
        DBGMSG("Control's main loop: %d events.", events.size());
        if(!events.empty())
        {
        	static uint64_t buf;
        	dp2p_assert(8 == read(fdEvents, &buf, 8) && buf == 1);
        	ControlLogicEvent* event = events.front();
        	events.pop_front();
        	ThreadAdapter::mutexUnlock(&eventsMutex);

        	ThreadAdapter::mutexLock(&mutex);
        	DBGMSG("Processing event: %s.", event->toString().c_str());
        	list<ControlLogicAction*> newActions = controlLogic->processEvent(event);
        	ThreadAdapter::mutexUnlock(&mutex);

        	const int numNewActions = newActions.size();

        	ThreadAdapter::mutexLock(&actionsMutex);
        	actions.splice(actions.end(), newActions);
        	buf = numNewActions;
        	dp2p_assert(8 == write(fdActions, &buf, 8));
        	DBGMSG("Received %d new actions. Now in total: %d pending actions.", numNewActions, actions.size());
        	ThreadAdapter::mutexUnlock(&actionsMutex);

        	continue;
        }
        else
        {
        	ThreadAdapter::mutexUnlock(&eventsMutex);
        }

        /* If there are actions, process ALL actions. */
        ThreadAdapter::mutexLock(&actionsMutex);
        DBGMSG("Control's main loop: %d actions.", actions.size());
        if(!actions.empty())
        {
        	while(!actions.empty())
        	{
        		static uint64_t buf;
        		dp2p_assert(8 == read(fdActions, &buf, 8) && buf == 1);
        		ControlLogicAction* action = actions.front();
        		actions.pop_front();
        		ThreadAdapter::mutexUnlock(&actionsMutex);

        		ThreadAdapter::mutexLock(&mutex);
        		DBGMSG("Processing action: %s.", action->toString().c_str());
        		if(processAction(*action)) {
        			delete action;
        			ThreadAdapter::mutexUnlock(&mutex);
        		} else {
        			THROW_RUNTIME("Action was rejected: %s.", action->toString().c_str());

        			/*list<ControlLogicAction*> newActions = controlLogic->actionRejected(action); */
        			ThreadAdapter::mutexUnlock(&mutex);

        			/*const int numNewActions = newActions.size();

        			ThreadAdapter::mutexLock(&actionsMutex);
        			actions.splice(actions.end(), newActions);
        			uint64_t buf = numNewActions;
        			dp2p_assert(8 == write(fdActions, &buf, 8));
        			DBGMSG("Received %d new actions. Now in total: %d pending actions.", numNewActions, actions.size());
        			ThreadAdapter::mutexUnlock(&actionsMutex);*/
        		}

        		ThreadAdapter::mutexLock(&actionsMutex);
        	}
        }
        ThreadAdapter::mutexUnlock(&actionsMutex);

    }
    dp2p_assert(state == ControlState_Terminating);

    return NULL;
}

#if 0
void Control::prepareToPause()
{
	// TODO: discard data in decoder FIFOs so that the playback is stopped immediately, alternatively disable video output here

	DBGMSG("Going to PrepareToPause at %s.", dashp2p::Utilities::getTimeString(stopTime).c_str());

	dp2p_assert(state == ControlState_Playing);

	ThreadAdapter::mutexLock(&mutex);
	state = ControlState_PrepareToPause;
	ThreadAdapter::mutexUnlock(&mutex);
}
#endif

#if 0
// TODO: consider stopTime
void Control::pause()
{
    // TODO: discard data in decoder FIFOs so that the playback is stopped immediately, alternatively disable video output here

    DBGMSG("Going to paused at %s.", dashp2p::Utilities::getTimeString(stopTime).c_str());

    dp2p_assert(state == ControlState_PrepareToPause);

    ThreadAdapter::mutexLock(&actionsMutex);
    actions.clear();
    dp2p_assert(0 == close(fdActions));
    fdActions = eventfd(0, EFD_SEMAPHORE);
    ThreadAdapter::mutexUnlock(&actionsMutex);

    ThreadAdapter::mutexLock(&eventsMutex);
    events.clear();
    dp2p_assert(0 == close(fdEvents));
    fdEvents = eventfd(0, EFD_SEMAPHORE);
    ThreadAdapter::mutexUnlock(&eventsMutex);

    ThreadAdapter::mutexLock(&mutex);
    state = ControlState_Paused;
    startTime = std::numeric_limits<int64_t>::max();
    //contour.clear();
    storage->clear();
    ThreadAdapter::mutexUnlock(&mutex);

    ThreadAdapter::mutexLock(&eventsMutex);
    events.push_back(new ControlLogicEventPause());
    uint64_t buf = 1;
    dp2p_assert(8 == write(fdEvents, &buf, 8));
    ThreadAdapter::mutexUnlock(&eventsMutex);

    //DisplayHandover::playbackTerminated();
}
#endif

//void Control::callbackForDownloadedData(unsigned reqId, string devName, const char* buffer, unsigned bytesFrom, unsigned bytesTo, unsigned bytesTotal)
void Control::httpCb(HttpEvent* _e)
{
	DBGMSG("Got new HTTP event: %s.", _e->toString().c_str());

	switch(_e->getType())
	{

	/*case HttpEvent_Connected: {
		const HttpEventConnected& e = dynamic_cast<const HttpEventConnected&>(*_e);
		httpConnected(e);
		break;
	}*/

	case HttpEvent_DataReceived: {
		HttpEventDataReceived& e = dynamic_cast<HttpEventDataReceived&>(*_e);
		httpDataReceived(e);
		break;
	}

	case HttpEvent_Disconnect: {
		const HttpEventDisconnect& e = dynamic_cast<const HttpEventDisconnect&>(*_e);
		httpDisconnect(e);
		break;
	}

	default: {
		ERRMSG("Unknown HTTP event type.");
		abort();
		break;
	}

	}

	delete _e;
}

#if WITH_OVERLAY_STATISTICS == 1
void Control::displayThroughputOverlay(int segNr, double t, double thrpt)
#else
void Control::displayThroughputOverlay(int /* segNr */, double /* t */, double thrpt)
#endif
{
    /* overlay output */
    //if(dashp2p::Utilities::getTime() / 1000000 > lastReportedThroughputTime)
    //{
    char tmp[128];
    if(controlType == ControlType_ST) {
        sprintf(tmp, "Throughput");
        //const double Delta_t = dynamic_cast<ControlLogicST*>(controlLogic)->get_Delta_t();
//#ifdef __ANDROID__
//        if(!withHandover)
//            DisplayHandover::displayOverlay(3, "%5s: % 8.3f Mbps", "Thrpt", Statistics::getThroughput(std::min<double>(Delta_t, dashp2p::Utilities::now())) / 1e6);
//#else
        OverlayAdapter::print(3, "%5s: % 8.3f Mbps", "Thrpt", thrpt / 1e6);
#if WITH_OVERLAY_STATISTICS == 1
        {
            static FILE* f = NULL;
            if(!f) f = fopen("/tmp/dp2p_throughput.txt", "w");
            fprintf(f, "%8.2f %5d %8.2f\n", t, segNr, thrpt / 1e6);
            fflush(f);
        }
#endif
//#endif
    }

#if 0
    else if(controlType == ControlType_MH) {
        const ControlLogicMH& controlLogicMH = dynamic_cast<const ControlLogicMH&>(*controlLogic);
        const string primaryIfName = controlLogicMH.getPrimaryIf().name;
        const string secondaryIfName = controlLogicMH.getSecondaryIf().name;
        const double Delta_t = 5; // TODO: ask controlLogicMH once implemented
        sprintf(tmp, "Thrpt %s", primaryIfName.c_str());
        OverlayAdapter::print(3, "%10s: % 8.3f Mbps", tmp, Statistics::getThroughput(std::min<double>(Delta_t, dashp2p::Utilities::now()), primaryIfName) / 1e6);
        sprintf(tmp, "Thrpt %s", secondaryIfName.c_str());
        OverlayAdapter::print(4, "%10s: % 8.3f Mbps", tmp, Statistics::getThroughput(std::min<double>(Delta_t, dashp2p::Utilities::now()), secondaryIfName) / 1e6);
    }
#endif

    else if(controlType == ControlType_P2P) {
        sprintf(tmp, "Throughput");
        OverlayAdapter::print(3, "%5s: % 8.3f Mbps", "Thrpt", 0);
    }

    else {
        dp2p_assert(0);
    }
        //lastReportedThroughputTime = dashp2p::Utilities::getTime() / 1000000;
    //}
}

pair<bool,bool> Control::waitSelect(struct timeval selectTimeout)
{
	fd_set fdSetRead;
	FD_ZERO(&fdSetRead);
	FD_SET(fdActions, &fdSetRead);
	FD_SET(fdEvents, &fdSetRead);
	DBGMSG("Going to select() for up to %gs.", selectTimeout.tv_sec + selectTimeout.tv_usec / 1e6);
	const int64_t ticBeforeSelect = Utilities::getAbsTime();
	const int retSelect = select(max<int>(fdActions, fdEvents) + 1, &fdSetRead, NULL, NULL, &selectTimeout);
	const int64_t tocAfterSelect = Utilities::getAbsTime();
	if(retSelect == -1) {
		printf("errno = %d\n", errno);
		abort();
	}

	DBGMSG("Was in select() for %gs, returning %d.", (tocAfterSelect - ticBeforeSelect) / 1e6, retSelect);

	return pair<bool,bool>(FD_ISSET(fdActions, &fdSetRead), FD_ISSET(fdEvents, &fdSetRead));
}

#if 0
void Control::httpConnected(const HttpEventConnected& e)
{
	ThreadAdapter::mutexLock(&eventsMutex);
	//DBGMSG("Locked mutex.");
	events.push_back(new ControlLogicEventConnected(e.getConnId()));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventConnected to the event list.");
	ThreadAdapter::mutexUnlock(&eventsMutex);
}
#endif

void Control::httpDataReceived(HttpEventDataReceived& e)
{
	switch(HttpRequestManager::getContentType(e.reqId))
	{

	case ContentType_Mpd:
		httpDataReceived_Mpd(e);
		break;

	case ContentType_Segment:
		httpDataReceived_Segment(e);
		break;

	/*case ContentType_Tracker:
		httpDataReceived_Tracker(e);
		break;*/

	/*case ContentType_MpdPeer:
		httpDataReceived_MpdPeer(e);
		break;*/

	default:
		ERRMSG("Unknown ContentId type.");
		abort();
		break;

	}
}

void Control::httpDataReceived_Mpd(HttpEventDataReceived& e)
{
	ThreadAdapter::mutexLock(&eventsMutex);
	dp2p_assert(HttpRequestManager::getHttpMethod(e.reqId) == HttpMethod_GET && state == ControlState_Playing && HttpRequestManager::getContentLength(e.reqId) > 0);
	DBGMSG("Got (piece of) the MPD, ContentId: %s.", HttpRequestManager::getContentId(e.reqId).toString().c_str());
	events.push_back(new ControlLogicEventDataReceived(e.getConnId(), e.reqId, e.byteFrom, e.byteTo, pair<int64_t, int64_t>(0,0)));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventDataReceived to the event list.");
	ThreadAdapter::mutexUnlock(&eventsMutex);
}

#if 0
void Control::httpDataReceived_Tracker(HttpEventDataReceived& e)
{
	ThreadAdapter::mutexLock(&eventsMutex);
	dp2p_assert(HttpRequestManager::getHttpMethod(e.reqId) == HttpMethod_GET && state == ControlState_Playing && HttpRequestManager::getContentLength(e.reqId) > 0);
	DBGMSG("Got (piece of) the Tracker, ContentId: %s.", HttpRequestManager::getContentId(e.reqId).toString().c_str());
	events.push_back(new ControlLogicEventDataReceived(e.getConnId(), e.reqId, e.byteFrom, e.byteTo, pair<int64_t, int64_t>(0,0)));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventDataReceived to the event list.");
	ThreadAdapter::mutexUnlock(&eventsMutex);
}

void Control::httpDataReceived_MpdPeer(HttpEventDataReceived& e)
{
	ThreadAdapter::mutexLock(&eventsMutex);
	dp2p_assert(HttpRequestManager::getHttpMethod(e.reqId) == HttpMethod_GET && state == ControlState_Playing && HttpRequestManager::getContentLength(e.reqId) > 0);
	DBGMSG("Got (piece of) the MpdPeer, ContentId: %s.", HttpRequestManager::getContentId(e.reqId).toString().c_str());
	events.push_back(new ControlLogicEventDataReceived(e.getConnId(), e.reqId, e.byteFrom, e.byteTo, pair<int64_t, int64_t>(0,0)));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventDataReceived to the event list.");
	ThreadAdapter::mutexUnlock(&eventsMutex);
}
#endif

void Control::httpDataReceived_Segment(HttpEventDataReceived& e)
{
	// TODO: This basically does two things: adds data to storage and notifies ControlLogic. Refactor by adding something like addDataToStorage()
	// TODO: consider connecting HttpRequest to the storage directly to avoid one copying

	DBGMSG("Got (piece of) a segment with ContentId: %s.", HttpRequestManager::getContentId(e.reqId).toString().c_str());

	switch(state) {
	case ControlState_Initializing: ERRMSG("[%.3fs] Enter. State: Initializing.", dashp2p::Utilities::now()); exit(1);
	case ControlState_Playing: DBGMSG("Enter. State: Playing."); break;
	//case ControlState_Paused: DBGMSG("Enter. State: Paused."); break;
	case ControlState_Terminating: DBGMSG("Enter. State: Terminating."); return;
	case ControlState_Dead: ERRMSG("[%.3fs] Enter. State: Dead.", dashp2p::Utilities::now()); return;
	default: dp2p_assert(0); break;
	}

	const ContentIdSegment& segId = dynamic_cast<const ContentIdSegment&>(HttpRequestManager::getContentId(e.reqId));

	/* Debug output and sanity checks */
	switch(HttpRequestManager::getHttpMethod(e.reqId)) {
	case HttpMethod_GET:
		DBGMSG("Received payload bytes [%d, %d] (out of %" PRId64 ") of segment %d (request nr. %d).",
				e.byteFrom, e.byteTo, HttpRequestManager::getContentLength(e.reqId), segId.segmentIndex(), e.reqId);
		if(e.byteTo == 0) // We just received the header. Not interested.
			return;
		break;
	case HttpMethod_HEAD:
		DBGMSG("Received HEAD of segment %d (request nr. %d). Payload: %" PRId64 ".", segId.segmentIndex(), e.reqId, HttpRequestManager::getContentLength(e.reqId));
		break;
	default:
		dp2p_assert_v(false, "Unknown HTTP method: %d. Bug?", HttpRequestManager::getHttpMethod(e.reqId));
		break;
	}

	if(HttpRequestManager::getHttpMethod(e.reqId) == HttpMethod_HEAD) {
		dp2p_assert(segmentSizes.insert(pair<ContentIdSegment,int64_t>(segId, HttpRequestManager::getContentLength(e.reqId))).second);
		Statistics::recordSegmentSize(segId, HttpRequestManager::getContentLength(e.reqId));
		return;
	}/* else if(state == ControlState_Paused) {
		DBGMSG("Ignoring downloaded data while in paused.");
		return;
	}*/

	ThreadAdapter::mutexLock(&mutex);
	DBGMSG("Locked mutex.");

	dp2p_assert(segId.segmentIndex() <= controlLogic->getStopSegment() && HttpRequestManager::getContentLength(e.reqId) > 0);

	/* If it is first data of the segment, initialize the corresponding Segment object */
	if(!storage->initialized(segId)) {
		DBGMSG("Segment not yet available in the storage. Initializing.");
		storage->initSegment(segId, HttpRequestManager::getContentLength(e.reqId), controlLogic->getMpdWrapper()->getSegmentDuration(segId));
	} else {
		DBGMSG("Segment aleady registered in the storage module.");
	}

	/* Create a DataBlock to hold a copy of the data and add it to the storage */
	DBGMSG("Bevor addData");
	// TODO: make DashHttp write data direct to segment storage so nothing needs to be done here
	storage->addData(segId, e.byteFrom, e.byteTo, HttpRequestManager::getPldBytes(e.reqId) + e.byteFrom, false);
	DBGMSG("After addData");

	/* If segment completed, dump segment data */
#if 0
	if(bytesTo == bytesTotal - 1)
	{
		dashp2p::URL splittedUrl = dashp2p::Utilities::splitURL(mpdWrapper->getSegmentURL(segId));
		//FILE* fileDumpSegmentData = fopen(splittedUrl.fileName.c_str(), "w");
		FILE* fileDumpSegmentData = fopen("output.mp4", "a");
		dp2p_assert(fileDumpSegmentData);
		char* segData = NULL;
		int segDataSize = 0;
		int segDataReturned = 0;
		int64_t segUsecReturned = 0;
		StreamPosition strPos = storage->getSegmentData(StreamPosition(segId, 0), &segData, &segDataSize, &segDataReturned, &segUsecReturned);
		dp2p_assert(strPos.segId == segId && strPos.byte == (int64_t)bytesTotal - 1 && segData && segDataSize == (int)bytesTotal && segDataReturned == (int)bytesTotal);
		dp2p_assert(bytesTotal == (int)fwrite(segData, 1, bytesTotal, fileDumpSegmentData));
		dp2p_assert(0 == fclose(fileDumpSegmentData));
	}
#endif

	DBGMSG("Calculating contig interval.");
	const pair<int64_t, int64_t> availableContigInterval = storage->getContigInterval(getNextPosition(), controlLogic->getContour());

	DBGMSG("Have %" PRId64 " bytes (%" PRId64 " us) of contiguous data in the storage.", availableContigInterval.second, availableContigInterval.first);

	// TODO: underrun handling!
	if(state == ControlState_Playing && availableContigInterval.second > 0)
		resumePlayback();

	/* logging */
	//if(dumpStatistics) {
	//    fprintf(fileBytesStored, "% 17.6f % 17.6f\n", dashp2p::Utilities::now(), (bytesStored.second - bytesStored.first) / 1e6);
	//    fprintf(fileSecStored, "% 17.6f % 17.6f\n", dashp2p::Utilities::now(), (usecStored.second - usecStored.first) / 1e6);
	//    fprintf(fileSecDownloaded, "% 17.6f % 17.6f\n", dashp2p::Utilities::now(), usecDownloaded / 1e6);
	//}

	/* overlay output */
	if(dashp2p::Utilities::getTime() / 1000000 > lastReportedBufferLevelTime || availableContigInterval.first / 1000000 > lastReportedBufferLevel / 1000000) {
//#ifdef __ANDROID__
//	    if(!withHandover)
//	        DisplayHandover::displayOverlay(2, "%5s: % 8.0f sec", "Buf", availableContigInterval.first / 1e6);
//#else
	    OverlayAdapter::print(2, "%5s: % 8.0f sec", "Buf", availableContigInterval.first / 1e6);
//#endif
	    lastReportedBufferLevelTime = dashp2p::Utilities::getTime() / 1000000;
	    lastReportedBufferLevel = availableContigInterval.first;
	}

	//dp2p_assert(state != ControlState_Paused);
	ThreadAdapter::mutexUnlock(&mutex);

	ThreadAdapter::mutexLock(&eventsMutex);
	events.push_back(new ControlLogicEventDataReceived(e.getConnId(), e.reqId, e.byteFrom, e.byteTo, availableContigInterval));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventDataReceived to the event list.");
	ThreadAdapter::mutexUnlock(&eventsMutex);

	DBGMSG("Return.");
}

void Control::httpDisconnect(const HttpEventDisconnect& e)
{
	ThreadAdapter::mutexLock(&eventsMutex);
	//ThreadAdapter::mutexLock(&actionsMutex);
	//DBGMSG("Locked mutex.");

#if 0
	/* get all queued actions belonging to this connection */
	list<ControlLogicAction*> A;
	for(list<ControlLogicAction*>::iterator it = actions.begin(); it != actions.end(); ) {
		ControlLogicAction* action = *it;
		if(action->tcpConnectionId == e.connId) {
			uint64_t buf = 0;
			dp2p_assert(8 == ::read(fdActions, &buf, 8) && buf == 1);
			it = actions.erase(it);
			A.push_back(action);
		} else {
			++it;
		}
	}

	/* get all queued events belonging to this connection */
	list<ControlLogicEvent*> E;
	for(list<ControlLogicEvent*>::iterator it = events.begin(); it != events.end(); ) {
		ControlLogicEvent* event = *it;
		if(event->tcpConnectionId == e.connId) {
			uint64_t buf = 0;
			dp2p_assert(8 == ::read(fdActions, &buf, 8) && buf == 1);
			it = actions.erase(it);
			A.push_back(action);
		} else {
			++it;
		}
	}
#endif

	//closeConnection(e.connId);
	events.push_back(new ControlLogicEventDisconnect(e.getConnId(), e.reqs));
	uint64_t buf = 1;
	dp2p_assert(8 == write(fdEvents, &buf, 8));
	DBGMSG("Added new ControlLogicEventDisconnect to the event list.");

	//ThreadAdapter::mutexUnlock(&actionsMutex);
	ThreadAdapter::mutexUnlock(&eventsMutex);
}

int Control::vlcCb(char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned)
{
    /* If terminating or stopTime passed, signal EOF */
    // TODO: set b_eof of access_t when EOF reached!

    bytesReturned[0] = 0;
    usecReturned[0] = 0;

    switch(state) {
    case ControlState_Initializing: DBGMSG("Enter. State: Initializing."); break;
    case ControlState_Playing: DBGMSG("Enter. State: Playing."); break;
    //case ControlState_PrepareToPause: DBGMSG("Enter. State: Preparing to pause."); break;
    //case ControlState_Paused: DBGMSG("Enter. State: Paused."); break;
    case ControlState_Terminating: DBGMSG("Termination flag set, aborting with EOF."); return 0;
    case ControlState_Dead: DBGMSG("We are already dead, aborting with EOF."); return 0;
    default: dp2p_assert(0); break;
    }

    /* Lock the mutex for storage access. */
    ThreadAdapter::mutexLock(&mutex);
    DBGMSG("Mutex locked.");

    /* If EOF, return. */
    if(eof()) {
        DBGMSG("VLC requests %d bytes. No data available. EOF reached.", bufferSize[0]);
        ThreadAdapter::mutexUnlock(&mutex);
        return 0;
    }

    /* Get absolute time */
    int64_t absNow = dashp2p::Utilities::getAbsTime();

    /*if(state == ControlState_PrepareToPause && getNextPosition().byte == 0)
    {
    	DBGMSG("We reached segment boundary. Going to pause.");
    	ThreadAdapter::mutexUnlock(&mutex);
    	pause();
    	return 1;
    }*/

    /* If we passed the startTime but no data is available yet: wait */
    //else if(state != ControlState_Paused && absNow >= startTime - startTimeTolerance)
    {
        //startTimeCrossed = true;
        if(controlLogic->getContour().empty() || !storage->dataAvailable(getNextPosition())) {
        	int64_t waitingTime = -1;
            if(controlLogic->getContour().empty()) {
            	waitingTime = 1000000;
                DBGMSG("We passed startTime but contour is empty. Will wait %gs until %11.4f.", waitingTime / 1e6, (absNow + waitingTime - Utilities::getReferenceTime()) / 1e6);
            } else {
            	waitingTime = 10000;
                DBGMSG("We passed startTime, curPos = (RepId: %d, SegNr: %d, offset: %" PRId64 "), no data available yet. Will wait %gs until %11.4f.",
                        curPos.segId.bitRate(), curPos.segId.segmentIndex(), curPos.byte, waitingTime / 1e6, (absNow + waitingTime - Utilities::getReferenceTime()) / 1e6);
            }
            if(curPos.valid() && beginUnderrun == -1) { // this is not the initial delay, this is a buffer underrun
                // TODO: consider the post-decoder cache when detecting underruns
                beginUnderrun = dashp2p::Utilities::getTime();
                DBGMSG("Buffer underrun detected.");
            }
            ThreadAdapter::condVarTimedWait(&playbackPausedCondvar, &mutex, absNow + waitingTime);
            // TODO: we might want to check and assert here that the return value of condVarTimedWait and the data availability are consistent
            if(state == ControlState_Terminating) { // terminating
                DBGMSG("Back from waiting. Everything is terminating. Will return.");
                ThreadAdapter::mutexUnlock(&mutex);
                return 0;
            } else if(!controlLogic->getContour().empty() && storage->dataAvailable(getNextPosition())) { // data available, go on
                DBGMSG("Back from waiting. Data is available.");
            } else { // still no data available, return
                DBGMSG("Back from waiting. Still no data available. Will return.");
                ThreadAdapter::mutexUnlock(&mutex);
                return 1;
            }
        }
    }

    /* We didn't pass the startTime yet or we are PAUSED. Wait */
    /*else
    {
        DBGMSG("We didn't pass the startTime yet or we are PAUSED. Will wait until %11.4fs (%11.4fs).",
        		(startTime - startTimeTolerance - Utilities::getReferenceTime()) / 1e6, (startTime - startTimeTolerance - absNow) / 1e6);
        int ret = ThreadAdapter::condVarTimedWait(&playbackPausedCondvar, &mutex, std::min<int64_t>(absNow + 1000000, startTime - startTimeTolerance));
        if(ret == 0) { // we have data and we passed startTime
            DBGMSG("Back from waiting. Data is available and startTime is passed.");
            absNow = dashp2p::Utilities::getAbsTime();
            dp2p_assert(storage->dataAvailable(getNextPosition()) && absNow >= startTime - startTimeTolerance);
            startTimeCrossed = true;
        } else {
            dp2p_assert(ret == ETIMEDOUT);
            if(controlLogic->getContour().empty() || !storage->dataAvailable(getNextPosition()) || absNow < startTime - startTimeTolerance) {
                DBGMSG("Back from waiting. Either still no data is available or we did't pass the startTime yet. Will return.");
                ThreadAdapter::mutexUnlock(&mutex);
                return 1;
            }
            startTimeCrossed = true;
        }
    }*/

    const pair<int64_t, int64_t> contigIntervalPre = storage->getContigInterval(getNextPosition(), controlLogic->getContour());
    DBGMSG("We passed startTime and %" PRId64 " bytes (%" PRId64 " us) are available starting from (RepId: %d, SegNr: %d, offset: %" PRId64 ").", contigIntervalPre.second, contigIntervalPre.first, curPos.segId.bitRate(), curPos.segId.segmentIndex(), curPos.byte);

    // TODO: this should be somewhere else for cleaner style
    //if(!curPos.valid()) {
    //    DisplayHandover::playbackStarted();
    //}

    // TODO: add underrun handling and output statistics!!!
    // [0xad3d7f70] main video output warning: picture is too late to be displayed (missing 100 ms)
    //
    // [0xb75006a8] main input error: ES_OUT_SET_(GROUP_)PCR  is called too late (jitter of 574 ms ignored)
    // [0xb75006a8] main input error: ES_OUT_RESET_PCR called

#if 0
    if(dashp2p::Utilities::now() >= 5 && dashp2p::Utilities::now() <= 10) {
        DBGMSG("VLC asks for %d bytes. Simulating underrun.", bufferSize[0]);
        //vlc_mutex_unlock(&storageMutex);
        ThreadAdapter::mutexUnlock(&mutex);
        return 1;
    }
#endif

    const StreamPosition lastPos = storage->getData(getNextPosition(), controlLogic->getContour(), buffer, bufferSize, bytesReturned, usecReturned);
    curPos = lastPos;

    const pair<int64_t, int64_t> contigIntervalPost = storage->getContigInterval(getNextPosition(), controlLogic->getContour());

#if 0
    /* Signal if beta is below Bdelay. */
    if(contigIntervalPost.first / 1e6 <= Bdelay) {
        //DBGMSG("Buffer level below Bdelay (%.6f <= %.6f). Can resume download.", secStored, Bdelay);
        Bdelay = std::numeric_limits<double>::infinity();
        vlc_cond_broadcast(&betaBelowBdelayCondition);
    }
#endif

    /* logging */
    Statistics::recordBytesStored(dashp2p::Utilities::getAbsTime(), contigIntervalPost.second);
    Statistics::recordUsecStored(dashp2p::Utilities::getAbsTime(), contigIntervalPost.first);
    if(beginUnderrun != -1) {
        Statistics::recordUnderrun(beginUnderrun, dashp2p::Utilities::getTime() - beginUnderrun);
        beginUnderrun = -1;
    }

    if(eof()) {
        DBGMSG("Returning %d bytes (%.6f sec). EOF reached.", bytesReturned[0], usecReturned[0] / 1e6);
        /* Unlock the mutex for storage access. */
        //vlc_mutex_unlock(&storageMutex);
        ThreadAdapter::mutexUnlock(&mutex);
        /* no more data to come */
        return 0;
    }

    DBGMSG("Returning %d bytes (%.6f sec). Still %.6f MB (%.6f sec) available. EOF not reached.",
            bytesReturned[0], usecReturned[0] / 1e6, contigIntervalPost.second / 1e6, contigIntervalPost.first / 1e6);

    //DBGMSG("%25s: %8u bps", "Displaying bit-rate", bitRateLastByte);
//#ifdef __ANDROID__
//    if(!withHandover)
//        DisplayHandover::displayOverlay(1, "%5s: % 8.3f bps", "Dspl", curPos.segId.repId() / 1e6);
//#else
    OverlayAdapter::print(1, "%5s: % 8.3f bps", "Dspl", curPos.segId.bitRate() / 1e6);
#if WITH_OVERLAY_STATISTICS == 1
    {
        static FILE* f = NULL;
        static double lastBrTime = -1;
        double now = dashp2p::Utilities::now();
        if(now >= lastBrTime + 1) {
            lastBrTime = now;
            if(!f) f = fopen("/tmp/dp2p_dspl.txt", "w");
            fprintf(f, "%8.2f %8.2f\n", now, curPos.segId.bitRate() / 1e6);
            fflush(f);
        }
    }
#endif
//#endif
    if(dashp2p::Utilities::getTime() / 1000000 > lastReportedBufferLevelTime || contigIntervalPost.first / 1000000 > lastReportedBufferLevel / 1000000) {
//#ifdef __ANDROID__
//        if(!withHandover)
//            DisplayHandover::displayOverlay(2, "%5s: % 8.0f sec", "Buf", contigIntervalPost.first / 1e6);
//#else
        OverlayAdapter::print(2, "%5s: % 8.0f sec", "Buf", contigIntervalPost.first / 1e6);
//#endif
        lastReportedBufferLevelTime = dashp2p::Utilities::getTime() / 1000000;
        lastReportedBufferLevel = contigIntervalPost.first;
    }

    /* Unlock the mutex for storage access. */
    //vlc_mutex_unlock(&storageMutex);
    ThreadAdapter::mutexUnlock(&mutex);

    ThreadAdapter::mutexLock(&eventsMutex);
    events.push_back(new ControlLogicEventDataPlayed(contigIntervalPost));
    uint64_t buf = 1;
    dp2p_assert(8 == write(fdEvents, &buf, 8));
    ThreadAdapter::mutexUnlock(&eventsMutex);

    /* more data to come */
    return 1;
}

int64_t Control::getPosition()
{
	const int64_t segmentSize = storage->getTotalSize(curPos.segId);
	return controlLogic->getMpdWrapper()->getPosition(curPos.segId, curPos.byte, segmentSize);
}

std::vector<int64_t> Control::getSwitchingPoints(int _num)
{
    if(controlLogic->getMpdWrapper() == NULL || !curPos.valid() || curPos.segId.segmentIndex() == controlLogic->getStopSegment())
        return std::vector<int64_t>();

    int num = std::min<int>(_num, controlLogic->getStopSegment() - curPos.segId.segmentIndex());
    std::vector<int64_t> retVal(num);
    for(unsigned i = 0; i < retVal.size(); ++i) {
    	const ContentIdSegment nextSegId(curPos.segId.periodIndex(), curPos.segId.adaptationSetIndex(), curPos.segId.bitRate(), curPos.segId.segmentIndex() + i);
    	retVal.at(i) = controlLogic->getMpdWrapper()->getEndTime(nextSegId);
    }
    return retVal;
}

#if 0
void Control::receiveDisplayHandover(const std::string& mpdUrl, int64_t startPosition, int64_t startTime)
{
    dp2p_assert(state == ControlState_Paused || state == ControlState_Initializing);

    ThreadAdapter::mutexLock(&mutex);

    if(state == ControlState_Initializing)
    {
        DBGMSG("Incoming handover while initializing. URL: %s, startPosition: %" PRId64 ", startTime: %" PRId64 " (%s).", mpdUrl.c_str(), startPosition, startTime, dashp2p::Utilities::getTimeString(startTime).c_str());

        dp2p_assert(!mpdUrl.empty());
        splittedMpdUrl = dashp2p::Utilities::splitURL(mpdUrl);

        controlLogic->setStartPosition(startPosition);
        Control::startTime = startTime;
        stopTime = std::numeric_limits<int64_t>::max();
        curPos = StreamPosition();
        forcedEof = false;

#if 0
        DBGMSG("Initializing DashHttp module(s).");
        unsigned maxPendingRequests = 0;
        if(controlType == ControlType_MH) {
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getPrimaryIf(), maxPendingRequests, callbackForDownloadedData);
            httpSecondary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getSecondaryIf(), maxPendingRequests, callbackForDownloadedData);
        } else {
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, IfData(), maxPendingRequests, callbackForDownloadedData);
            httpSecondary = NULL;
        }
        DBGMSG("DashHttp module initialized.");
#endif

        dp2p_assert(0 == sem_post(&displayHandoverSema));
    }
    else
    {
        dp2p_assert(state == ControlState_Paused);

        DBGMSG("Incoming handover while paused. URL: %s, startPosition: %" PRId64 ", startTime: %" PRId64 ".", mpdUrl.c_str(), startPosition, startTime);

        dp2p_assert(controlLogic->getContour().empty());
        Control::startTime = startTime;
        stopTime = std::numeric_limits<int64_t>::max();
        curPos = StreamPosition();
        forcedEof = false;

#if 0
        // TODO: this is a work-around. actually, when in paused, when want to keep the connection open all the time. however, we observe strange things like sending out a request on an intact TCP connection but going to close while waiting for header. need to investigate into that.
        DBGMSG("Initializing DashHttp module(s).");
        unsigned maxPendingRequests = 0;
        if(controlType == ControlType_MH) {
            delete httpPrimary;
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getPrimaryIf(), maxPendingRequests, callbackForDownloadedData);
            delete httpSecondary;
            httpSecondary = new DashHttp(splittedMpdUrl.hostName, dynamic_cast<ControlLogicMH*>(controlLogic)->getSecondaryIf(), maxPendingRequests, callbackForDownloadedData);
        } else {
            delete httpPrimary;
            httpPrimary = new DashHttp(splittedMpdUrl.hostName, IfData(), maxPendingRequests, callbackForDownloadedData);
            httpSecondary = NULL;
        }
        DBGMSG("DashHttp module initialized.");
#endif

        ThreadAdapter::mutexLock(&eventsMutex);
        events.push_back(new ControlLogicEventResumePlayback(startPosition));
        uint64_t buf = 1;
        dp2p_assert(8 == write(fdEvents, &buf, 8));
        ThreadAdapter::mutexUnlock(&eventsMutex);
    }

    state = ControlState_Playing;

    ThreadAdapter::mutexUnlock(&mutex);
}
#endif

bool Control::processAction(const ControlLogicAction& _a)
{
    DBGMSG("Action: %s.", _a.toString().c_str());

    bool result = false;

    switch(_a.getType())
    {

    case Action_CloseTcpConnection: {
    	const ControlLogicActionCloseTcpConnection& a = dynamic_cast<const ControlLogicActionCloseTcpConnection&>(_a);
    	result = processActionCloseTcpConnection(a);
    	break;
    }

    case Action_CreateTcpConnection: {
        const ControlLogicActionOpenTcpConnection& a = dynamic_cast<const ControlLogicActionOpenTcpConnection&>(_a);
        result = processActionOpenTcpConnection(a);
        break;
    }

    case Action_StartDownload: {
        const ControlLogicActionStartDownload& a = dynamic_cast<const ControlLogicActionStartDownload&>(_a);
        result = processActionStartDownload(a);
        break;
    }

    default:
        dp2p_assert(0);
        break;
    }

    DBGMSG("Return %s.", result ? "OK" : "NOK");
    return result;
}

bool Control::processActionCloseTcpConnection(const ControlLogicActionCloseTcpConnection& a)
{
	closeConnection(a.tcpConnectionId);
	return true;
}

bool Control::processActionOpenTcpConnection(const ControlLogicActionOpenTcpConnection& a)
{
	DashHttp* http = new DashHttp(a.tcpConnectionId, httpCb);
	dp2p_assert(httpMap.insert(pair<int, DashHttp*>(a.tcpConnectionId, http)).second);
	return true;
}

bool Control::processActionStartDownload(const ControlLogicActionStartDownload& a)
{
	dp2p_assert(httpMap.find(a.tcpConnectionId) != httpMap.end());

	DBGMSG("Processing action StartDownload with %d requests.", a.contentIds.size());

	const TcpConnection& tc = TcpConnectionManager::get(a.tcpConnectionId);
	//const SourceData& sd = SourceManager::get(tc.srcId);

	list<int> reqs;
	list<const ContentId*>::const_iterator it = a.contentIds.begin();
	list<dashp2p::URL>::const_iterator jt = a.urls.begin();
	list<HttpMethod>::const_iterator kt = a.httpMethods.begin();
	while(it != a.contentIds.end())
	{
		dp2p_assert(jt != a.urls.end());

		const int reqId = HttpRequestManager::newHttpRequest(a.tcpConnectionId, (*it)->copy(), /*jt->hostName,*/ jt->withoutHostname, true, *kt);
		//dp2p_assert(requestMap.insert(pair<ReqId, pair<ContentIdSegment, HttpMethod> >(req->reqId, pair<ContentIdSegment, HttpMethod>(*it, HttpMethod_HEAD))).second == true);
		reqs.push_back(reqId);

		if((*it)->getType() == ContentType_Mpd)
			Statistics::recordScalarDouble("startDownloadMPD", dashp2p::Utilities::now());

		DBGMSG("Starting request nr. %d: %s over interface %s.\n", reqId, (*it)->toString().c_str(),
				tc.getIfString().c_str());

		++it; ++jt; ++kt;
	}

	/* Register the request with the downloader */
	if(httpMap.at(a.tcpConnectionId)->newRequest(reqs))
	{
		/* Over output if segment */
		if(a.contentIds.front()->getType() == ContentType_Segment)
		{
			const ContentIdSegment& segId = dynamic_cast<const ContentIdSegment&>(*a.contentIds.front());
//#ifdef __ANDROID__
//			if(!withHandover)
//				DisplayHandover::displayOverlay(0, "%5s: % 8.3f bps", "Dwnld", segId.repId() / 1e6);
//#else
			OverlayAdapter::print(0, "%5s: % 8.3f bps", "Dwnld", segId.bitRate() / 1e6);
#if WITH_OVERLAY_STATISTICS == 1
			{
			    static FILE* f = NULL;
			    if(!f) f = fopen("/tmp/dp2p_dwnld.txt", "w");
			    fprintf(f, "%8.2f %8.2f\n", dashp2p::Utilities::now(), segId.bitRate() / 1e6);
			    fflush(f);
			}
#endif
//#endif
		}
		return true;
	}
	else
	{
		/* request was rejected (potentially disconnected) */
		return false;
	}
}

void Control::resumePlayback()
{
    DBGMSG("Enter resumePlayback().");

    const StreamPosition nextPos = getNextPosition();
    if(!storage->dataAvailable(nextPos)) {
        ERRMSG("Bug?");
        DBGMSG("Current position: %s.", curPos.toString().c_str());
        DBGMSG("Downloaded data:\n%s", storage->printDownloadedData(0, 0).c_str());
        exit(1);
    }

    /*if(!startTimeCrossed) {
        const int64_t absNow = dashp2p::Utilities::getAbsTime();
        if(absNow < startTime - startTimeTolerance) {
            return;
        } else {
            startTimeCrossed = true;
        }
    }*/
    ThreadAdapter::condVarSignal(&playbackPausedCondvar);

    DBGMSG("Return from resumePlayback().");
}

StreamPosition Control::getNextPosition()
{
    //DBGMSG("Enter. We are at position: (%d, %d, %" PRId64 ").", curPos.segId.repId(), curPos.segId.segNr(), curPos.byte);

    if(!curPos.valid()) {
        dp2p_assert(!controlLogic->getContour().empty());
        ContentIdSegment startSegId = controlLogic->getContour().getStart();
        dp2p_assert(startSegId.segmentIndex() == 0 || startSegId.segmentIndex() == controlLogic->getStartSegment()); // we might start directly with the start segment after a pausing
        StreamPosition startPosition(startSegId, 0);
        return startPosition;
    }

    if(curPos.byte < storage->getTotalSize(curPos.segId) - 1) {
        StreamPosition nextPos(curPos);
        nextPos.byte = curPos.byte + 1;
        return nextPos;
    }

    return StreamPosition(controlLogic->getContour().getNext(curPos.segId), 0);
}

bool Control::eof()
{
	if(controlLogic->getMpdWrapper() == NULL) {
		return false;
	} else if(forcedEof) {
		return true;
	} else if(controlLogic->getStopSegment() > 0 && curPos.segId.segmentIndex() == controlLogic->getStopSegment() && curPos.byte == storage->getTotalSize(curPos.segId) - 1) {
		return true;
	}
	return false;
}

void Control::closeConnection(const ConnectionId& connId)
{
	delete httpMap.at(connId);
	dp2p_assert(1 == httpMap.erase(connId));
}

}

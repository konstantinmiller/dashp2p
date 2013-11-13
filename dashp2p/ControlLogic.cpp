/****************************************************************************
 * ControlLogic.cpp                                                         *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
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


#include "ContentId.h"
#include "ControlLogic.h"
#include "ControlLogicAction.h"
#include "HttpRequestManager.h"
#include "SegmentStorage.h"
#include "Statistics.h"

#include <cassert>

namespace dashp2p {

ControlLogic::ControlLogic(int width, int height)
  : state(NO_MPD),
    mutex(),
    width(width),
    height(height),
    bitRates(),
    ifData(),
    contour(),
    //mpdWrapper(nullptr),
    //mpdDataField(nullptr),
    pendingActions()
{
	ThreadAdapter::mutexInit(&mutex);
}

ControlLogic::~ControlLogic()
{
	DBGMSG("Clean-up. Have %d pending actions.", pendingActions.size());
	ThreadAdapter::mutexDestroy(&mutex);
	//delete mpdWrapper;
	//delete mpdDataField;
	while(!pendingActions.empty()) {
		delete pendingActions.front();
		pendingActions.pop_front();
	}
}

list<ControlLogicAction*> ControlLogic::processEvent(ControlLogicEvent* e)
{
	dp2p_assert(e);

    DBGMSG("Event: %s.", e->toString().c_str());
    ThreadAdapter::mutexLock(&mutex);

    /* Return object */
    ActionList actions;

    /* We are done -> no actions. */
    if(state == DONE) {
        DBGMSG("State: DONE.");
        delete e;
        ThreadAdapter::mutexUnlock(&mutex);
        return actions;
    }

    /* Act depending on received event */
    switch(e->getType())
    {

    /*case Event_Connected: {
    	const ControlLogicEventConnected& event = dynamic_cast<const ControlLogicEventConnected&>(*e);
    	actions = processEventConnected(event);
    	break;
    }*/

    case Event_DataPlayed: {
    	dp2p_assert(state == HAVE_MPD);
    	const ControlLogicEventDataPlayed& event = dynamic_cast<const ControlLogicEventDataPlayed&>(*e);
    	actions = processEventDataPlayed(event);
    	break;
    }

    case Event_DataReceived: {
    	ControlLogicEventDataReceived& event = dynamic_cast<ControlLogicEventDataReceived&>(*e);
    	actions = processEventDataReceived(event);
    	break;
    }

    case Event_Disconnect: {
    	const ControlLogicEventDisconnect& event = dynamic_cast<const ControlLogicEventDisconnect&>(*e);
    	actions = processEventDisconnect(event);
    	break;
    }

    /*case Event_Pause: {
    	dp2p_assert(state == HAVE_MPD);
    	const ControlLogicEventPause& event = dynamic_cast<const ControlLogicEventPause&>(*e);
    	actions = processEventPause(event);
    	break;
    }

    case Event_ResumePlayback: {
        dp2p_assert(state == HAVE_MPD);
        const ControlLogicEventResumePlayback& event = dynamic_cast<const ControlLogicEventResumePlayback&>(*e);
        actions = processEventResumePlayback(event);
        break;
    }*/

    case Event_StartPlayback: {
    	const ControlLogicEventStartPlayback& event = dynamic_cast<const ControlLogicEventStartPlayback&>(*e);
    	actions = processEventStartPlayback(event);
    	break;
    }

    default:
        THROW_RUNTIME("Got unexpected event: %s.", e->toString().c_str());
    }

    delete e;

    for(ActionList::const_iterator it = actions.begin(); it != actions.end(); ++it) {
    	pendingActions.push_back((*it)->copy());
    }

    ThreadAdapter::mutexUnlock(&mutex);
    DBGMSG("Returning %d actions. Pending actions: %d.", actions.size(), pendingActions.size());
    return actions;
}

#if 0
list<ControlLogicAction*> ControlLogic::actionRejected(ControlLogicAction* a)
{
	dp2p_assert(a);

    DBGMSG("Action: %s.", a->toString().c_str());
    ThreadAdapter::mutexLock(&mutex);
    //DBGMSG("Mutex locked.");

    /* Return object */
    ActionList actions;

    /* We are done -> no actions. */
    if(state == DONE) {
        DBGMSG("State: DONE.");
        delete a;
        ThreadAdapter::mutexUnlock(&mutex);
        return actions;
    }

    /* Act depending on received action */
    switch(a->getType())
    {

    case Action_StartDownload: {
    	ControlLogicActionStartDownload* action = dynamic_cast<ControlLogicActionStartDownload*>(a);
    	actions = actionRejectedStartDownload(action);
    	break;
    }

    default:
        ERRMSG("Unsupported rejected action type.");
        abort();
        break;
    }

    for(ActionList::const_iterator it = actions.begin(); it != actions.end(); ++it) {
    	pendingActions.push_back((*it)->copy());
    }

    ThreadAdapter::mutexUnlock(&mutex);
    return actions;
}
#endif

int ControlLogic::getStartSegment() const
{
	//const int periodIndex = 0;
	//const int adaptationSetIndex = 0;
	//const int representationIndex = 0;

	//int ret = 1 + startPosition / mpdWrapper->getNominalSegmentDuration(periodIndex, adaptationSetIndex, representationIndex);
	//dp2p_assert(ret > 0 && ret < mpdWrapper->getNumSegments(periodIndex, adaptationSetIndex, representationIndex));
	return 1;
}

int ControlLogic::getStopSegment() const
{
	const int periodIndex = 0;
	const int adaptationSetIndex = 0;
	const int representationIndex = 0;

	int ret;
	//if(stopPosition == 0)
		ret = MpdWrapper::getNumSegments(periodIndex, adaptationSetIndex, representationIndex) - 1;
	//else
	//	ret = ((stopPosition % mpdWrapper->getNominalSegmentDuration(periodIndex, adaptationSetIndex, representationIndex) == 0) ? 0 : 1) + stopPosition / mpdWrapper->getNominalSegmentDuration(periodIndex, adaptationSetIndex, representationIndex);
	dp2p_assert(ret > 0 && ret < MpdWrapper::getNumSegments(periodIndex, adaptationSetIndex, representationIndex));
	return ret;
}

list<ControlLogicAction*> ControlLogic::processEventDataReceived(ControlLogicEventDataReceived& e)
{
	switch(HttpRequestManager::getContentType(e.reqId))
	{

	case ContentType_Mpd:
		dp2p_assert(state == NO_MPD);
		dp2p_assert(contour.empty());
		return processEventDataReceivedMpd(e);

	/*case ContentType_MpdPeer:
		dp2p_assert(state == HAVE_MPD);
		return processEventDataReceivedMpdPeer(e);*/

	case ContentType_Segment:
		dp2p_assert(state == HAVE_MPD);
		dp2p_assert(!contour.empty());
		return processEventDataReceivedSegment(e);

	/*case ContentType_Tracker:
		dp2p_assert(state == HAVE_MPD);
		return processEventDataReceivedTracker(e);*/

	default:
		ERRMSG("Unknown content type.");
		abort();
		break;
	}

	return list<ControlLogicAction*>();
}

#if 0
bool ControlLogic::ackActionConnected (const ConnectionId& connId)
{
	int ret = 0;
	for(ActionList::iterator it = pendingActions.begin(); it != pendingActions.end(); )
	{
		ControlLogicAction* _a = *it;
		if(_a->getType() == Action_CreateTcpConnection) {
			ControlLogicActionOpenTcpConnection* a = dynamic_cast<ControlLogicActionOpenTcpConnection*>(_a);
			if(a->tcpConnectionId == connId) {
				++ret;
				delete a;
				ActionList::iterator jt = it;
				++it;
				pendingActions.erase(jt);
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
	DBGMSG("Removed %d actions from pending actions list. Pending: %d.", ret, pendingActions.size());
	return ret;
}
#endif

bool ControlLogic::ackActionRequestCompleted (const ContentId& contentId)
{
	int ret = 0;
	int actionCount = 0;
	for(ActionList::iterator it = pendingActions.begin(); it != pendingActions.end(); )
	{
		ControlLogicAction* _a = *it;
		if(_a->getType() == Action_StartDownload)
		{
			ControlLogicActionStartDownload* a = dynamic_cast<ControlLogicActionStartDownload*>(_a);
			for(list<const ContentId*>::iterator kt = a->contentIds.begin(); kt != a->contentIds.end();) {
				if(**kt == contentId) {
					++ret;
					delete (*kt);
					list<const ContentId*>::iterator lt = kt;
					++kt;
					a->contentIds.erase(lt);
				} else {
					++kt;
				}
			}

			if(a->contentIds.empty()) {
				delete a;
				++actionCount;
				ActionList::iterator jt = it;
				++it;
				pendingActions.erase(jt);
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
	DBGMSG("Removed %d downloads. Removed %d actions from pending actions list. Pending: %d.", ret, actionCount, pendingActions.size());
	return ret;
}

/*bool ControlLogic::ackActionDisconnect (const TcpConnectionId& tcpConnectionId)
{
	int ret = 0;
	for(ActionList::iterator it = pendingActions.begin(); it != pendingActions.end(); )
	{
		ControlLogicAction* _a = *it;
		if(_a->getType() == Action_CloseTcpConnection) {
			ControlLogicActionCloseTcpConnection* a = dynamic_cast<ControlLogicActionCloseTcpConnection*>(_a);
			if(a->tcpConnectionId == tcpConnectionId) {
				++ret;
				delete a;
				ActionList::iterator jt = it;
				++it;
				pendingActions.erase(jt);
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
	DBGMSG("Removed %d actions from pending actions list. Pending: %d.", ret, pendingActions.size());
	return ret;
}*/

unsigned ControlLogic::getIndex(int bitrate)
{
    for(unsigned i = 0; i < bitRates.size(); ++i) {
        if(bitRates.at(i) == bitrate) {
            return i;
        }
    }
    dp2p_assert(false);
    return 0;
}

void ControlLogic::processEventDataReceivedMpd_Completed(const ContentIdMpd& contentIdMpd)
{
	//dp2p_assert(mpdDataField->full());

#if 0
	FILE* f = fopen("dbg_mpd.txt", "w");
	const char* tmpBuf = mpdDataField->getCopy();
	fprintf(f, "%.*s", (int)mpdDataField->getOccupiedSize(), tmpBuf);
	delete [] tmpBuf;
	fclose(f);
#endif

	/* Logging. */
	Statistics::recordScalarD64("completedDownloadMPD", dashp2p::Utilities::getTime());

	/* Parse MPD */
	//MpdWrapper::init(mpdDataField->getCopy((char*)malloc(mpdDataField->getReservedSize() * sizeof(char)), mpdDataField->getReservedSize()), mpdDataField->getReservedSize()); // we reserve this memory with malloc since it will be freed by the VLC XML plugin which uses free()
	MpdWrapper::init(contentIdMpd);
	//delete mpdDataField;
	//mpdDataField = nullptr;

	/* Logging. */
	DBGMSG("Parsed MPD file.");
	Statistics::recordScalarD64("parsedMPD", dashp2p::Utilities::getTime());
	Statistics::recordScalarDouble("videoDuration", MpdWrapper::getVideoDuration() / 1e6);

	/* Give MPD to the Statistics module */
	//Statistics::setMpdWrapper(mpdWrapper);

	INFOMSG("MPD file downloaded and parsed.");
	state = HAVE_MPD;

	/* Show the list of available spatial resolutions. */
	const int periodIndex = 0;
	const int adaptationSetIndex = 0;
	const int representationIndex = 0;
	std::string resolutions = MpdWrapper::printSpatialResolutions(periodIndex, adaptationSetIndex);
	INFOMSG("Available spatial resolutions:\n%s.", resolutions.c_str());
	INFOMSG("Segment duration: %.3f.", MpdWrapper::getNominalSegmentDuration(periodIndex, adaptationSetIndex, representationIndex) / 1e6);

	/* Check the requested spatial resolution */
	if(width == -1)
	{
		dp2p_assert(width == height);
		pair<int,int> newWidthHeight = MpdWrapper::getLowestSpatialResolution(periodIndex, adaptationSetIndex);
		INFOMSG("Lowest spatial resolution requested. Selecting: %d x %d.", newWidthHeight.first, newWidthHeight.second);
		width = newWidthHeight.first;
		height = newWidthHeight.second;
	}
	else if(width == 1)
	{
		dp2p_assert(width == height);
		pair<int,int> newWidthHeight = MpdWrapper::getHighestSpatialResolution(periodIndex, adaptationSetIndex);
		INFOMSG("Highest spatial resolution requested. Selecting: %d x %d.", newWidthHeight.first, newWidthHeight.second);
		width = newWidthHeight.first;
		height = newWidthHeight.second;
	}
	else if(width == 0)
	{
		dp2p_assert(width == height);
		INFOMSG("Requested to dynamically switch spatial resolutions. Hopefully, your decoder supports that!");
	}
	else
	{
		INFOMSG("Requested fixed spatial resolution: %d x %d.", width, height);
	}

	if(MpdWrapper::getNumRepresentations(periodIndex, adaptationSetIndex, width, height) == 0) {
		ERRMSG("Requested screen size of %u x %u is not supported by the stream.", width, height);
		abort();
	}

	/* Sanity checks and debug output */
	int startSegment = getStartSegment();
	int stopSegment = getStopSegment();
	dp2p_assert(stopSegment > 0 && stopSegment < MpdWrapper::getNumSegments(periodIndex, adaptationSetIndex, representationIndex));
	DBGMSG("Setting startSegment: %d, stopSegment: %d.", startSegment, stopSegment);
}

ControlLogicAction* ControlLogic::createActionDownloadSegments(list<const ContentId*> segIds, const TcpConnectionId& tcpConnectionId, HttpMethod httpMethod) const
{
	list<dashp2p::URL> urls;
	list<HttpMethod> httpMethods;
	for(list<const ContentId*>::iterator it = segIds.begin(); it != segIds.end(); ++it)
	{
	    const ContentIdSegment& segId = *dynamic_cast<const ContentIdSegment*>(*it);

		//DBGMSG("%s", (*it)->toString().c_str());
		urls.push_back(dashp2p::Utilities::splitURL(MpdWrapper::getSegmentURL(segId)));
		httpMethods.push_back(httpMethod);

		if(!SegmentStorage::initialized(segId)) {
		    DBGMSG("Segment not yet available in the storage. Initializing.");
		    SegmentStorage::initSegment(segId, -1, MpdWrapper::getSegmentDuration(segId));
		} else {
		    DBGMSG("Segment aleady registered in the storage module.");
		}
	}
	return new ControlLogicActionStartDownload(tcpConnectionId, segIds, urls, httpMethods);
}

}

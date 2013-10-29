/****************************************************************************
 * ControlLogicP2P.cpp                                                      *
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Dashp2pTypes.h"
#include "ControlLogicP2P.h"
#include "ControlLogicActionCreateTcpConnection.h"
#include "ControlLogicActionStartDownload.h"
#include "ControlLogicEventMpdReceived.h"
#include "ControlLogicEventDataReceived.h"
#include "ControlLogicEventDataPlayed.h"
#include "ControlLogicEventResumePlayback.h"
#include "ControlLogicEventStartPlayback.h"
#include "Statistics.h"
#include "OverlayAdapter.h"
#include "DebugAdapter.h"

#include <stdio.h>
#include <assert.h>
#include <utility>
#include <limits>
using std::numeric_limits;


ControlLogicP2P::ControlLogicP2P(const std::string& /*config*/)
  : ControlLogic()
{

	/* Parse configuration string if you want to pass some parameters from the command line */
	// TODO:
}

ControlLogicP2P::~ControlLogicP2P()
{
    // TODO:
}

list<ControlLogicAction*> ControlLogicP2P::processEvent(const ControlLogicEvent* e)
{
	DBGMSG("Enter.");
	ThreadAdapter::mutexLock(&mutex);
	DBGMSG("Mutex locked.");

    dp2p_assert(e);

    /* Return object */
    list<ControlLogicAction*> actions;

    /* We are done -> no actions. */
    if(state == DONE) {
        DBGMSG("State: DONE.");
        delete e;
        DBGMSG("Return.");
        return actions;
    }

    /* Act depending on received event */
    switch(e->getType()) {

    case Event_StartPlayback:
    {
        const ControlLogicEventStartPlayback& event = dynamic_cast<const ControlLogicEventStartPlayback&>(*e);
        const dash::URL mpdUrl = event.mpdUrl;

        const int connectionId = 0;

        /* Create a new TCP connection */
        actions.push_back(new ControlLogicActionCreateTcpConnection(connectionId, IfData(), mpdUrl.hostName, 0));

        /* Start MPD download */
        list<const ContentId*> contentIds(1, new ContentIdSegment(0, -1));
        actions.push_back(createActionDownloadSegments(contentIds, connectionId, HttpMethod_GET));

        break;
    }

    case Event_MpdReceived:
    {
        DBGMSG("Event: mpd received.");
        INFOMSG("[%.3fs] MPD file downloaded and parsed.");

        dp2p_assert(state == NO_MPD);
        dp2p_assert(contour.empty());
        state = HAVE_MPD;

        const ControlLogicEventMpdReceived& event = dynamic_cast<const ControlLogicEventMpdReceived&>(*e);
        representations = event.representations;
        startSegment = event.startSegment;
        stopSegment = event.stopSegment;

        const unsigned lowestBitrate = representations.at(0).bandwidth;

        /* Download the initiallization and the first segment at lowest quality, pipelined */
        list<const ContentId*> segIds;
        segIds.push_back(new ContentIdSegment(lowestBitrate, 0));
        segIds.push_back(new ContentIdSegment(lowestBitrate, startSegment));
        list<dash::URL> urls;
        list<HttpMethod> httpMethods;
        for(list<const ContentId*>::const_iterator it = segIds.begin(); it != segIds.end(); ++it) {
        	urls.push_back(dash::Utilities::splitURL(mpdWrapper->getSegmentURL(dynamic_cast<const ContentIdSegment&>(**it))));
        	httpMethods.push_back(HttpMethod_GET);
        	contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
        }
        actions.push_back(new ControlLogicActionStartDownload(0, segIds, urls, httpMethods));
    }
    break;

    case Event_DataReceived:
    {
        DBGMSG("Event: data received.");

        dp2p_assert(state == HAVE_MPD);

        //const ControlLogicEventDataReceived& event = dynamic_cast<const ControlLogicEventDataReceived&>(*e);
    }
    break;

    case Event_DataPlayed:
    {
        DBGMSG("Event: data played.");

        dp2p_assert(state == HAVE_MPD);

        //const ControlLogicEventDataPlayed& event = dynamic_cast<const ControlLogicEventDataPlayed&>(*e);
    }
    break;

    case Event_ResumePlayback:
    {
        DBGMSG("Event: resume playback.");

        dp2p_assert(state == HAVE_MPD);
        dp2p_assert(contour.empty());
    }
    break;

    default:
        dp2p_assert(0);
        break;
    }

    delete e;
    DBGMSG("Return.");
    return actions;
}

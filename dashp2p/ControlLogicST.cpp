/****************************************************************************
 * ControlLogicST.cpp                                                       *
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

/**
 * Performs the adaptation as in
 *     "Adaptation Algorithm for Adaptive Streaming over HTTP",
 *     K Miller, E Quacchio, G Gennari, A Wolisz,
 *     IEEE 19th International Packet Video Workshop (PV 2012),
 *     May 2012, Munich, Germany
 *
 * The adaptation algorithm is subject to a pending patent application by STMicroelectronics:
 *     "Media-quality adaptation mechanisms for dynamic adaptive streaming"
 *     Inventors: Konstantin Miller, Emanuele Quacchio,
 *     Publication date: 2013/2/25
 *     Patent office: US
 *     Application number: 13/775,885 **/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

//#include "Dashp2pTypes.h"
#include "ContentId.h"
#include "ControlLogicST.h"
#include "ControlLogicAction.h"
#include "Statistics.h"
#include "OverlayAdapter.h"
#include "DebugAdapter.h"
#include "HttpRequestManager.h"
#include "TcpConnectionManager.h"
#include "SourceManager.h"

#include <cstdio>
#include <cassert>
#include <utility>
#include <limits>
using std::numeric_limits;

// TODO: improve upswitching responsiveness after bandwidth increase
// TODO: switch to higher bit-rate even if the buffer level is low if the throughput is high enough and stable for a long time!
// TODO: if bandwidth increases, start to download higher bit-rate in parallel, later discard buffer content
// TODO: take throughput more into account. e.g., if buffer level high enough but Bmin relatively low (seconds), then it might be too late to switch to lower or lowest bandwidth when beta crosses Bmin. if no throughput, have to switch down faster.
// TODO: at the moment, if we don't manage to ramp up to appropriate bit-rate during initial increase, it takes too long to adapt. fix that.

namespace dashp2p {

ControlLogicST::ControlLogicST(int width, int height, const std::string& config)
  : ControlLogic(width, height),
    Bmin(0),
    Blow(0),
    Bhigh(0),
    alfa1(0),
    alfa2(0),
    alfa3(0),
    alfa4(0),
    alfa5(0),
    Delta_t(0),
    betaTimeSeries(NULL),
    initialIncrease(true),
    initialIncreaseTerminationTime(0),
    Bdelay(numeric_limits<int64_t>::max()),
    delayedRequests(),
    connId(-1),
    mpdUrl()
{

    if(10 != sscanf(config.c_str(), "%lf:%lf:%lf:%lf:%lf:%lf:%lf:%lf:%lf:%d", &Bmin, &Blow, &Bhigh, &alfa1, &alfa2, &alfa3, &alfa4, &alfa5, &Delta_t, &fetchHeads)) {
        ERRMSG("ControlLogicST module could not parse the configuration string \"%s\".", config.c_str());
        dp2p_assert(0);
    }

    betaTimeSeries = new TimeSeries<int64_t>(1000000, false, true);

    Statistics::recordScalarDouble("Bmin", Bmin);
    Statistics::recordScalarDouble("Blow", Blow);
    Statistics::recordScalarDouble("Bhigh", Bhigh);
    Statistics::recordScalarDouble("alfa1", alfa1);
    Statistics::recordScalarDouble("alfa2", alfa2);
    Statistics::recordScalarDouble("alfa3", alfa3);
    Statistics::recordScalarDouble("alfa4", alfa4);
    Statistics::recordScalarDouble("alfa5", alfa5);
    Statistics::recordScalarDouble("Delta_t", Delta_t);
    Statistics::recordScalarDouble("fetchHeads", fetchHeads);
}

ControlLogicST::~ControlLogicST()
{
    delete betaTimeSeries; betaTimeSeries = NULL;
}

list<ControlLogicAction*> ControlLogicST::processEventConnected(const ControlLogicEventConnected& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	dp2p_assert(e.connId == connId);

	list<ControlLogicAction*> actions;

	ackActionConnected(e.connId);

	return actions;
}

list<ControlLogicAction*> ControlLogicST::processEventDataPlayed(const ControlLogicEventDataPlayed& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	/* Calculate if beta_min is increasing. */
	betaTimeSeries->pushBack(dashp2p::Utilities::getTime(), e.availableContigInterval.first);

	if(!delayedRequests.empty() && e.availableContigInterval.first <= Bdelay)
	{
		INFOMSG("[%.3fs] beta <= Bdelay (%.3g <= %.3g). Release %d delayed request(s).", dashp2p::Utilities::now(), e.availableContigInterval.first / 1e6, Bdelay / 1e6, delayedRequests.size());
		dp2p_assert(delayedRequests.size() == 1);
		actions.push_back(createActionDownloadSegments(delayedRequests, connId, HttpMethod_GET));
		delayedRequests.clear();
		Bdelay = numeric_limits<int64_t>::max();
	}

	return actions;
}

list<ControlLogicAction*> ControlLogicST::processEventDataReceivedMpd(ControlLogicEventDataReceived& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	if(mpdDataField == NULL) {
		dp2p_assert(e.byteFrom == 0);
		mpdDataField = new DataField(HttpRequestManager::getContentLength(e.reqId));
		dp2p_assert(mpdDataField);
	}

	dp2p_assert(!mpdDataField->full());

	mpdDataField->setData(e.byteFrom, e.byteTo, HttpRequestManager::getPldBytes(e.reqId) + e.byteFrom, false);

	if(mpdDataField->full()) {
		processEventDataReceivedMpd_Completed();
		ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId));
		Statistics::recordRequestStatistics(e.connId, e.reqId);
	} else {
		return actions;
	}

	const int periodIndex = 0;
	const int adaptationSetIndex = 0;

	bitRates = mpdWrapper->getBitrates(periodIndex, adaptationSetIndex, width, height);
	const int startSegment = getStartSegment();
	const int stopSegment = getStopSegment();

	const unsigned lowestBitrate = bitRates.at(0);

	/* Fetch HEADs of the segments to get segment sizes. */
	if(fetchHeads)
	{
		// TODO: this is an extension to the original ST algo
		list<const ContentId*> segIdsHeads;
		for(unsigned i = 0; i < bitRates.size(); ++i) {
			const int bitRate = bitRates.at(i);
			segIdsHeads.push_back(new ContentIdSegment(periodIndex, adaptationSetIndex, bitRate, 0));
			for(int segNr = startSegment; segNr <= stopSegment ; ++segNr) {
				segIdsHeads.push_back(new ContentIdSegment(periodIndex, adaptationSetIndex, bitRate, segNr));
			}
		}
		actions.push_back(this->createActionDownloadSegments(segIdsHeads, connId, HttpMethod_HEAD));
	}

	/* Download the initiallization and the first segment at lowest quality, pipelined */
	list<const ContentId*> segIds;
	segIds.push_back(new ContentIdSegment(periodIndex, adaptationSetIndex, lowestBitrate, 0));
	segIds.push_back(new ContentIdSegment(periodIndex, adaptationSetIndex, lowestBitrate, startSegment));

	for(list<const ContentId*>::const_iterator it = segIds.begin(); it != segIds.end(); ++it)
	{
		//ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
		//updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
		//updateContour->segId = *it;
		//actions.push_back(updateContour);
		contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
	}

	actions.push_back(this->createActionDownloadSegments(segIds, connId, HttpMethod_GET));

	return actions;
}

list<ControlLogicAction*> ControlLogicST::processEventDataReceivedSegment(ControlLogicEventDataReceived& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	/* Return object */
	list<ControlLogicAction*> actions;

	const ContentIdSegment& segId = dynamic_cast<const ContentIdSegment&>(HttpRequestManager::getContentId(e.reqId));

	/* We do not start a new download if (i) the last one is not finished yet, or (ii) we have already downloading the stop segment,
	 * or (iii) we downloaded the initial segment (since we have aready requested initial segment and start segment pipelined) */
	if(e.byteTo != HttpRequestManager::getContentLength(e.reqId) - 1) {
		DBGMSG("Segment not ready yet. No action required.");
		return actions;
	}

	DBGMSG("Segment completed. Processing.");

	dp2p_assert(ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId)));
	dp2p_assert(delayedRequests.empty());

	/* Give the HttpRequest object to the Statistics module. It will delete it later. */
	Statistics::recordRequestStatistics(e.connId, e.reqId);

	betaTimeSeries->pushBack(dashp2p::Utilities::getTime(), e.availableContigInterval.first);

	if (segId.segmentIndex() == getStopSegment()) {
		DBGMSG("Stop segment. No action required.");
		return actions;
	} else if (segId.segmentIndex() == 0) {
		DBGMSG("Init segment. No action required.");
		return actions;
	}

	/* select bit-rate */
	const bool ifBetaMinIncreasing = betaTimeSeries->minIncreasing();
	const int64_t beta = e.availableContigInterval.first;
	// TODO: in the next two lines we need to use the proper connection ID!
	const double rho = Statistics::getThroughput(e.connId, std::min<double>(Delta_t, dashp2p::Utilities::now()));
	const double rhoLast = Statistics::getThroughputLastRequest(e.connId);
	const unsigned completedRequests = (segId.segmentIndex() == 0) ? 1 : segId.segmentIndex() - getStartSegment() + 2;
	Decision adaptationDecision = selectRepresentation(
			ifBetaMinIncreasing,
			beta / 1e6,
			rho,
			rhoLast,
			completedRequests,
			segId);
	//std::pair<Representation, double> repAndDelay = AdaptationST::selectRepresentationAlwaysLowest();
	//std::pair<Representation, double> repAndDelay = AdaptationST::selectRepresentation("0");

	Bdelay = adaptationDecision.Bdelay;
	dp2p_assert(Bdelay > 0);
	const int r_new = adaptationDecision.bitRate;

	const int periodIndex = 0;
	const int adaptationSetIndex = 0;
	const ContentIdSegment* segNext = new ContentIdSegment(periodIndex, adaptationSetIndex, r_new, segId.segmentIndex() + 1);
	DBGMSG("Will download segment Nr. %d (last one will be %d, segment 0 is initial segment.)", segNext->segmentIndex(), getStopSegment());

	DBGMSG("Selected bit-rate %.3f Mbit/sec. Delay download until beta == %.3f sec",
			segNext->bitRate() / 1e6, Bdelay / 1e6);

	/* log selected bit-rate */
	Statistics::recordAdaptationDecision(dashp2p::Utilities::now(), beta, rho, rhoLast, segId.bitRate(), r_new, Bdelay, ifBetaMinIncreasing, adaptationDecision.reason);

	/* Update contour */
	//ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
	//updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
	//updateContour->segId = segNext;
	//actions.push_back(updateContour);
	contour.setNext(*segNext);

	/* Either request the next segment immediately or save it in delayedRequests */
	if(e.availableContigInterval.first <= Bdelay) {
		list<const ContentId*> contentIds;
		contentIds.push_back(segNext);
		actions.push_back(this->createActionDownloadSegments(contentIds, connId, HttpMethod_GET));
	} else {
		delayedRequests.push_back(segNext);
	}

	return actions;
}

#if 0
list<ControlLogicAction*> ControlLogicST::processEventDisconnect(const ControlLogicEventDisconnect& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	dp2p_assert_v(e.connId == connId, "e.connId: %d, connId: %d", e.connId, connId);

	list<ControlLogicAction*> actions;

	if(0 == ackActionDisconnect(e.connId)) {
		actions.push_back(new ControlLogicActionCloseTcpConnection(connId));
	}

	connId = Statistics::registerTcpConnection();

	actions.push_back(new ControlLogicActionCreateTcpConnection(connId, IfData(), mpdUrl.hostName, 0));

	list<const ContentId*> contentIds;
	for(list<int>::const_iterator it = e.reqs.begin(); it != e.reqs.end(); ++it)
		contentIds.push_back(HttpRequestManager::getContentId(*it).copy());
	if(!contentIds.empty()) {
		actions.push_back(createActionDownloadSegments(contentIds, connId, HttpMethod_GET));
	}

	return actions;
}
#endif

#if 0
list<ControlLogicAction*> ControlLogicST::processEventPause(const ControlLogicEventPause& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());
	contour.clear();
	return list<ControlLogicAction*>();
}
#endif

#if 0
list<ControlLogicAction*> ControlLogicST::processEventResumePlayback(const ControlLogicEventResumePlayback& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	dp2p_assert(contour.empty());

	delayedRequests.clear();

	//startPosition = e.startPosition;

	DBGMSG("Setting startSegment: %d, stopSegment: %d.", getStartSegment(), getStopSegment());

	const unsigned lowestBitrate = bitRates.at(0);

	/* Add the Initialization segment to Contour to make the decoder happy. */
	//ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
	//updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
	//updateContour->segId = SegId(lowestBitrate, 0);
	//actions.push_back(updateContour);
	const int periodIndex = 0;
	const int adaptationSetIndex = 0;
	contour.setNext(ContentIdSegment(periodIndex, adaptationSetIndex, lowestBitrate, 0));

	/* Download the first segment at lowest quality, pipelined */
	list<const ContentId*> segIds;
	segIds.push_back(new ContentIdSegment(periodIndex, adaptationSetIndex, lowestBitrate, getStartSegment()));

	for(list<const ContentId*>::const_iterator it = segIds.begin(); it != segIds.end(); ++it)
	{
		//ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
		//updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
		//updateContour->segId = *it;
		//actions.push_back(updateContour);
		contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
	}

	actions.push_back(this->createActionDownloadSegments(segIds, connId, HttpMethod_GET));

	return actions;
}
#endif

list<ControlLogicAction*> ControlLogicST::processEventStartPlayback(const ControlLogicEventStartPlayback& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	mpdUrl = e.mpdUrl;

	/* Create a new TCP connection */
	const int srcId = SourceManager::add(mpdUrl.hostName);
	connId = TcpConnectionManager::create(srcId);
	actions.push_back(new ControlLogicActionOpenTcpConnection(connId));

	/* Start MPD download */
	list<const ContentId*> contentIds(1, new ContentIdMpd);
	list<dashp2p::URL> urls(1, mpdUrl);
	list<HttpMethod> httpMethods(1, HttpMethod_GET);
	actions.push_back(new ControlLogicActionStartDownload(connId, contentIds, urls, httpMethods));

	return actions;
}

#if 0
list<ControlLogicAction*> ControlLogicST::actionRejectedStartDownload(ControlLogicActionStartDownload* a)
{
	const int numPendingActions = pendingActions.size();
	DBGMSG("Pending actions: %d. Rejected action: %s.", numPendingActions, a->toString().c_str());

	list<ControlLogicAction*> actions;

	/* Check the pending connections */
	bool connectionClosed = false;
	ConnectionId newConnId = -1;
	int earlierDownloads = 0;
	int laterDownloads = 0;
	bool ownFound = false;
	int cnt = 0;
	ActionList::iterator itOwn = pendingActions.end();
	for(ActionList::iterator it = pendingActions.begin(); it != pendingActions.end(); ++it)
	{
		DBGMSG("Pending action %d(%d): %s.", ++cnt, numPendingActions, (*it)->toString().c_str());

		switch ((*it)->getType()) {

		case Action_CloseTcpConnection: {
			const ControlLogicActionCloseTcpConnection& aa = dynamic_cast<const ControlLogicActionCloseTcpConnection&>(**it);
			if(aa.id == a->connId)
				connectionClosed = true;
			else if(aa.id == newConnId)
				newConnId = -1;
			break;
		}

		case Action_CreateTcpConnection: {
			const ControlLogicActionCreateTcpConnection& aa = dynamic_cast<const ControlLogicActionCreateTcpConnection&>(**it);
			newConnId = aa.id;
			break;
		}

		case Action_StartDownload: {
			const ControlLogicActionStartDownload& aa = dynamic_cast<const ControlLogicActionStartDownload&>(**it);
			if(*a == aa) {
				dp2p_assert(!ownFound);
				ownFound = true;
				itOwn = it;
			} else if(!ownFound) {
				++earlierDownloads;
			} else {
				++laterDownloads;
			}
			break;
		}

		default: ERRMSG("Unknown action type: %d", (*it)->getType()); abort(); break;

		}
	}

	dp2p_assert(ownFound && !earlierDownloads && !laterDownloads); // this is a simplification for this control logic, since it does all requests sequentially

	DBGMSG("connectionClosed: %s, newConnId: %d, earlierDownloads: %d, laterDownloads: %d, ownFound: %s",
			connectionClosed ? "yes" : "no", newConnId, earlierDownloads, laterDownloads, ownFound ? "yes" : "no");

	pendingActions.erase(itOwn);

	if(!connectionClosed) {
		actions.push_back(new ControlLogicActionCloseTcpConnection(a->connId));
	}

	if(newConnId != -1) {
		a->connId = newConnId;
		actions.push_back(a);
	} else {
		dp2p_assert(connId == a->connId);
		connId = Statistics::registerTcpConnection();
		actions.push_back(new ControlLogicActionCreateTcpConnection(connId, IfData(), mpdUrl.hostName, 0));
		a->connId = connId;
		actions.push_back(a);
	}

	return actions;
}
#endif

enum DecisionReason {
    II1_UP = 20,
    II1_KEEP = 30,
    II2_UP = 40,
    II2_KEEP = 50,
    II3_UP = 60,
    II3_KEEP = 70,
    NO0_LOWEST = 80,
    NO1_LOWEST = 90,
    NO1_KEEP = 100,
    NO1_DOWN = 110,
    NO2_DELAY = 120,
    NO2_NODELAY = 130,
    NO3_DELAY = 140,
    NO3_UP = 150
};

// TODO: use rhoLast instead of rho in the initial incrase phase to account for potentially significant throughput increase TCP gives you during the beggining of the transmission (rho would average over the last n seconds, which usually would cover more than just the last download)

ControlLogicST::Decision ControlLogicST::selectRepresentation(bool ifBetaMinIncreasing, double beta,
        double rho, double rhoLast, unsigned completedRequests, const ContentIdSegment& lastSegment)
{
    /* debug output */
    INFOMSG("Selecting representation. %s, beta: %.3g, rho: %.3g, rhoLast: %.3g, r_last: %.3g.",
            ifBetaMinIncreasing ? "beta_min incr." : "beta_min decr.", beta, rho / 1e6, rhoLast / 1e6, lastSegment.bitRate() / 1e6);

    /* current time since beginning of download */
    const int64_t now = dashp2p::Utilities::getTime();

    /* verify that we already obtained a list of representations */
    dp2p_assert(bitRates.size() && completedRequests > 1);

    /* Get indexes of some relevant representations for faster access. */
    const unsigned iLowest = 0;
    const unsigned iLast = getIndex(lastSegment.bitRate());
    const unsigned iHighest = bitRates.size() - 1;

    /* Are we in the initial increase phase? */
    if (initialIncrease)
    {
        DBGMSG("We are in the initial increase phase.");

        /* Check if we shall terminate the initial increase phase. */

        /* Check if we already selected highest representation. */
        if (initialIncrease && iLast == iHighest) {
            initialIncrease = false;
            INFOMSG("[%.3fs] Terminating initial increase phase. Reached highest representation.", dashp2p::Utilities::now());
        }

        /* Check if the discretized minimum buffer level had been decreasing. */
        if (initialIncrease && !ifBetaMinIncreasing) {
            initialIncrease = false;
            INFOMSG("[%.3fs] Terminating initial increase phase. Discretized min buffer level was decreasing: %s.", dashp2p::Utilities::now(), betaTimeSeries->printDiscretizedMin().c_str());
        }

        /* Check if current representation is above alfa1 * rho. */
        if (initialIncrease && bitRates.at(iLast) > alfa1 * rho) {
            INFOMSG("[%.3fs] Terminating initial increase phase. r_higher > alfa1 * rho (%.3f > %f * %.3f).", dashp2p::Utilities::now(),
            		bitRates.at(iLast) / 1e6, alfa1, rho / 1e6);
            initialIncrease = false;
        }

        /* if we didn't terminate the initial increase phase,
         * act depending on buffer level and throughput. */
        if (initialIncrease && beta < Bmin)
        {
            DBGMSG("We are in [0,Bmin).");
            if (bitRates.at(iLast + 1) <= alfa2 * rho) {
                INFOMSG("[%.3fs] InitialIncrease. beta in [0, Bmin) (%.3g in [0, %g)). Throughput high enough to increase: r_higher <= alfa2 * rho (%.3g <= %g * %.3g).", dashp2p::Utilities::now(),
                        beta, Bmin, bitRates.at(iLast + 1) / 1e6, alfa2, rho / 1e6);
                return Decision(bitRates.at(iLast + 1), std::numeric_limits<int64_t>::max(), II1_UP);
            } else {
                INFOMSG("[%.3fs] InitialIncrease. beta in [0, Bmin) (%.3g in [0, %g)). Throughput too low to increase: r_higher > alfa2 * rho (%.3g <= %g * %.3g).", dashp2p::Utilities::now(),
                        beta, Bmin, bitRates.at(iLast + 1) / 1e6, alfa2, rho / 1e6);
                return Decision(bitRates.at(iLast), std::numeric_limits<int64_t>::max(), II1_KEEP);
            }
        }
        else if (initialIncrease && beta < Blow)
        {
            DBGMSG("We are in [Bmin, Blow).");
            if (bitRates.at(iLast + 1) <= alfa3 * rho) {
                INFOMSG("[%.3fs] InitialIncrease. beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput high enough to increase: r_higher <= alfa3 * rho (%.3g <= %g * %.3g).", dashp2p::Utilities::now(),
                        beta, Bmin, Blow, bitRates.at(iLast + 1) / 1e6, alfa3, rho / 1e6);
                return Decision(bitRates.at(iLast + 1), std::numeric_limits<int64_t>::max(), II2_UP);
            } else {
                INFOMSG("[%.3fs] InitialIncrease. beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput too low to increase: r_higher > alfa3 * rho (%.3g <= %g * %.3g).", dashp2p::Utilities::now(),
                        beta, Bmin, Blow, bitRates.at(iLast + 1) / 1e6, alfa3, rho / 1e6);
                return Decision(bitRates.at(iLast), std::numeric_limits<int64_t>::max(), II2_KEEP);
            }
        }
        else if (initialIncrease)
        {
            DBGMSG("We are in [Blow, Inf).");
            if (bitRates.at(iLast + 1) <= alfa4 * rho) {
            	const ContentIdSegment nextSegment(lastSegment.periodIndex(), lastSegment.adaptationSetIndex(), bitRates.at(iLast + 1), lastSegment.segmentIndex() + 1);
            	const double Bdelay_new = (Bhigh - mpdWrapper->getSegmentDuration(nextSegment) / 1e6);
                if(Bdelay_new < beta) {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher <= alfa4 * rho (%.3f <= %f * %.3f). Delay until beta = %.3f.", dashp2p::Utilities::now(),
                            beta, Blow, bitRates.at(iLast + 1) / 1e6, alfa4, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher <= alfa4 * rho (%.3f <= %f * %.3f).", dashp2p::Utilities::now(),
                            beta, Blow, bitRates.at(iLast + 1) / 1e6, alfa4, rho / 1e6);
                }
                return Decision(bitRates.at(iLast + 1), Bdelay_new * 1000000, II3_UP);
            } else {
            	const ContentIdSegment nextSegment(lastSegment.periodIndex(), lastSegment.adaptationSetIndex(), bitRates.at(iLast), lastSegment.segmentIndex() + 1);
            	const double Bdelay_new = (Bhigh - mpdWrapper->getSegmentDuration(nextSegment) / 1e6);
                if(Bdelay_new < beta) {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher > alfa4 * rho (%.3f <= %f * %.3f). Delay until beta = %.3f.", dashp2p::Utilities::now(),
                            beta, Blow, bitRates.at(iLast) / 1e6, alfa4, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher > alfa4 * rho (%.3f <= %f * %.3f).", dashp2p::Utilities::now(),
                            beta, Blow, bitRates.at(iLast) / 1e6, alfa4, rho / 1e6);
                }
                return Decision(bitRates.at(iLast), Bdelay_new * 1000000, II3_KEEP);
            }
        }
        else
        {
            /* we just left the initial increase phase */
            initialIncreaseTerminationTime = now;
        }
    }

    /* if we are not in the initial increase phase,
     * act depending on buffer level and throughput. */

    /* we are in [0, Bmin) */
    if (beta < Bmin)
    {
        INFOMSG("[%.3fs] beta in [0, Bmin) (%.3g in [0, %g)). Switching to lowest representation: %.3g. rho = %.3g. rhoLast = %.3g.", dashp2p::Utilities::now(), beta, Bmin, bitRates.at(iLowest) / 1e6, rho / 1e6, rhoLast / 1e6);
        return Decision(bitRates.at(iLowest), std::numeric_limits<int64_t>::max(), NO0_LOWEST);
    }

    /* We are in [Bmin, Blow). */
    else if (beta < Blow)
    {DBGMSG("We are NOT at lowest representation. Checking if shall switch down.");
        if (iLast == iLowest) {
            INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Already at lowest representation: %.3g.", dashp2p::Utilities::now(), beta, Bmin, Blow, bitRates.at(iLowest) / 1e6);
            return Decision(bitRates.at(iLast), std::numeric_limits<int64_t>::max(), NO1_LOWEST);
        } else {
            if (rhoLast > (double)bitRates.at(iLast)) {
                INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput high enough to keep bit-rate: rhoLast > r_last (%.3g > %.3g).", dashp2p::Utilities::now(), beta, Bmin, Blow, rhoLast / 1e6, bitRates.at(iLast) / 1e6);
                return Decision(bitRates.at(iLast), std::numeric_limits<int64_t>::max(), NO1_KEEP);
            } else {
                INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput too low to keep bit-rate: rhoLast <= r_last (%.3g > %.3g).", dashp2p::Utilities::now(), beta, Bmin, Blow, rhoLast / 1e6, bitRates.at(iLast) / 1e6);
                return Decision(bitRates.at(iLast - 1), std::numeric_limits<int64_t>::max(), NO1_DOWN);
            }
        }
    }

    /* we are in [Blow, Bhigh) */
    else if  (beta < Bhigh)
    {
        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || bitRates.at(iLast + 1) >= alfa5 * rho) {
            /* delay */
        	const ContentIdSegment nextSegment(lastSegment.periodIndex(), lastSegment.adaptationSetIndex(), bitRates.at(iLast), lastSegment.segmentIndex() + 1);
            const double Bdelay_new = std::max<double>(beta - mpdWrapper->getSegmentDuration(nextSegment) / 1e6, 0.5 * (Blow + Bhigh));
            if(iLast == iHighest) {
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Already at highest bit-rate (%.3g). Delay until beta = %.3g.", dashp2p::Utilities::now(), beta, Blow, Bhigh, bitRates.at(iHighest) / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Already at highest bit-rate (%.3g).", dashp2p::Utilities::now(), beta, Blow, Bhigh, bitRates.at(iHighest) / 1e6);
                }
            } else {
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput too low, will delay: r_higher >= alfa5 * rho (%.3g >= %g * %.3g). Delay until beta = %.3g.", dashp2p::Utilities::now(), beta, Blow, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g).", dashp2p::Utilities::now(), beta, Blow, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6);
                }
            }
            return Decision(bitRates.at(iLast), Bdelay_new * 1000000, NO2_DELAY);
        } else {
            /* don't delay */
            INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput high enough not to delay: r_higher < alfa5 * rho (%.3g < %g * %.3g).", dashp2p::Utilities::now(), beta, Blow, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6);
            return Decision(bitRates.at(iLast), std::numeric_limits<int64_t>::max(), NO2_NODELAY);
        }
    }

    /* we are in [Bhigh, Inf) */
    else
    {
        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || bitRates.at(iLast + 1) >= alfa5 * rho) {
            /* delay */
            int64_t Bdelay_new = 0;
            if(iLast == iHighest) {
            	const ContentIdSegment nextSegment(lastSegment.periodIndex(), lastSegment.adaptationSetIndex(), bitRates.at(iLast), lastSegment.segmentIndex() + 1);
                Bdelay_new = std::max<int64_t>(1000000 * (int64_t)beta - mpdWrapper->getSegmentDuration(nextSegment) - 1000000, 500000 * (int64_t)(Blow + Bhigh));
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Already at highest bit-rate (%.3g). Delay until beta = %.3g.", dashp2p::Utilities::now(), beta, Bhigh, bitRates.at(iHighest) / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Already at highest bit-rate (%.3g).", dashp2p::Utilities::now(), beta, Bhigh, bitRates.at(iHighest) / 1e6);
                }
            } else {
            	const ContentIdSegment nextSegment(lastSegment.periodIndex(), lastSegment.adaptationSetIndex(), bitRates.at(iLast), lastSegment.segmentIndex() + 1);
                Bdelay_new = std::max<int64_t>(1000000 * (int64_t)beta - mpdWrapper->getSegmentDuration(nextSegment), 500000 * (int64_t)(Blow + Bhigh));
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g). Delay until beta = %.3g.", dashp2p::Utilities::now(), beta, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g).", dashp2p::Utilities::now(), beta, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6);
                }
            }
            return Decision(bitRates.at(iLast), Bdelay_new, NO3_DELAY);
        } else {
            /* don't delay */
            INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher < alfa5 * rho (%.3g < %g * %.3g).", dashp2p::Utilities::now(), beta, Bhigh, bitRates.at(iLast + 1) / 1e6, alfa5, rho / 1e6);
            return Decision(bitRates.at(iLast + 1), std::numeric_limits<int64_t>::max(), NO3_UP);
        }
    }
}

}

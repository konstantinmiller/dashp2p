/****************************************************************************
 * ControlLogicMH.cpp                                                       *
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

#include "ControlLogicMH.h"
#include "ControlLogicActionStartDownload.h"
//#include "ControlLogicActionUpdateContour.h"
#include "ControlLogicEventMpdReceived.h"
#include "ControlLogicEventDataReceived.h"
#include "ControlLogicEventDataPlayed.h"
#include "ContentIdSegment.h"
#include "DebugAdapter.h"
#include "Statistics.h"
#include "Utilities.h"
#include <assert.h>
#include <utility>
#include <limits>
using std::numeric_limits;

#ifndef __ANDROID__
# include <ifaddrs.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <net/if.h>
# include <errno.h>
# include <string.h>
#endif


ControlLogicMH::ControlLogicMH(const std::string& config)
  : ControlLogic(),
    Bmin(2),
    Blow(10),
    Bhigh(30),
    alfa1(0.75),
    alfa2(0.8),
    alfa3(0.8),
    alfa4(0.8),
    alfa5(0.9),
    Delta_t(5),
    betaTimeSeries(new TimeSeries<dash::Usec>(1000000, false, true)),
    initialIncrease(true),
    initialIncreaseTerminationTime(0),
    Bdelay(),
    delayedRequests()
{
#ifdef __ANDROID__
    return;
#else
    DBGMSG("Initializing ControlLogicMH with configuration: %s.", config.c_str());

    /* Get the names of the devices */
    dp2p_assert(config.at(0) == '(');
    size_t end = config.find(')', 1);
    dp2p_assert(end != config.npos);
    std::string devsStr = config.substr(1, end - 1);
    std::vector<std::string> ifNames = dash::Utilities::tokenize(devsStr, ':');
    dp2p_assert(ifNames.size() == 2); // only support 2 devs currently
    ifData.resize(ifNames.size());
    for(unsigned i = 0; i < ifNames.size(); ++i)
        ifData.at(i).name = ifNames.at(i);

    /* Get the IP addresses */
    int fdTmp = socket(AF_INET, SOCK_DGRAM, 0);
    dp2p_assert(fdTmp != -1);
    for(unsigned i = 0; i < ifData.size(); ++i)
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        DBGMSG("Trying to find IP address assigned to device %s.", ifData.at(i).name.c_str());
        strncpy(ifr.ifr_name, ifData.at(i).name.c_str(), IFNAMSIZ-1);
        int ioctlRet = ioctl(fdTmp, SIOCGIFADDR, &ifr);
        if(ioctlRet == -1) {
            WARNMSG("No device with device name [%s] found.", ifData.at(i).name.c_str());
            memset(&ifData.at(i).sockaddr_in, 0, sizeof(ifData.at(i).sockaddr_in));
        } else {
            INFOMSG("Device with device name [%s] found.", ifData.at(i).name.c_str());
            memcpy(&ifData.at(i).sockaddr_in, &ifr.ifr_addr, sizeof(ifData.at(i).sockaddr_in));
        }
        dp2p_assert(ifData.at(i).sockaddr_in.sin_port == 0 && ifData.at(i).sockaddr_in.sin_port == 0);
        ifData.at(i).initialized = true;
    }
    dp2p_assert(0 == close(fdTmp));

    /* The following code obtains IP adresses using the getifaddrs() function but this fails with point-to-point links like 3G */
#if 0
    struct ifaddrs* ifaddrs = NULL;
    dp2p_assert(0 == getifaddrs(&ifaddrs) && ifaddrs != NULL);
    const struct ifaddrs* ifaddrNext = ifaddrs;
    while(ifaddrNext != NULL)
    {
        printf("%s\n", ifaddrNext->ifa_name);
        printf("%d\n", ifaddrNext->ifa_addr->sa_family);
        if(ifaddrNext->ifa_addr->sa_family == AF_INET) {
            for(unsigned i = 0; i < ifData.size(); ++i) {
                if(0 == strcmp(ifaddrNext->ifa_name, ifData.at(i).name.c_str())) {
                    dp2p_assert(ifData.at(i).initialized == false);
                    memcpy(&ifData.at(i).sockaddr_in, ifaddrNext->ifa_addr, sizeof(ifData.at(i).sockaddr_in));
                    ifData.at(i).initialized = true;
                    break;
                }
            }
        }
        ifaddrNext = ifaddrNext->ifa_next;
    }
    freeifaddrs(ifaddrs);
    for(unsigned i = 0; i < ifData.size(); ++i)
        dp2p_assert(ifData.at(i).initialized);
    dp2p_assert(ifData.at(0).sockaddr_in.sin_port == 0 && ifData.at(1).sockaddr_in.sin_port == 0);
#endif

    DBGMSG("Will use %s (%s) and %s (%s).", ifData.at(0).name.c_str(), ifData.at(0).printAddr().c_str(), ifData.at(1).name.c_str(), ifData.at(1).printAddr().c_str());

    /* Check what is the primary interface */
    char buf[2048] = {0}; // should be enough for any usage below
    bool found = false;
    FILE* f = popen("ip route list", "r");
    dp2p_assert(f > 0);
    found = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, "default")) {
            dp2p_assert(!found);
            found = true;
            char* p = strstr(buf, "dev");
            dp2p_assert(p);
            p += 4;
            while(*p == ' ')
                ++p;
            DBGMSG("[%s], %d, %s", ifData.at(0).name.c_str(), ifData.at(0).name.size(), ifData.at(0).hasAddr() ? "yes" : "no");
            DBGMSG("[%s], %d, %s", ifData.at(1).name.c_str(), ifData.at(1).name.size(), ifData.at(1).hasAddr() ? "yes" : "no");
            if(ifData.at(0).hasAddr() && 0 == strncmp(p, ifData.at(0).name.c_str(), ifData.at(0).name.size())) {
                ifData.at(0).primary = true;
                found = true;
                DBGMSG("%s is primary interface.", ifData.at(0).name.c_str());
            } else if(ifData.at(1).hasAddr() && 0 == strncmp(p, ifData.at(1).name.c_str(), ifData.at(1).name.size())) {
                ifData.at(1).primary = true;
                found = true;
                DBGMSG("%s is primary interface.", ifData.at(1).name.c_str());
            } else {
                ERRMSG("Primary interface is %s but we are not using this interface.", p);
                exit(1);
            }
        }
    }
    dp2p_assert(found);
    dp2p_assert(0 == fclose(f));

    /* from here on, the hasSecondary() function might be called to ask if operating truly multi-homed */

    /* Check if we have the entry 'mhstreaming' in /etc/iproute2/rt_tables */
    f = fopen("/etc/iproute2/rt_tables", "r");
    if(!f) {
        ERRMSG("The file /etc/iproute2/rt_tables does not exist. Routing based on source address (required for multihoming) is not possible.");
        ERRMSG("Hint: have to install iproute2 package?");
        exit(1);
    }
    found = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, "mhstreaming")) {
            dp2p_assert(!found);
            found = true;
        }
    }
    dp2p_assert(0 == fclose(f));
    dp2p_assert(found);
    if(!found) {
        ERRMSG("No entry for the mhstreaming routing table found in /etc/iproute2/rt_tables. Please create one.");
        ERRMSG("Hint: 'echo 10 >> /etc/iproute2/rt_tables'.");
        exit(1);
    }
    DBGMSG("/etc/iproute2/rt_tables exists and has an entry 'mhstreaming'. Good!");

    /* Check if we have a correct default route in the mhstreaming table */
    if(hasSecondary())
    {
        f = popen("ip route list table mhstreaming", "r");
        dp2p_assert(f > 0);
        found = false;
        while(fgets(buf, sizeof(buf), f)) {
            if(strstr(buf, "default via")) {
                dp2p_assert(!found);
                char tmp[1024];
                sprintf(tmp, "dev %s", getSecondaryIf().name.c_str());
                dp2p_assert(strstr(buf, tmp));
                found = true;
            }
        }
        dp2p_assert(0 == fclose(f));
        if(!found) {
            ERRMSG("The mhstreaming routing table has no default entry.");
            ERRMSG("Hint: sudo ip route add default via <gateway IP> dev %s table mhstreaming.", getSecondaryIf().name.c_str());
            exit(1);
        }
        DBGMSG("Default route for the mhstreaming routing table is set correctly. Good!");
    }
    else
    {
        WARNMSG("Didn't check if a default route entry for the mhstreaming routing table exists and is correct, since no secondary interface present at the moment.");
    }

    /* check if the routing rules are correct for the secondary interface */
    if(hasSecondary())
    {
        char tmp1[2048];
        sprintf(tmp1, "from all to %s lookup mhstreaming", getSecondaryIf().printAddr().c_str());
        char tmp2[2048];
        sprintf(tmp2, "from %s lookup mhstreaming", getSecondaryIf().printAddr().c_str());
        f = popen("ip rule show", "r");
        dp2p_assert(f > 0);
        bool found1 = false;
        bool found2 = false;
        while(fgets(buf, sizeof(buf), f)) {
            if(strstr(buf, tmp1)) {
                dp2p_assert(!found1);
                found1 = true;
            } else if(strstr(buf, tmp2)) {
                dp2p_assert(!found2);
                found2 = true;
            }
        }
        dp2p_assert(0 == fclose(f));
        if(!found1 || !found2) {
            ERRMSG("Routing table is missing entries for your secondary interface.");
            ERRMSG("Hint 1: sudo ip rule add from %s table mhstreaming.", getSecondaryIf().printAddr().c_str());
            ERRMSG("Hint 2: sudo ip rule add to %s table mhstreaming.", getSecondaryIf().printAddr().c_str());
            exit(1);
        }
        DBGMSG("Routes from/to secondary IP are set correctly. Good!");
    }
    else
    {
        WARNMSG("Didn't check if routing table entries for the secondary interface exist and are correct, since no secondary interface present at the moment.");
    }

    if(hasSecondary()) {
        INFOMSG("Will stream over %d interfaces: %s (%s) and %s (%s).", ifData.size(), ifData.at(0).name.c_str(), ifData.at(0).printAddr().c_str(),
                ifData.at(1).name.c_str(), ifData.at(1).printAddr().c_str());
    } else {
        WARNMSG("Will stream over 1 interface: %s (%s).", getPrimaryIf().name.c_str(), getPrimaryIf().printAddr().c_str());
    }

    Bdelay.resize(ifData.size(), numeric_limits<dash::Usec>::max());
    delayedRequests.resize(ifData.size(), list<const ContentIdSegment*>());
    for(unsigned i = 0; i < ifData.size(); ++i) {
        //delayedRequests.at(i)
        //Bdelay.at(i)
    }
#endif
}

ControlLogicMH::~ControlLogicMH()
{
	for(unsigned i = 0; i < delayedRequests.size(); ++i) {
		while(!delayedRequests.at(i).empty()) {
			delete delayedRequests.at(i).front();
			delayedRequests.at(i).pop_front();
		}
	}
}

list<ControlLogicAction*> ControlLogicMH::processEventDataPlayed(const ControlLogicEventDataPlayed& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEventDataReceived(const ControlLogicEventDataReceived& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEventDisconnect(const ControlLogicEventDisconnect& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEventPause(const ControlLogicEventPause& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEventResumePlayback(const ControlLogicEventResumePlayback& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEventStartPlayback(const ControlLogicEventStartPlayback& e)
{

}

list<ControlLogicAction*> ControlLogicMH::processEvent(const ControlLogicEvent* e)
{
    dp2p_assert(e);

    /* Return object */
    list<ControlLogicAction*> actions;

    /* We are done -> no actions. */
    if(state == DONE) {
        delete e;
        return actions;
    }

    /* Act depending on received event */
    switch(e->getType()) {

    case Event_MpdReceived:
    {
        dp2p_assert(state == NO_MPD && contour.empty() && !mpdWrapper);
        state = HAVE_MPD;

        const ControlLogicEventMpdReceived& event = dynamic_cast<const ControlLogicEventMpdReceived&>(*e);
        mpdWrapper = event.mpdWrapper;
        representations = event.representations;
        startSegment = event.startSegment;
        stopSegment = event.stopSegment;

        const unsigned lowestBitrate = representations.at(0).bandwidth;

        /* Update the contour with all segments at lowest quality. */
        //ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
        //updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
        //updateContour->segId = SegId(lowestBitrate, 0);
        //actions.push_back(updateContour);
        contour.setNext(ContentIdSegment(lowestBitrate, 0));
        for(int i = startSegment; i <= stopSegment; ++i) {
            //ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
            //updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
            //updateContour->segId = SegId(lowestBitrate, i);
            //actions.push_back(updateContour);
        	contour.setNext(ContentIdSegment(lowestBitrate, i));
        }

        /* Primary interface will download the initialization segment and the first data segment. */
        list<const ContentId*> segIds1;
        segIds1.push_back(new ContentIdSegment(lowestBitrate, 0));
        segIds1.push_back(new ContentIdSegment(lowestBitrate, startSegment));
        //for(int i = startSegment; i <= stopSegment; i+=2)
        //    segIds1.push_back(SegId(lowestBitrate, i));
        actions.push_back(createActionDownloadSegments(segIds1, getPrimaryIfInd(), HttpMethod_GET));

        /* Secondary interface will download the second data segment */
        if(hasSecondary() && startSegment < stopSegment)
        {
            list<const ContentId*> segIds2;
            segIds2.push_back(new ContentIdSegment(lowestBitrate, startSegment + 1));
            //for(int i = startSegment + 1; i <= stopSegment; i+=2)
            //    segIds2.push_back(SegId(lowestBitrate, i));
            //if(!segIds2.empty())
            actions.push_back(createActionDownloadSegments(segIds2, getSecondaryIfInd(), HttpMethod_GET));
        }
    }
    break;

    case Event_DataReceived:
    {
        DBGMSG("Event: data received.");

        dp2p_assert(state == HAVE_MPD);

        const ControlLogicEventDataReceived& event = dynamic_cast<const ControlLogicEventDataReceived&>(*e);

        const ContentIdSegment* const segId = dynamic_cast<const ContentIdSegment*>(event.req->contentId);

        /* We do not start a new download if (i) the last one is not finished yet, or (ii) we have already downloading the stop segment,
         * or (iii) we downloaded the initial segment (since we have aready requested initial segment and start segment pipelined) */
        if(event.byteTo != event.req->hdr.contentLength - 1) {
            DBGMSG("Segment not ready yet. No action required.");
            break;
        } else if (segId->segNr() == stopSegment) {
            DBGMSG("Stop segment. No action required.");
            break;
        } else if (segId->segNr() == 0) {
            DBGMSG("Init segment. No action required.");
            break;
        }

        const int ifInd = getIfInd(event.req->devName);

        dp2p_assert(delayedRequests.at(ifInd).empty());

        betaTimeSeries->pushBack(dash::Utilities::getTime(), event.availableContigInterval.first);

        /* select bit-rate */
        const bool ifBetaMinIncreasing = betaTimeSeries->minIncreasing();
        const dash::Usec beta = event.availableContigInterval.first;
        const double rho = Statistics::getThroughput(std::min<double>(Delta_t, dash::Utilities::now()), event.req->devName);
        const double rhoLast = Statistics::getThroughputLastRequest();
        const unsigned completedRequests = (segId->segNr() == 0) ? 1 : segId->segNr() - startSegment + 2;
        const int r_last = segId->repId();
        Decision adaptationDecision = selectRepresentation(
                ifBetaMinIncreasing,
                beta / 1e6,
                rho,
                rhoLast,
                completedRequests,
                r_last);
        //std::pair<Representation, double> repAndDelay = AdaptationST::selectRepresentationAlwaysLowest();
        //std::pair<Representation, double> repAndDelay = AdaptationST::selectRepresentation("0");

        dp2p_assert(adaptationDecision.Bdelay > 0);
        Bdelay.at(ifInd) = adaptationDecision.Bdelay;
        const int r_new = adaptationDecision.rep.bandwidth;

        const ContentIdSegment* segNext = new ContentIdSegment(r_new, segId->segNr() + 1);
        DBGMSG("Will download segment Nr. %d over device %s (last one will be %d, segment 0 is initial segment.)", segNext->segNr(), event.req->devName.c_str(), stopSegment);

        DBGMSG("Selected bit-rate %.3f Mbit/sec. Delay download until beta == %.3f sec",
                segNext->repId() / 1e6, Bdelay.at(ifInd) / 1e6);

        /* log selected bit-rate */
        Statistics::recordAdaptationDecision(dash::Utilities::now(), beta, rho, rhoLast, r_last, r_new, Bdelay.at(ifInd), ifBetaMinIncreasing, adaptationDecision.reason);

        /* Update contour */
        //ControlLogicActionUpdateContour* updateContour = new ControlLogicActionUpdateContour();
        //updateContour->updateAction = ControlLogicActionUpdateContour::SET_NEXT;
        //updateContour->segId = segNext;
        //actions.push_back(updateContour);
        contour.setNext(*segNext);

        /* Either request the next segment immediately or save it in delayedRequests */
        if(event.availableContigInterval.first <= Bdelay.at(ifInd)) {
            list<const ContentId*> segIds;
            segIds.push_back(segNext);
            actions.push_back(createActionDownloadSegments(segIds, ifInd, HttpMethod_GET));
        } else {
            delayedRequests.at(ifInd).push_back(segNext->copy());
        }
    }
    break;

    case Event_DataPlayed:
    {
        /* nothing here, for the moment */
    }
    break;

    default:
        dp2p_assert(0);
        break;
    }

    delete e;
    return actions;
}

int ControlLogicMH::getIfInd(string devName) const
{
    for(unsigned i = 0; i < ifData.size(); ++i) {
        if(ifData.at(i).name.compare(devName) == 0)
            return i;
    }
    dp2p_assert(0);
    return 0;
}

IfData ControlLogicMH::getPrimaryIf() const
{
    int ind = -1;
    for(unsigned i = 0; i < ifData.size(); ++i) {
        if(ifData.at(i).primary) {
            dp2p_assert(ind == -1);
            ind = i;
        }
    }
    dp2p_assert(ind != -1);
    return ifData.at(ind);
}

IfData ControlLogicMH::getSecondaryIf() const
{
    int ind = -1;
    for(unsigned i = 0; i < ifData.size(); ++i) {
        if(!ifData.at(i).primary) {
            dp2p_assert(ind == -1);
            ind = i;
        }
    }
    dp2p_assert(ind != -1);
    return ifData.at(ind);
}

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

ControlLogicMH::Decision ControlLogicMH::selectRepresentation(bool ifBetaMinIncreasing, double beta,
        double rho, double rhoLast, unsigned completedRequests, int r_last)
{
    /* debug output */
    INFOMSG("Selecting representation. %s, beta: %.3g, rho: %.3g, rhoLast: %.3g, r_last: %.3g.",
            ifBetaMinIncreasing ? "beta_min incr." : "beta_min decr.", beta, rho / 1e6, rhoLast / 1e6, r_last / 1e6);

    /* current time since beginning of download */
    const dash::Usec now = dash::Utilities::getTime();

    /* verify that we already obtained a list of representations */
    dp2p_assert(representations.size() && completedRequests > 1);

    /* Get indexes of some relevant representations for faster access. */
    const unsigned iLowest = 0;
    const unsigned iLast = getIndex(r_last);
    const unsigned iHighest = representations.size() - 1;

    /* Are we in the initial increase phase? */
    if (initialIncrease)
    {
        DBGMSG("We are in the initial increase phase.");

        /* Check if we shall terminate the initial increase phase. */

        /* Check if we already selected highest representation. */
        if (initialIncrease && iLast == iHighest) {
            initialIncrease = false;
            INFOMSG("[%.3fs] Terminating initial increase phase. Reached highest representation.");
        }

        /* Check if the discretized minimum buffer level had been decreasing. */
        if (initialIncrease && !ifBetaMinIncreasing) {
            initialIncrease = false;
            INFOMSG("[%.3fs] Terminating initial increase phase. Discretized min buffer level was decreasing: %s.", dash::Utilities::now(), betaTimeSeries->printDiscretizedMin().c_str());
        }

        /* Check if current representation is above alfa1 * rho. */
        if (initialIncrease && representations.at(iLast).bandwidth > alfa1 * rho) {
            INFOMSG("[%.3fs] Terminating initial increase phase. r_higher > alfa1 * rho (%.3f > %f * %.3f).", dash::Utilities::now(),
                    representations.at(iLast).bandwidth / 1e6, alfa1, rho / 1e6);
            initialIncrease = false;
        }

        /* if we didn't terminate the initial increase phase,
         * act depending on buffer level and throughput. */
        if (initialIncrease && beta < Bmin)
        {
            DBGMSG("We are in [0,Bmin).");
            if (representations.at(iLast + 1).bandwidth <= alfa2 * rho) {
                INFOMSG("[%.3fs] InitialIncrease. beta in [0, Bmin) (%.3g in [0, %g)). Throughput high enough to increase: r_higher <= alfa2 * rho (%.3g <= %g * %.3g).", dash::Utilities::now(),
                        beta, Bmin, representations.at(iLast + 1).bandwidth / 1e6, alfa2, rho / 1e6);
                return Decision(representations.at(iLast + 1), std::numeric_limits<dash::Usec>::max(), II1_UP);
            } else {
                INFOMSG("[%.3fs] InitialIncrease. beta in [0, Bmin) (%.3g in [0, %g)). Throughput too low to increase: r_higher > alfa2 * rho (%.3g <= %g * %.3g).", dash::Utilities::now(),
                        beta, Bmin, representations.at(iLast + 1).bandwidth / 1e6, alfa2, rho / 1e6);
                return Decision(representations.at(iLast), std::numeric_limits<dash::Usec>::max(), II1_KEEP);
            }
        }
        else if (initialIncrease && beta < Blow)
        {
            DBGMSG("We are in [Bmin, Blow).");
            if (representations.at(iLast + 1).bandwidth <= alfa3 * rho) {
                INFOMSG("[%.3fs] InitialIncrease. beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput high enough to increase: r_higher <= alfa3 * rho (%.3g <= %g * %.3g).", dash::Utilities::now(),
                        beta, Bmin, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa3, rho / 1e6);
                return Decision(representations.at(iLast + 1), std::numeric_limits<dash::Usec>::max(), II2_UP);
            } else {
                INFOMSG("[%.3fs] InitialIncrease. beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput too low to increase: r_higher > alfa3 * rho (%.3g <= %g * %.3g).", dash::Utilities::now(),
                        beta, Bmin, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa3, rho / 1e6);
                return Decision(representations.at(iLast), std::numeric_limits<dash::Usec>::max(), II2_KEEP);
            }
        }
        else if (initialIncrease)
        {
            DBGMSG("We are in [Blow, Inf).");
            const double Bdelay_new = (Bhigh - representations.at(iLast)._nominalSegmentDuration / 1e6);
            if (representations.at(iLast + 1).bandwidth <= alfa4 * rho) {
                if(Bdelay_new < beta) {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher <= alfa4 * rho (%.3f <= %f * %.3f). Delay until beta = %.3f.", dash::Utilities::now(),
                            beta, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa4, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher <= alfa4 * rho (%.3f <= %f * %.3f).", dash::Utilities::now(),
                            beta, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa4, rho / 1e6);
                }
                return Decision(representations.at(iLast + 1), Bdelay_new * 1000000, II3_UP);
            } else {
                if(Bdelay_new < beta) {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher > alfa4 * rho (%.3f <= %f * %.3f). Delay until beta = %.3f.", dash::Utilities::now(),
                            beta, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa4, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] InitialIncrease. beta in [Blow, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher > alfa4 * rho (%.3f <= %f * %.3f).", dash::Utilities::now(),
                            beta, Blow, representations.at(iLast + 1).bandwidth / 1e6, alfa4, rho / 1e6);
                }
                return Decision(representations.at(iLast), Bdelay_new * 1000000, II3_KEEP);
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
        INFOMSG("[%.3fs] beta in [0, Bmin) (%.3g in [0, %g)). Switching to lowest representation: %.3g. rho = %.3g. rhoLast = %.3g.", dash::Utilities::now(), beta, Bmin, representations.at(iLowest).bandwidth / 1e6, rho / 1e6, rhoLast / 1e6);
        return Decision(representations.at(iLowest), std::numeric_limits<dash::Usec>::max(), NO0_LOWEST);
    }

    /* We are in [Bmin, Blow). */
    else if (beta < Blow)
    {DBGMSG("We are NOT at lowest representation. Checking if shall switch down.");
    if (iLast == iLowest) {
        INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Already at lowest representation: %.3g.", dash::Utilities::now(), beta, Bmin, Blow, representations.at(iLowest).bandwidth / 1e6);
        return Decision(representations.at(iLast), std::numeric_limits<dash::Usec>::max(), NO1_LOWEST);
    } else {
        if (rhoLast > (double)representations.at(iLast).bandwidth) {
            INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput high enough to keep bit-rate: rhoLast > r_last (%.3g > %.3g).", dash::Utilities::now(), beta, Bmin, Blow, rhoLast / 1e6, representations.at(iLast).bandwidth / 1e6);
            return Decision(representations.at(iLast), std::numeric_limits<dash::Usec>::max(), NO1_KEEP);
        } else {
            INFOMSG("[%.3fs] beta in [Bmin, Blow) (%.3g in [%g, %g)). Throughput too low to keep bit-rate: rhoLast <= r_last (%.3g > %.3g).", dash::Utilities::now(), beta, Bmin, Blow, rhoLast / 1e6, representations.at(iLast).bandwidth / 1e6);
            return Decision(representations.at(iLast - 1), std::numeric_limits<dash::Usec>::max(), NO1_DOWN);
        }
    }
    }

    /* we are in [Blow, Bhigh) */
    else if  (beta < Bhigh)
    {
        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || representations.at(iLast + 1).bandwidth >= alfa5 * rho) {
            /* delay */
            const double Bdelay_new = std::max<double>(beta - representations.at(iLast)._nominalSegmentDuration / 1e6, 0.5 * (Blow + Bhigh));
            if(iLast == iHighest) {
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Already at highest bit-rate (%.3g). Delay until beta = %.3g.", dash::Utilities::now(), beta, Blow, Bhigh, representations.at(iHighest).bandwidth / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Already at highest bit-rate (%.3g).", dash::Utilities::now(), beta, Blow, Bhigh, representations.at(iHighest).bandwidth / 1e6);
                }
            } else {
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput too low, will delay: r_higher >= alfa5 * rho (%.3g >= %g * %.3g). Delay until beta = %.3g.", dash::Utilities::now(), beta, Blow, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g).", dash::Utilities::now(), beta, Blow, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6);
                }
            }
            return Decision(representations.at(iLast), Bdelay_new * 1000000, NO2_DELAY);
        } else {
            /* don't delay */
            INFOMSG("[%.3fs] beta in [Blow, Bhigh) (%.3g in [%g, %g)). Throughput high enough not to delay: r_higher < alfa5 * rho (%.3g < %g * %.3g).", dash::Utilities::now(), beta, Blow, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6);
            return Decision(representations.at(iLast), std::numeric_limits<dash::Usec>::max(), NO2_NODELAY);
        }
    }

    /* we are in [Bhigh, Inf) */
    else
    {
        /* check if throughput is enough for next higher encoding rate. */
        if(iLast == iHighest || representations.at(iLast + 1).bandwidth >= alfa5 * rho) {
            /* delay */
            dash::Usec Bdelay_new = 0;
            if(iLast == iHighest) {
                Bdelay_new = std::max<dash::Usec>(1000000 * (dash::Usec)beta - representations.at(iLast)._nominalSegmentDuration - 1000000, 500000 * (dash::Usec)(Blow + Bhigh));
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Already at highest bit-rate (%.3g). Delay until beta = %.3g.", dash::Utilities::now(), beta, Bhigh, representations.at(iHighest).bandwidth / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Already at highest bit-rate (%.3g).", dash::Utilities::now(), beta, Bhigh, representations.at(iHighest).bandwidth / 1e6);
                }
            } else {
                Bdelay_new = std::max<dash::Usec>(1000000 * (dash::Usec)beta - representations.at(iLast)._nominalSegmentDuration, 500000 * (dash::Usec)(Blow + Bhigh));
                if(beta > Bdelay_new) {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g). Delay until beta = %.3g.", dash::Utilities::now(), beta, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6, Bdelay_new);
                } else {
                    INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput too low to increase: r_higher >= alfa5 * rho (%.3g >= %g * %.3g).", dash::Utilities::now(), beta, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6);
                }
            }
            return Decision(representations.at(iLast), Bdelay_new, NO3_DELAY);
        } else {
            /* don't delay */
            INFOMSG("[%.3fs] beta in [Bhigh, Inf) (%.3g in [%g, Inf)). Throughput high enough to increase: r_higher < alfa5 * rho (%.3g < %g * %.3g).", dash::Utilities::now(), beta, Bhigh, representations.at(iLast + 1).bandwidth / 1e6, alfa5, rho / 1e6);
            return Decision(representations.at(iLast + 1), std::numeric_limits<dash::Usec>::max(), NO3_UP);
        }
    }
}

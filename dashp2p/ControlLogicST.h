/****************************************************************************
 * ControlLogicST.h                                                         *
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

#ifndef CONTROLLOGICST_H_
#define CONTROLLOGICST_H_

#include "dashp2p.h"
#include "ControlLogic.h"
#include "TimeSeries.h"
#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;

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
 *     Application number: 13/775,885
 *
 * Configuration parameters:
 *   o Delta_t - time window for averaging throughput
 *   o alfa_1, ..., alfa_5
 *   o B_min, B_low, B_high
 *   o minimum number of completed requests before which we enforce lowest representation
 *
 * Input information (static):
 *   o r                   - Bit-rates of available representations in [bit/sec], sorted in ascending order
 *   o tau                 - Segment duration (assumed to be constant) in [sec]
 *
 * Input information (dynamic):
 *   o beta_min_decreasing - First point in time when beta_min was decreasing in [sec]
 *   o beta                - Current buffer level in [sec]
 *   o rho                 - Average throughput over [t-Delta_t, t] in [bit/sec]
 *   o rhoLast             - Throughput of the last GET request in [bit/sec]
 *   o number of completed requests
 *   o r_last              - Last selected representation
 *
 * Output information:
 *   o B_delay             - Buffer level until which to delay next GET request in [us]
 *   o r_next              - Next represenation
 */

namespace dashp2p {

class ControlLogicST: public ControlLogic
{
/* Public methods */
public:
    ControlLogicST(int width, int height, const string& config);
    virtual ~ControlLogicST();
    virtual ControlType getType() const {return ControlType_ST;}

    double get_Delta_t() const {return Delta_t;}

/* Private types */
private:
    /* Data tupe to hold the decision. */
    class Decision {
    public:
        Decision(const int bitRate, double Bdelay, int reason) : bitRate(bitRate), Bdelay(Bdelay), reason(reason) {}
        int            bitRate;
        int64_t     Bdelay;
        int            reason; // for debugging/logging purposes
    };

/* Private methods */
private:

    virtual list<ControlLogicAction*> processEventConnected           (const ControlLogicEventConnected& e);
    virtual list<ControlLogicAction*> processEventDataPlayed          (const ControlLogicEventDataPlayed& e);
    virtual list<ControlLogicAction*> processEventDataReceivedMpd     (ControlLogicEventDataReceived& e);
    //virtual list<ControlLogicAction*> processEventDataReceivedMpdPeer (ControlLogicEventDataReceived& /*e*/) {abort(); return list<ControlLogicAction*>();}
    virtual list<ControlLogicAction*> processEventDataReceivedSegment (ControlLogicEventDataReceived& e);
    //virtual list<ControlLogicAction*> processEventDataReceivedTracker (ControlLogicEventDataReceived& /*e*/) {abort(); return list<ControlLogicAction*>();}
    //virtual list<ControlLogicAction*> processEventDisconnect          (const ControlLogicEventDisconnect& e);
    //virtual list<ControlLogicAction*> processEventPause               (const ControlLogicEventPause& e);
    //virtual list<ControlLogicAction*> processEventResumePlayback      (const ControlLogicEventResumePlayback& e);
    virtual list<ControlLogicAction*> processEventStartPlayback       (const ControlLogicEventStartPlayback& e);

    //virtual list<ControlLogicAction*> actionRejectedStartDownload(ControlLogicActionStartDownload* a);

    /* Selects the representation for the next segment and the buffer level when the download should be started (Inf, if immediately). */
    Decision selectRepresentation(bool ifBetaMinIncreasing, double beta,
    		double rho, double rhoLast, unsigned completedRequests, const ContentIdSegment& lastSegment);

/* Private fields */
private:
    /* Parameters */
    double Bmin;
    double Blow;
    double Bhigh;
    double alfa1;
    double alfa2;
    double alfa3;
    double alfa4;
    double alfa5;
    double Delta_t;
    int fetchHeads;

    TimeSeries<int64_t>* betaTimeSeries;

    /* Indicates if we are in the initial increase phase. */
    bool initialIncrease;

    /* Time when the initial increase phase was terminated. */
    double initialIncreaseTerminationTime;

    int64_t Bdelay;
    list<const ContentId*> delayedRequests;

    int connId;
    dashp2p::URL mpdUrl;
};

}

#endif /* CONTROLLOGICST_H_ */

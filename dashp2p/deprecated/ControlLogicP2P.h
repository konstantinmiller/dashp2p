/****************************************************************************
 * ControlLogicP2P.h                                                        *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
 * Authors: Konstantin Miller <konstantin.miller@tu-berlin.de>              *
 * 			Jasminka Serafimoska											*
 * 			Farid Rosli														*
 * 			Benjamin Barth										            *
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

#ifndef CONTROLLOGICP2P_H_
#define CONTROLLOGICP2P_H_


#include "ControlLogic.h"
//#include "TimeSeries.h"
#include <string>
#include <vector>
#include "Peer.h"
using std::string;
using std::vector;


class ControlLogicP2P: public ControlLogic
{
/* Public methods */
public:
	ControlLogicP2P(int width, int height, Usec startPosition, Usec stopPosition, const string& config);
    virtual ~ControlLogicP2P();
    virtual ControlType getType() const {return ControlType_P2P;}
/* Private fields */
public:
/* Private methods */
private:

    virtual list<ControlLogicAction*> processEventConnected           (const ControlLogicEventConnected& e);
    virtual list<ControlLogicAction*> processEventDataPlayed         (const ControlLogicEventDataPlayed& e);
    virtual list<ControlLogicAction*> processEventDataReceivedMpd    (ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDataReceivedMpdPeer (ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDataReceivedSegment(ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDataReceivedTracker (ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDisconnect         (const ControlLogicEventDisconnect& e);
    // TOOD: you don't have handle this event
    virtual list<ControlLogicAction*> processEventPause              (const ControlLogicEventPause& /*e*/) {abort(); return list<ControlLogicAction*>();}
    // TOOD: you don't have handle this event
    virtual list<ControlLogicAction*> processEventResumePlayback     (const ControlLogicEventResumePlayback& /*e*/) {abort(); return list<ControlLogicAction*>();}
    virtual list<ControlLogicAction*> processEventStartPlayback      (const ControlLogicEventStartPlayback& e);

    virtual list<ControlLogicAction*> actionRejectedStartDownload(ControlLogicActionStartDownload* a);

    struct decisionParameters {int Index; int repId;};
    struct decisionParameters decision(int segNr,list<ControlLogicAction*> *actions);
    //int decision(int segNr);
    void updatePeerMPD(int index, list<ControlLogicAction*> *actions);
    void updateAllInterestingPeerMPDs(list<ControlLogicAction*> *actions);
    int findMattchingRepId(double bandwith);
    int findNextLowerRepId(int repid);
    int modeToInt();
    void downloadSegment(list<ControlLogicAction*> *actions);

/* Private fields */
private:
    double actualFetchTime;
    double previousFetchTime;
    int expectedServerRepId;
    int countServerSegmentsAfterTcpSlowStart;
    int serverId;
    int connId;
    int trackerConnId;
    uint peerCounter;
    enum Mode {Server_Measurment, Peer_Measurment, Streaming, Downloading_PeerMPD, No_P2P,init};
    Mode mode;
    dash::URL mpdUrl;
    vector<Peer>peers;
    vector<int> indexesGoodPeers;
    uint index;
    bool tcpSlowStartServerOver;
    bool firstPeermeasurement;
    bool firstMDPPeerUpdate;
    bool peersMeasured;
    bool lastdownloadAtServer;
    bool firstSegment;
    bool measurementEnable;
    bool bufferlevelIsOkIWait;
    bool downloadedAllSegments;
    bool waitForMPDPeerUpdate;
    bool serverMeasured;
    double serverBandwith;
	int nextSegId;
	double segmentDuraton;
	int lastSegmentsBitrate;
	dash::Usec actualBufferlevel;
	string httpString;
	string trackerHostname;
	string trackerPath;
	string trackerFilenName;
	string MpdPeerFileName;
	string MpdPeerPath;
	string fileSystemUrl;
	//self Information
	string myIp;
	string myPort;
	string myId;
	string myBaseUrl;
	string actualMpdId;
	//Measurement settings
	uint segmentsAvoidedInPeerMeasurement;
	//EvaluationSettings
	string pathMeasurmentFile;
	//needs to be global for the evaluation
	int nextRepId;
};


#endif /* CONTROLLOGICP2P_H_ */

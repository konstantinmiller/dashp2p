/****************************************************************************
 * ControlLogicP2P.cpp                                                      *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
 * Authors: Konstantin Miller <konstantin.miller@tu-berlin.de> 				*
 * 			Jasminka Serafimoska											*
 * 			Farid Rosli														*
 * 			Benjamin Barth										            *
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
#include "ContentId.h"
#include "ControlLogicP2P.h"
#include "ControlLogicAction.h"
#include "Statistics.h"
#include "OverlayAdapter.h"
#include "DebugAdapter.h"
#include "xml/basic_xml.h"
#include "XMLParserP2P.h"
#include "HttpRequestManager.h"
using namespace dp2p;

#include <stdio.h>
#include <assert.h>
#include <utility>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Control.h"

using std::numeric_limits;
using std::stringstream;

ControlLogicP2P::ControlLogicP2P(int width, int height, Usec startPosition, Usec stopPosition, const std::string& /*config*/)
  : ControlLogic(width, height, startPosition, stopPosition),
    mpdUrl()
{
	// TODO: you might pass some configuration parameters via the config string and parse them here
	/* **
	 *
		Inits
	 *
	*/
	//Variables for the Fetchtime. These are used to check if TCP slowstart has ended
	actualFetchTime = 1.0;
	previousFetchTime = 0.0;
	//Counts the number of Peer. Is used to check if all MPDPeer are received in the initial download
	peerCounter = 0;
	//ConnId to the Server
	serverId = 0;
	//ConnId to tracker
	trackerConnId = 1;
	//ConnId to the Peers. will be saved in the Peer object
	connId = 2;
	//Index used to point on Peers in the peers list
	index = 0;
	//Counts the number of segments received from the server after his TCP slow start phase
	countServerSegmentsAfterTcpSlowStart = 0;
	//Variable saves the expected quality level from the server
	expectedServerRepId = 0;
	//Bools
	tcpSlowStartServerOver = false;
	firstPeermeasurement = true;
	firstMDPPeerUpdate = true;
	peersMeasured = false;
	lastdownloadAtServer = false;
	firstSegment = true;
	measurementEnable = true;
	bufferlevelIsOkIWait= false;
	downloadedAllSegments = false;
	waitForMPDPeerUpdate = false;
	serverMeasured = false;
	//This Bufferlevel is used to decide
	//wether we wait till an MPD is Updated or we download from the next source
	actualBufferlevel = 0;
	//Set mode to init
	mode = init;
	//often used string
	httpString = "http://";
	//MPDPeer information
	MpdPeerPath = "";
	MpdPeerFileName = "PeerMPD.xml";
	//Information about myself
	myId = "einstein";
	fileSystemUrl = "/var/www/";
	myBaseUrl = "Segments/";
	myIp = "192.168.60.28";
	myPort = "80";
	//Tracker information
	trackerHostname  = "130.149.49.253";
	trackerPath = "/pjdashp2p/benniszeug/";
	//Measurement settings
	//The number here gives the number of packets
	//not being considered in the peer measurement
	//to avoid their tcp slowstart phase and get a better
	//Idea of the real quality available
	segmentsAvoidedInPeerMeasurement = 3;
	//EvaluationSettings
	//pathMeasurmentFile = "/home/benni/TKNTraces/";

	if(myId.compare("einstein") == 0)
	{
		myIp = "192.168.60.27";
		pathMeasurmentFile = "/home/jasminka/TKNTraces/scenario/trace";
	}
	if(myId.compare("benni") == 0)
	{
		myIp = "192.168.60.28";
		pathMeasurmentFile = "/home/benni/TKNTraces/scenario/trace";
	}
	if(myId.compare("jasminka") == 0)
	{
		myIp = "192.168.60.30";
		pathMeasurmentFile = "/home/jasminka/TKNTraces/scenario/trace";
	}
	if(myId.compare("faridrosli") == 0)
	{
		myIp = "192.168.60.29";
		pathMeasurmentFile = "/home/faridrosli/TKNTraces/scenario/trace";
	}
}

ControlLogicP2P::~ControlLogicP2P()
{
    // TODO: do not forget to delete all the member variables you allocate
	/*
	//Download the tracker for the MPD ID
	trackerFilenName = "SQLTracker01.php?c=delete&ID=" + myId + "&MPD_ID=" + actualMpdId;
	dash::URL trackerurl(httpString,trackerHostname,trackerPath,trackerFilenName);
	list<const ContentId*> contentIds(1, new ContentIdTracker);
	list<dash::URL> urls(1, trackerurl);
	list<HttpMethod> httpMethods(1, HttpMethod_GET);
	actions.push_back(new ControlLogicActionStartDownload(connId, contentIds, urls, httpMethods));
	DBGMSG("set actionstartdownload TRACKER");
	*/
}

list<ControlLogicAction*> ControlLogicP2P::processEventConnected(const ControlLogicEventConnected& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

//	dp2p_assert(e.connId == connId); TODO: assert

	list<ControlLogicAction*> actions;

	ackActionConnected(e.connId);

	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::processEventDataPlayed(const ControlLogicEventDataPlayed& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	// TODO: data has been played means that the buffer level decreased. take actions if you want to.
	//Record traces for Measurement
	if(measurementEnable)
	{
		Statistics::recordP2PBufferlevelToFile(pathMeasurmentFile, e.availableContigInterval.first , e.availableContigInterval.second);
	}
	//We need this for the decicion if we wit till a PeerMPD is updated
	actualBufferlevel = e.availableContigInterval.first;
	//If we measuring the Peerstring filePath,string fileName ,s or are in StreamingMode we check if the Bufferlevel
	//Is under a threshold in microseconds. If it is we switch to ServermeasureMode To Fill up the Buffer again
	if((mode == Peer_Measurment) || (mode == Streaming))
	{
		//lower bufferlevel threshold
		dash::Usec timeThreshold = 2000000;
		if(actualBufferlevel <= timeThreshold)
		{
			DBGMSG("Bufferlevel degreased heavily switched to Server to fill up the buffer");
			mode = Server_Measurment;
			firstSegment = true;
			previousFetchTime = 0.0;
			serverMeasured = false;
		}
	}
	//If the mode is ServerMeasurement
	if(mode == Server_Measurment)
	{
		if(serverMeasured)
		{
			//upper bufferlevel threshold
			dash::Usec timeThreshold = 4000000;
			//Check if tcp Slowstart is over and the Bufferlevel is high enough
			if(actualBufferlevel >= timeThreshold)
			{
				//if all peers have been measured, we switched in ServerMeasurement because of
				//a to low bufflevel. Now we can switch back to streamingMode
				if(peersMeasured)
				{
					mode = Streaming;
				}
				/*If not all peers have been measured we switch back to PeerMeasurent.
				Or the initial ServerMeasurent has ended an we switch the first time to PeerMeasurent*/
				else
				{
					mode = Peer_Measurment;
					//Calculate the expected Qualitylevel we could gain from server
					expectedServerRepId = expectedServerRepId / countServerSegmentsAfterTcpSlowStart;
					DBGMSG("The expected quality from server is: %i",(int) expectedServerRepId);
					firstSegment = true;
					//reset the Fetchtime
					previousFetchTime = 0.0;
				}
			}
		}
		if(tcpSlowStartServerOver)
		{
			mode = Peer_Measurment;
			//Calculate the expected Qualitylevel we could gain from server
			expectedServerRepId = expectedServerRepId / countServerSegmentsAfterTcpSlowStart;
			DBGMSG("The expected quality from server is: %i",(int) expectedServerRepId);
			firstSegment = true;
			//reset the Fetchtime
			previousFetchTime = 0.0;
			serverMeasured = true;
			DBGMSG("Switched to server");
		}
	}
	/*Defining thresholds in which we do not download segments*/
	/*upper bufferlevel threshold*/
	dash::Usec upperTimeThreshold = 30000000;
	/*lower bufferlevel threshold*/
	dash::Usec lowerTimeThreshold = 15000000;
	if(actualBufferlevel >= upperTimeThreshold)
	{
		//we wait
		bufferlevelIsOkIWait = true;
		DBGMSG("Bufferlevel is over the threshold. Stop Downloading");
	}
	if(actualBufferlevel <= lowerTimeThreshold && bufferlevelIsOkIWait && !downloadedAllSegments)
	{
		//we start again
		bufferlevelIsOkIWait = false;
		downloadSegment(&actions);
		DBGMSG("Bufferlevel  is under the Threshold. Restart Download");
	}
	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::processEventDataReceivedMpd(ControlLogicEventDataReceived& e)
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
		Statistics::recordRequestStatistics(e.connId, e.reqId);
		ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId));
	} else {
		return actions;
	}

	bitRates = mpdWrapper->getBitrates(0, 0, width, height);
	DBGMSG("I read the MPD and fill the representation Vector");
	//const SegNr startSegment = getStartSegment();
	//const SegNr stopSegment = getStopSegment();

	//const unsigned lowestBitrate = representations.at(0).bandwidth;

	// TODO: you downloaded and parsed the MPD. now decide what to do next.
	//get the MPD Id
	string myMpdId = mpdWrapper->getMpdId();
	//check if an mpdid is in MPD
	if(myMpdId.compare("") !=0)
	{
		actualMpdId = myMpdId;
		//Download the tracker for the MPD ID
		trackerFilenName = "SQLTracker01.php?c=get&IP=" + myIp + ":" + myPort + "&ID=" + myId + "&MPD_ID=" + myMpdId;
		dash::URL trackerurl(httpString,trackerHostname,trackerPath,trackerFilenName);
		actions.push_back(new ControlLogicActionCreateTcpConnection(trackerConnId, IfData(), trackerHostname, 0));
		list<const ContentId*> contentIds(1, new ContentIdTracker);
		list<dash::URL> urls(1, trackerurl);
		list<HttpMethod> httpMethods(1, HttpMethod_GET);
		actions.push_back(new ControlLogicActionStartDownload(trackerConnId, contentIds, urls, httpMethods));
		DBGMSG("set actionstartdownload TRACKER");
		return actions;
	}
	//no P2P possible
	else
	{
		mode = No_P2P;
		/* Start downloading Segments with lowest quality */
		nextRepId = bitRates.at(0);
		list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, 0));
		string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, 0));
		dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
		list<dash::URL> urls(1, segmentURL);
		list<HttpMethod> httpMethods(1, HttpMethod_GET);
		actions.push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
		for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
		{
			contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
		}
		firstSegment = false;
		lastdownloadAtServer = true;
		DBGMSG("No ID in MPD. Download From Server no P2P Mode.");
		return actions;
	}
}

list<ControlLogicAction*> ControlLogicP2P::processEventDataReceivedMpdPeer(ControlLogicEventDataReceived& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	if(e.byteTo == HttpRequestManager::getContentLength(e.reqId) - 1) {
		ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId));
		DBGMSG("Received a complete MpdPeer");
		// TODO: you downloaded a peer MPD. now decide what to do next.

		//Parse the received MPDPeer
		XMLParserP2P::parseMPDPeer(HttpRequestManager::getPldBytes(e.reqId), HttpRequestManager::getContentLength(e.reqId) - 1, peers);

		if(peerCounter < peers.size() && mode == Downloading_PeerMPD)
		{
			peerCounter++;
			DBGMSG("actual number of MPDPeer received is: %d.", peerCounter);
		}
		//if all MPDPeers are received and we are in the initial MPDPeer Download
		if(peerCounter == peers.size() && mode == Downloading_PeerMPD)
		{
			DBGMSG("Received all MpdPeer");
			mode = Server_Measurment;
			/* Start downloading Segments with lowest quality */
			nextRepId = bitRates.at(0);
			list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, 0));
			string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, 0));
			dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
			list<dash::URL> urls(1, segmentURL);
			list<HttpMethod> httpMethods(1, HttpMethod_GET);
			actions.push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
			for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
			{
				contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
			}
			lastdownloadAtServer = true;
			firstSegment = false;
			return actions;
		}
		/*in this case an update of the MPDPeer was done while measuring a peer
		to get back to the receivedSegment action we have to chose another segment to download.*/
		if(mode == Peer_Measurment)
		{
			//check if the update was usefull and the peer now has the Segment we asked for
			vector<Peer::segmentInformation>* segInfo = peers[indexesGoodPeers[index]].getSegmentsInformation();
			uint k = 0;
			int repIdIndex = -1;
			while(k < segInfo->size())
			{
				if(nextSegId == segInfo->at(k).segment)
				{
					repIdIndex = k;
					k = segInfo->size();
				}
				k++;
			}
			//the peer has the segment after the MPDPeer Update and we will download it
			if(repIdIndex != -1)
			{
				stringstream segNr, repId;
					segNr << nextSegId;
					repId << segInfo->at(repIdIndex).representationID;
				string filename = segNr.str() + "-" + repId.str();
				dash::URL peerUrl("http://",peers[indexesGoodPeers[index]].getIP(),peers[indexesGoodPeers[index]].getclientBaseURL(),filename);
				nextRepId = segInfo->at(repIdIndex).representationID;
				list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, nextSegId));
				list<HttpMethod> httpMethods(1, HttpMethod_GET);
				list<dash::URL> peerUrls (1,peerUrl);
				actions.push_back(new ControlLogicActionStartDownload(peers[indexesGoodPeers[index]].getConnId(), contentIds, peerUrls, httpMethods));
				for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
				{
					contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
				}
				firstMDPPeerUpdate = true;
				firstSegment = false;
				previousFetchTime = 0;
				return actions;
				lastdownloadAtServer = false;
			}
			else
			{
				/*In this case the peer does not have the desired SegNr after an update of the PeerMPD.
				In order to get back in ProcessEventSegmentReceived we will download a segment from the Server*/
				nextRepId = findMattchingRepId(serverBandwith);
				list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, nextSegId));
				string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, nextSegId));
				dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
				list<dash::URL> urls(1, segmentURL);
				list<HttpMethod> httpMethods(1, HttpMethod_GET);
				actions.push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
				for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
				{
					contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
				}
				index++;
				lastdownloadAtServer = true;
				firstMDPPeerUpdate = true;
				previousFetchTime = 0;
				return actions;
				firstSegment = false;

			}
		}
		if(mode == Streaming && waitForMPDPeerUpdate)
		{
			downloadSegment(&actions);
			waitForMPDPeerUpdate = false;
		}

		Statistics::recordRequestStatistics(e.connId, e.reqId);
	}
	else
	{
		return actions;
	}



	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::processEventDataReceivedSegment(ControlLogicEventDataReceived& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	/* Return object */
	list<ControlLogicAction*> actions;

	const ContentIdSegment& segId = dynamic_cast<const ContentIdSegment&>(HttpRequestManager::getContentId(e.reqId));

	/* We do not start a new download if (i) the last one is not finished yet, or (ii) we have already downloading the stop segment,
	 * or (iii) we downloaded the initial segment (since we have already requested initial segment and start segment pipelined) */
	if(e.byteTo != HttpRequestManager::getContentLength(e.reqId) - 1) {
		DBGMSG("Segment not ready yet. No action required.");
		return actions;
	}

	DBGMSG("Segment completed. Processing.");

	dp2p_assert(ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId)));
	/*else if (segId.segNr() == 0) {
		DBGMSG("Init segment. No action required.");
		return actions;
	}*/

	// TODO: another segment is ready now, consider what actions to do next
	/*Writing in MPDPeer*/
	stringstream segNr, repId;
	segNr << segId.segmentIndex();
	repId << segId.bitRate();

	xmlDocPtr pDoc = xmlParseFile((fileSystemUrl+MpdPeerFileName).c_str());

	if(pDoc == NULL){
		/* no MPDPeer exists yet due to first start*/
		//MPDPeer is created here*/
		XMLParserP2P::createMPDPeer(segNr.str(), repId.str(), mpdWrapper->getMpdId().c_str(), pDoc, myId, myBaseUrl, fileSystemUrl+MpdPeerFileName);
		DBGMSG("MPDPeer created successfully");
	} else {
		/*MPD Peer already existed*/
		// check if it contains any segment information when downloading the first segment
		if (segId.segmentIndex() == 0){
			// remove and recreate a new MPDPeer
			if( remove((fileSystemUrl+MpdPeerFileName).c_str()) != 0 )
				DBGMSG( "Error deleting MPDPeer" );
			else
				XMLParserP2P::createMPDPeer(segNr.str(), repId.str(), mpdWrapper->getMpdId().c_str(), pDoc, myId, myBaseUrl, fileSystemUrl+MpdPeerFileName);
				DBGMSG( "MPDPeer removed and recreated successfully" );
		} else {
			/*add segment details into the MPDPeer*/
			XMLParserP2P::updateMPDPeer(segNr.str(), repId.str(), pDoc, fileSystemUrl+MpdPeerFileName);
			DBGMSG( "My MPDPeer updated successfully" );
		}
	}
	/*Writing segment to file*/
	string fileName = fileSystemUrl + myBaseUrl + segNr.str() + "-" + repId.str();
	DBGMSG("Writing file to : %s", fileName.c_str());
	Control::toFile(segId, fileName);
	/*check if this was the last segment*/
	if (segId.segmentIndex() == getStopSegment())
		{
			DBGMSG("Stop segment. No action required.");
			downloadedAllSegments = true;
			return actions;
		}
	/*
	 *
	 * Download the Next Segment
	 *
	 */
	/*Select next Segment*/
	nextSegId = segId.segmentIndex() + 1;
	/*Calculate the actual Fetchtime*/
	actualFetchTime = HttpRequestManager::getTsLastByte(e.reqId) - HttpRequestManager::getTsSent(e.reqId);
	DBGMSG("The actual fetchtime is : %lf", actualFetchTime);
	Statistics::recordRequestStatistics(e.connId, e.reqId);
	segmentDuraton = ((double) mpdWrapper->getSegmentDuration(segId)) * pow(10.0,-6);
	DBGMSG("The actual segments duration is : %lf ", segmentDuraton);
	lastSegmentsBitrate = segId.bitRate();

	if(!bufferlevelIsOkIWait)
		downloadSegment(&actions);
	else
		DBGMSG("Bufferlevel is still ok, I wait");

	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::processEventDataReceivedTracker(ControlLogicEventDataReceived& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	if(e.byteTo == HttpRequestManager::getContentLength(e.reqId) - 1) {
		ackActionRequestCompleted(HttpRequestManager::getContentId(e.reqId));
		// TODO: you downloaded a tracker. now decide what to do next.

		DBGMSG("Tracker is Downloaded");
		//Parse the Tracker
		XMLParserP2P::parseTracker(HttpRequestManager::getPldBytes(e.reqId), HttpRequestManager::getContentLength(e.reqId) - 1, peers, myId);
		//If no peers have been received switch to No_P2P mode
		if(peers.size() == 0)
		{
			mode = No_P2P;
			/* Start downloading Segments with lowest quality */
			nextRepId = bitRates.at(0);
			list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, 0));
			string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, 0));
			dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
			list<dash::URL> urls(1, segmentURL);
			list<HttpMethod> httpMethods(1, HttpMethod_GET);
			actions.push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
			for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
			{
				contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
			}
			lastdownloadAtServer = true;
			firstSegment = false;
			DBGMSG("Download From Server no P2P Mode");
		}
		else
		{
			//Set TCP Connection to each Peer in the Tracker File and Download MPDPeer
			mode = Downloading_PeerMPD;

			for (uint i=0; i < peers.size(); i++) {
				connId++;
				peers[i].setConnId(connId);
				list<const ContentId*> contentIds(1, new ContentIdMpdPeer(peers[i].getClientID()));
				dash::URL peerUrl(httpString, peers[i].getIP(),MpdPeerPath,MpdPeerFileName);
				actions.push_back(new ControlLogicActionCreateTcpConnection(connId, IfData(), peers[i].getIP(), 0));
				list<HttpMethod> httpMethods(1, HttpMethod_GET);
				list<dash::URL> urls (1, peerUrl);
				actions.push_back(new ControlLogicActionStartDownload(connId, contentIds, urls, httpMethods));
			}
		}
		Statistics::recordRequestStatistics(e.connId, e.reqId);
	}
	else
	{
		return actions;
	}
	return actions;
}


list<ControlLogicAction*> ControlLogicP2P::processEventDisconnect(const ControlLogicEventDisconnect& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	//dp2p_assert(e.connId == connId);

	list<ControlLogicAction*> actions;

	if(0 == ackActionDisconnect(e.connId)) {
		actions.push_back(new ControlLogicActionCloseTcpConnection(connId));
	}
	DBGMSG("the ConnId of disconnection :%i",e.connId);
	//Create a new connection
	//check if connection to server is lost
	if(e.connId == serverId)
	{
		serverId = ++connId;
		actions.push_back(new ControlLogicActionCreateTcpConnection(serverId, IfData(), mpdUrl.hostName, 0));
	}
	//check if the tracker has disconnected
	if(e.connId == trackerConnId)
		{
			trackerConnId = ++connId;
			actions.push_back(new ControlLogicActionCreateTcpConnection(trackerConnId, IfData(), mpdUrl.hostName, 0));
		}
	//check which Peers connection is lost
	else
	{
		uint k = 0;
		while(e.connId != peers[k].getConnId())
		{
			//DBGMSG("Im in loop for %u",k);
			//DBGMSG("checking next ConnId: %i",peers[k].getConnId());
			k++;
			dp2p_assert(k <= peers.size()-1);
		}
		peers[k].setConnId(++connId);
		//TODO check if Peer is GOOD before reconnect
		actions.push_back(new ControlLogicActionCreateTcpConnection(connId, IfData(), peers[k].getIP(), 0));
	}

	list<const ContentId*> contentIds;
	for(list<int>::const_iterator it = e.reqs.begin(); it != e.reqs.end(); ++it)
	{
		contentIds.push_back(HttpRequestManager::getContentId(*it).copy());
	}
	/*
	if(!contentIds.empty()) {
		actions.push_back(createActionDownloadSegments(contentIds, connId, HttpMethod_GET));
	}
	*/
	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::processEventStartPlayback(const ControlLogicEventStartPlayback& e)
{
	DBGMSG("Event: %s.", e.toString().c_str());

	list<ControlLogicAction*> actions;

	mpdUrl = e.mpdUrl;

	DBGMSG("MPD URL: %s", mpdUrl.hostName.c_str());

	/* Create a new TCP connection */
	actions.push_back(new ControlLogicActionCreateTcpConnection(serverId, IfData(), mpdUrl.hostName, 0));

	/* Start MPD download */
	list<const ContentId*> contentIds(1, new ContentIdMpd);
	list<dash::URL> urls(1, mpdUrl);
	list<HttpMethod> httpMethods(1, HttpMethod_GET);
	actions.push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));

	return actions;
}

list<ControlLogicAction*> ControlLogicP2P::actionRejectedStartDownload(ControlLogicActionStartDownload* a)
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
		//Frage ConnId
		actions.push_back(new ControlLogicActionCreateTcpConnection(++connId, IfData(), mpdUrl.hostName, 0));
		a->connId = connId;
		actions.push_back(a);
	}

	return actions;
}

//chooses the next Source and return the index in the Peer list of this source
ControlLogicP2P::decisionParameters ControlLogicP2P::decision(int segNr,list<ControlLogicAction*> *actions)
{
	//Generate a list of all Peers the Bandwith is known
	vector<int> indexPeersBandwithKnown;
	for(uint i= 0 ; i <= indexesGoodPeers.size()-1 ; i++)
	{
		double bandwith = peers[indexesGoodPeers[i]].getBandwith();
		if(bandwith != -1)
		{
			//DBGMSG("I know the Bandwith from Peer: %i",i);
			indexPeersBandwithKnown.push_back(i);
		}
	}
	//Desired Quality is the highest
	int runRepId = bitRates.size()-1;
	int desiredRepId = bitRates.at(runRepId);
	int bestPeerIndex = -1;
	//find RepId feasible from server
	int feasibleRepIdServer = findMattchingRepId(serverBandwith);
	if(indexPeersBandwithKnown.size() != 0)
	{
		//find best Peer
		double actualBestBandwith = 0;
		bool bestPeerFound = false;
		bool peerListEmpty = false;
		while(!bestPeerFound && !peerListEmpty)
		{
			desiredRepId = bitRates.at(runRepId);
			uint i = 0;
			//Loop over all peers the bandwith is known
			while(i < indexPeersBandwithKnown.size() && !peerListEmpty)
			{			desiredRepId = bitRates.at(runRepId);

				//Does the peer have the Segment
				//DBGMSG("the Index i want is: %i",indexesGoodPeers[indexPeersBandwithKnown[i]]);
				vector<Peer::segmentInformation>* segInfo = peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].getSegmentsInformation();
				uint k = 0;
				int repIdIndex = 0;
				bool peerHasSeg = false;
				while(k < segInfo->size())
				{
					if(segNr == segInfo->at(k).segment)
					{
						repIdIndex = k;
						k = segInfo->size();
						peerHasSeg = true;
					}
					k++;
				}
				//DBGMSG("Peer has segment %i:",(int) peerHasSeg);
				if(!peerHasSeg)
				{
					DBGMSG("This Peer doesn't have the desired Segment with no: %i. Updating his MPDPeer",(int) nextSegId);
					updatePeerMPD(indexesGoodPeers[indexPeersBandwithKnown[i]], actions);

					//MPDPeers should not be updated the whole process
					//we define a threshold at 3 updates in a row
					int counter = peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].getCounterOfMPDPeerUpdatesInARow();
					peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].setCounterOfMPDPeerUpdatesInARow(++counter);
					if(counter == 3)
					{
						//If the Bandwith is set to -1 the peer wont be taken in acount anymore
						DBGMSG("Delete Peer, updated 3 times in a row and no futher information.");
						peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].setBandwith(-1);
					}
					//delete Peer from list indexPeersBandwithKnown
					//because we don't have actual information about it
					indexPeersBandwithKnown.erase(indexPeersBandwithKnown.begin()+i);
					i--;
					if(indexPeersBandwithKnown.size() == 0)
					{
						peerListEmpty = true;
						DBGMSG("No futher information about peers. Start download from Server. Update of the MPDPeer requested");
						//define a threshold. if the bufferlevel is over the threshold, we can wait untill the PeerMPD is Updated.
						dash::Usec threshold = 10000000;
						if(actualBufferlevel > threshold)
						{
							//we wait return -2,-2
							struct decisionParameters returnParameters;
							returnParameters.Index = -2;
							returnParameters.repId = -2;
							return returnParameters;
						}
					}
				}
				else
				{
					//if he has the segment set his counter to 0
					DBGMSG("Comparing the Peer RepId: hes has %i, desired is %i",segInfo->at(repIdIndex).representationID,desiredRepId);
					peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].setCounterOfMPDPeerUpdatesInARow(0);
					//If the segment quality the peer has is equal to the desired quality
					DBGMSG("Comparing the Peer RepId: he has %i, desired is: %i",segInfo->at(repIdIndex).representationID, desiredRepId);
					if(desiredRepId == segInfo->at(repIdIndex).representationID)
					{
						//Check Bandwith to the Peer
						double peersBandwith = peers[indexesGoodPeers[indexPeersBandwithKnown[i]]].getBandwith();
						//DBGMSG("Peer bandwith is %lf:", peersBandwith);
						if(desiredRepId <= peersBandwith)
						{
							//Check if another peer should be prefered
							if(actualBestBandwith < peersBandwith)
							{
								//int bestPeerRepId = desiredRepId;
								bestPeerIndex = indexPeersBandwithKnown[i];
								bestPeerFound = true;
								actualBestBandwith = peersBandwith;
							}
						}
						else
						{
							//delete Peer from list indexPeersBandwithKnown
							//because his segment cant be afforded to be downloaded
							indexPeersBandwithKnown.erase(indexPeersBandwithKnown.begin()+i);
							i--;
							if(indexPeersBandwithKnown.size() == 0)
								peerListEmpty = true;
						}
					}
				}
				i++;
			}
			runRepId--;
		}
		//compare Peer to Server
		if((int) feasibleRepIdServer <= bitRates.at(++runRepId) && !peerListEmpty)
		{
			struct decisionParameters returnParameters;
			returnParameters.Index = bestPeerIndex;
			returnParameters.repId = bitRates.at(runRepId);
			return returnParameters;
		}
	}
	//If Server is the best source return -1
	struct decisionParameters returnParameters;
	returnParameters.Index = -1;
	returnParameters.repId = feasibleRepIdServer;
	return returnParameters;
}

void ControlLogicP2P::updatePeerMPD(int actualIndex, list<ControlLogicAction*> *actions)
{
//	list<ControlLogicAction*> actions;
	list<const ContentId*> contentIds(1, new ContentIdMpdPeer(peers[indexesGoodPeers[actualIndex]].getClientID()));
	dash::URL peerUrl(httpString, peers[actualIndex].getIP(),MpdPeerPath,MpdPeerFileName);
	list<HttpMethod> httpMethods(1, HttpMethod_GET);
	list<dash::URL> urls (1, peerUrl);
	actions->push_back(new ControlLogicActionStartDownload(peers[actualIndex].getConnId(), contentIds, urls, httpMethods));
}
void ControlLogicP2P::updateAllInterestingPeerMPDs(list<ControlLogicAction*> *actions)
{
	for (uint i=0; i < indexesGoodPeers.size(); i++) {
		list<const ContentId*> contentIds(1, new ContentIdMpdPeer(peers[indexesGoodPeers[index]].getClientID()));
		dash::URL peerUrl(httpString, peers[i].getIP(),MpdPeerPath,MpdPeerFileName);
		list<HttpMethod> httpMethods(1, HttpMethod_GET);
		list<dash::URL> urls (1, peerUrl);
		actions->push_back(new ControlLogicActionStartDownload(peers[indexesGoodPeers[i]].getConnId(), contentIds, urls, httpMethods));
	}
}

int ControlLogicP2P::findMattchingRepId(double bandwith)
{
	uint i=bitRates.size()-1;

	while(bandwith < (double)bitRates.at(i))
	{
		i--;
		if(i == 0)
			return bitRates.at(i);
	}
	int ret = bitRates.at(i);
	return ret;
}

int ControlLogicP2P::findNextLowerRepId(int repid)
{
	uint i=bitRates.size()-1;

	while((int) repid < bitRates.at(i))
	{
		i--;
		if(i == 0)
			return bitRates.at(i);
	}
	int ret = bitRates.at(--i);
	return ret;
}
int ControlLogicP2P::modeToInt()
{
	switch(mode)
	{
		case Server_Measurment: return 5;
		case Peer_Measurment: return 4;
		case Streaming: return 3;
		case Downloading_PeerMPD: return 2;
		case No_P2P: return -1;
		case init: return 1;
		default: return -99;
	}
}

void ControlLogicP2P::downloadSegment(list<ControlLogicAction*> *actions)
{
	bool actualDownloadWasAtServer = lastdownloadAtServer;
	//If the last Fetchtime is to high the bandwith to the peer is to bad to download
	if(mode == Server_Measurment || mode == Peer_Measurment)
	{
		if(actualFetchTime > segmentDuraton && !lastdownloadAtServer)
		{
			DBGMSG("The Bandwith to the Peer was to bad. Deleted It");
			indexesGoodPeers.erase(indexesGoodPeers.begin()+index);
			firstSegment = true;
		}
	}
	if(mode == Server_Measurment || mode == No_P2P)
	{
		if(mode == Server_Measurment)
			DBGMSG("Server Measurement");
		else
			DBGMSG("No Peers Found. Switched to No P2P Mode");
		//Save the actual Bandwith to the Server
		if(!firstSegment) {
			/*TODO: insert right connection ID here */
			serverBandwith=Statistics::getThroughputLastRequest(0);
		}
		//find the next Quality just considering the Abndwith from the download before
		nextRepId = findMattchingRepId(serverBandwith);
		DBGMSG("Actual Bandwidth to Server: %lf. Selected next Segment bandwidth: %d.The next segment is SegNo: %d",serverBandwith, nextRepId, nextSegId);
		//Download the next segment from the Server
		list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, nextSegId));
		string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, nextSegId));
		dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
		list<dash::URL> urls(1, segmentURL);
		list<HttpMethod> httpMethods(1, HttpMethod_GET);
		actions->push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
		for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
		{
			contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
		}
		//Check if tcp Slowstart is over
		if(!firstSegment)
		{
			if(previousFetchTime >= actualFetchTime)//-20ms Range
				tcpSlowStartServerOver = true;
		}
		//If tcp Slowstart is over prepare for the determiation of the quality level expected from Server
		if(tcpSlowStartServerOver)
		{
			countServerSegmentsAfterTcpSlowStart++;
			expectedServerRepId = expectedServerRepId + lastSegmentsBitrate;
		}
		if(!firstSegment)
			previousFetchTime = actualFetchTime;
		firstSegment = false;
		lastdownloadAtServer = true;
	}
	if(mode == Peer_Measurment)
	{
		DBGMSG("In Peer Measurement Mode");
		//If it is the fist time we step in here we will determine all peer worthy to download from
		//for this the mean Quality from every Peer is compared with the Quality we expect from the Server
		if(firstPeermeasurement)
		{
			//expectedServerRepId = findMattchingRepId(serverBandwith);
			DBGMSG("Expected average bandwith to Server: %d", (int)expectedServerRepId);
			//select possible sources
			for(uint i = 0 ; i < peers.size();i++)
			{
				//Calculate mean repId
				vector<Peer::segmentInformation>* segInfo = peers[i].getSegmentsInformation();
				double meanQuality = 0;
				uint k;
				for(k = segmentsAvoidedInPeerMeasurement ; k < segInfo->size();k++)
				{
					meanQuality += segInfo->at(k).representationID;
				}
				meanQuality = meanQuality/(segInfo->size()-segmentsAvoidedInPeerMeasurement);
				DBGMSG("Mean quality of Peer is %lf", meanQuality);
				//To not disselect peers which may have just some bad segments
				//we set the threshold to one repID lower than the expectedRepId
				int theshId = findNextLowerRepId(expectedServerRepId);
				DBGMSG("The Threshold for the good peers is %i",(int) theshId);
				if(meanQuality >= (double) theshId)
				{
					indexesGoodPeers.push_back(i);

				}
				else
				{
					actions->push_back(new ControlLogicActionCloseTcpConnection(peers[i].getConnId()));
				}
			}
			firstPeermeasurement = false;
			DBGMSG("Choose the Peers worthy to download. They are %i Peers", indexesGoodPeers.size());
		}
		//Check if there are peer worthy to download from
		if(indexesGoodPeers.size()>0)
		{
			bool peerCompleted = false;
			DBGMSG("previous Fetchtime: %lf actualt fetchtime:%lf FirstSegment: %d",previousFetchTime,actualFetchTime, (int) firstSegment);
			//Check if his TCP slow start phase is over
			if(previousFetchTime >= actualFetchTime)//-20ms Range
			{
				//reset the Fetchtime
				previousFetchTime = 0.0;
				//set the determined bandwidth of the peer
				//TODO: use propeor connection ID in the next line
				peers[indexesGoodPeers[index]].setBandwith(Statistics::getThroughputLastRequest(0));
				DBGMSG("Finished the Measurement of Peer: %s actual Bandwith is :%lf",peers[indexesGoodPeers[index]].getClientID().toString().c_str(),peers[indexesGoodPeers[index]].getBandwith());
				//select the next peer
				index++;
				peerCompleted = true;
				firstSegment = true;
			}
			if(!firstSegment)
				previousFetchTime = actualFetchTime;
			//Check if all peers are measured and if go to streaming mode
			if(index == indexesGoodPeers.size() && peerCompleted)
			{
				mode = Streaming;
				DBGMSG("All Peers Measured, switched to streaming mode");
				peersMeasured = true;
				index--;
			}
			else
			{
				//Check if the peer has the segment
				vector<Peer::segmentInformation>* segInfo = peers[indexesGoodPeers[index]].getSegmentsInformation();
				uint k = 0;
				int repIdIndex =-1;
				while(k < segInfo->size())
				{
					//DBGMSG("k ist %u:",k);
					if(nextSegId == segInfo->at(k).segment)
					{
						repIdIndex = k;
						k = segInfo->size();
					}
					k++;
				};
				//if peer has the segment download the segment
				if(repIdIndex != -1)
				{
					stringstream segNr, repId;
					segNr << nextSegId;
					repId << segInfo->at(repIdIndex).representationID;
					string filename = segNr.str() + "-" + repId.str();
					dash::URL peerUrl("http://",peers[indexesGoodPeers[index]].getIP(),peers[indexesGoodPeers[index]].getclientBaseURL(),filename);
					nextRepId = segInfo->at(repIdIndex).representationID;
					list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, nextSegId));
					list<HttpMethod> httpMethods(1, HttpMethod_GET);
					list<dash::URL> peerUrls (1,peerUrl);
					actions->push_back(new ControlLogicActionStartDownload(peers[indexesGoodPeers[index]].getConnId(), contentIds, peerUrls, httpMethods));
					DBGMSG("peer has ConId: %i",connId);
					DBGMSG("Downloading next Segment with No: %d from Peer: %s ",nextSegId, peers[indexesGoodPeers[index]].getClientID().toString().c_str());
					for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
					{
						contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
					}
					firstSegment = false;
					lastdownloadAtServer = false;
				}
				//if peer hasnt the segment update the MPDPeer
				else
				{
					if(firstMDPPeerUpdate)
					{
						DBGMSG("need to update the MPDPeer to measure it");
						updatePeerMPD(indexesGoodPeers[index], actions);
						firstMDPPeerUpdate = false;
					}
				}
			}
		}
		else
		{
			DBGMSG("No worthy Peers Found. Switched to No P2P Mode");
			mode = No_P2P;
			if(!firstSegment) {
				// TODO: use propoer connection ID in the next line
				serverBandwith=Statistics::getThroughputLastRequest(0);
			}
			nextRepId = findMattchingRepId(serverBandwith);
			DBGMSG("Actual Bandwidth to Server: %lf. Selected next Segment bandwidth: %d.The next segment is SegNo: %d",serverBandwith, nextRepId, nextSegId);
			list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, nextRepId, nextSegId));
			string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, nextRepId, nextSegId));
			dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
			list<dash::URL> urls(1, segmentURL);
			list<HttpMethod> httpMethods(1, HttpMethod_GET);
			actions->push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
			for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
			{
				contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
			}
			previousFetchTime = actualFetchTime;
			lastdownloadAtServer = true;
			firstSegment = false;

		}
	}
	if(mode == Streaming)
	{
		DBGMSG("In streaming mode now");
		//SetBandwith
		if (lastdownloadAtServer)
		{
			// TODO: use propoer connection ID in the next line
			serverBandwith=Statistics::getThroughputLastRequest(0);
		}
		else
		{
			// TODO: use propoer connection ID in the next line
			peers[indexesGoodPeers[index]].setBandwith(Statistics::getThroughputLastRequest(0));
		}
		//Decide which will be the next source
		struct decisionParameters decisionResults = decision(nextSegId, actions);// returns the index and the RepID of the Best Peer
		//check if we wait
		if(decisionResults.Index == -2 && decisionResults.repId == -2)
		{
			waitForMPDPeerUpdate = true;
		}
		else
		{
			nextRepId = decisionResults.repId;
			//Server is best source i = -1
			if(decisionResults.Index == -1)
			{
				DBGMSG("Made a decision. Download next segment with No: %i  from Server",(int) nextSegId);
				list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, decisionResults.repId, nextSegId));
				string stringSegmentURl = mpdWrapper->getSegmentURL(ContentIdSegment(0, 0, decisionResults.repId, nextSegId));
				dash::URL segmentURL = Utilities::splitURL(stringSegmentURl);
				list<dash::URL> urls(1, segmentURL);
				list<HttpMethod> httpMethods(1, HttpMethod_GET);
				actions->push_back(new ControlLogicActionStartDownload(serverId, contentIds, urls, httpMethods));
				for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
				{
					contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
				}
				lastdownloadAtServer = true;
			}
			else
			{
				DBGMSG("Made a decision. Download next segment with No: %i from %s",(int) nextSegId,peers[indexesGoodPeers[decisionResults.Index]].getClientID().toString().c_str());
				index = decisionResults.Index;
				stringstream segNr, repId;
				segNr << nextSegId;
				repId << decisionResults.repId;
				if(repId.str().compare("-1") != 0)
				{
					string filename = segNr.str() + "-" + repId.str();
					dash::URL peerUrl("http://",peers[indexesGoodPeers[index]].getIP(),peers[indexesGoodPeers[index]].getclientBaseURL(),filename);
					list<const ContentId*> contentIds(1, new ContentIdSegment(0, 0, decisionResults.repId, nextSegId));
					list<HttpMethod> httpMethods(1, HttpMethod_GET);
					list<dash::URL> peerUrls (1,peerUrl);
					actions->push_back(new ControlLogicActionStartDownload(peers[indexesGoodPeers[index]].getConnId(), contentIds, peerUrls, httpMethods));
					for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it)
					{
						contour.setNext(dynamic_cast<const ContentIdSegment&>(**it));
					}
					lastdownloadAtServer = false;
				}
				else
				{
					exit(0);
					DBGMSG("Error! Didnt find any source");
				}
			}
		}
	}
	//Record the measurement to file:
	if(measurementEnable)
	{
		int modeInt = modeToInt();
		if(actualDownloadWasAtServer)
			Statistics::recordP2PMeasurementToFile(pathMeasurmentFile,nextSegId,
					nextRepId, -1,serverBandwith, modeInt, actualFetchTime);
		else
		{
			//find the peerindex
			Statistics::recordP2PMeasurementToFile(pathMeasurmentFile,nextSegId,
					nextRepId, index,peers[indexesGoodPeers[index]].getBandwith(), modeInt,actualFetchTime);
		}
	}
}

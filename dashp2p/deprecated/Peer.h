/****************************************************************************
 * Peer.h			                                                        *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 29.11.2012v 29, 2012                                     *
 * Authors: 			                                    				*
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

#ifndef PEER_H_
#define PEER_H_

//Includes
#include <string>
#include <vector>
#include "PeerId.h"
using std::vector;
using std::string;

class Peer {

public:
	//Peer( );//string clientID, int segments, int representationID, double bandwith, int numberOfSegments);
	virtual ~Peer();
	Peer(PeerId clientID, string IP, int port);

	//Set functions
	void setPeerID(PeerId clientID);
	void setclientBaseURL(string clientBaseURL);
	void setBandwith(double bandwith);
	void setIP(string IP);
	void setConnId(int  connId);
	void setPort(int port);
	void setCounterOfMPDPeerUpdatesInARow(int counterOfMPDPeerUpdatesInARow);

	//add functions
	void addSegmentInformations(int segment, int representationID);

	//Get functions
	PeerId getClientID();
	string getclientBaseURL();
	double getBandwith();
	string getIP();
	int getPort();
	int getConnId();
	int getCounterOfMPDPeerUpdatesInARow();

	//Struct of segment Information
	struct segmentInformation { int segment ; int representationID ; };

	// Segment Information Vector functions
	vector<struct segmentInformation>* getSegmentsInformation();
	void clearsegmentInformationVector();
	void deleteFromBeginTillsegmentInformationVector(int end);

private:
	//Variables
	PeerId clientID;
	string IP;
	int port;
	string clientBaseURL;
	double bandwith;
	int connId;
	vector<struct segmentInformation> segmentVector;
    int counterOfMPDPeerUpdatesInARow;
};

#endif /* PEER_H_ */

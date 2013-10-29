/****************************************************************************
 * Peer.cpp			                                                        *
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

//Includes:
#include "Peer.h"
#include "Utilities.h"
//Constructor
/*PeerAttribute::PeerAttribute()
{
	// TODO Auto-generated constructor stub

}*/
Peer::Peer(PeerId clientID, string IP, int port)
:clientID(clientID)
{
	// TODO Auto-generated constructor stub
	this->clientID = clientID;
	this->IP = IP;
	this->port = port;
	bandwith = -1;
	counterOfMPDPeerUpdatesInARow = 0;
}
//Descructor
Peer::~Peer()
{
	// TODO Auto-generated destructor stub
}
//******************************************************************
//Set methodes
void Peer::setPeerID(PeerId clientID)
{
	this->clientID = clientID;
}
void Peer::setclientBaseURL(string clientBaseURL)
{
	this->clientBaseURL = clientBaseURL;
}
void Peer::setBandwith(double bandwith)
{
	this->bandwith = bandwith;
}
void Peer::setIP(string IP)
{
	this->IP = IP;
}
void Peer::setPort(int port)
{
	this->port = port;
}
void Peer::setConnId(int  connId)
{
	this->connId = connId;
}
void Peer::setCounterOfMPDPeerUpdatesInARow(int counterOfMPDPeerUpdatesInARow)
{
	this->counterOfMPDPeerUpdatesInARow = counterOfMPDPeerUpdatesInARow;
}
//******************************************************************
//add methods
void Peer::addSegmentInformations(int segment, int representationID)
{
	struct segmentInformation actualSegments;
	actualSegments.segment = segment;
	actualSegments.representationID = representationID;
	segmentVector.push_back(actualSegments);
}
//******************************************************************
//Get Methods
PeerId Peer::getClientID()
{
	return clientID;
}
string Peer::getclientBaseURL()
{
	return clientBaseURL;
}

vector<Peer::segmentInformation>* Peer::getSegmentsInformation()
{
	return &segmentVector;
}

double Peer::getBandwith()
{
	return bandwith;
}
string Peer::getIP()
{
	return IP;
}
int Peer::getPort()
{
	return port;
}
int Peer::getConnId()
{
	return connId;
}
int Peer::getCounterOfMPDPeerUpdatesInARow()
{
	return counterOfMPDPeerUpdatesInARow;
}
//******************************************************************
//Vector Control
void Peer::clearsegmentInformationVector()
{
	segmentVector.clear();
}
void Peer::deleteFromBeginTillsegmentInformationVector(int end)
{
	segmentVector.erase(segmentVector.begin(),segmentVector.begin()+end);
}
//******************************************************************

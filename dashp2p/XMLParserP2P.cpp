/****************************************************************************
 * XMLParserP2P.cpp                                                         *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Dec 03, 2012                                                 *
 * Authors: Benjamin Barth, Farid Rosli                                     *
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


#include "XMLParserP2P.h"
#include "Utilities.h"


void XMLParserP2P::parseTracker(const char* content, int64_t size, vector<Peer> &peers, string myId)
{
	DBGMSG("Parsing the Tracker");
	xmlDocPtr doc; /* the resulting document tree */
	xmlNodePtr cur;
	/*
	 * The document being in memory, it have no base per RFC 2396,
	 * and the "noname.xml" argument will serve as its base.
	 */
	doc = xmlReadMemory(content, size, "noname.xml", NULL, 0);
	if (doc == NULL)
	{
		DBGMSG("Failed to parse document");
	}
	cur = xmlDocGetRootElement(doc);
	//Check if it is a Tracker XML
	if(cur->name != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "Tracker"))
		{
			cur = cur->xmlChildrenNode;

			//Retrieving Attribute Values
			while (cur != NULL)
			{
				if ((!xmlStrcmp(cur->name, (const xmlChar *) "Peer")))
				{
					char* charID = (char*)xmlGetProp(cur,(const xmlChar*) "ID");
					PeerId clientID = PeerId(string(charID));
					xmlFree(charID);
					char* clientIP = (char*)xmlGetProp(cur,(const xmlChar*) "IP");
					if(strcmp(clientID.toString().c_str(),myId.c_str()) != 0)
					{
						dash::Utilities::PeerInformation peerInfo = dash::Utilities::splitIPStringMPDPeer(clientIP);
						//Generate a new Peer object and save it in the list
						Peer* temp = new Peer(clientID, peerInfo.ip, peerInfo.port);
						peers.push_back(*temp);
						DBGMSG("Received ClientID : %s",clientID.toString().c_str());
						DBGMSG("Received PeerIP : %s at Port: %i",temp->getIP().c_str(),temp->getPort());
						delete temp;
						temp = NULL;
					}
					else
					{
						DBGMSG("Receveid my own ID in Tracker");
					}
					xmlFree(clientIP);
				}
				cur = cur->next;
			}
		}
		else
		{
			DBGMSG("No valid Tracker XML");
		}
		delete cur;
		cur = NULL;
		xmlFreeDoc(doc);
	}
	else
	{
		DBGMSG("No valid Tracker XML, P2P Mode disabled");
	}
}


void XMLParserP2P::parseMPDPeer(const char* content, int64_t size, vector<Peer> &peers)
{
	xmlDocPtr doc; /* the resulting document tree */
	xmlNodePtr cur;
	int vectorIndex=0;
	/*
	 * The document being in memory, it have no base per RFC 2396,
	 * and the "noname.xml" argument will serve as its base.
	 */
	doc = xmlReadMemory(content, size, "noname.xml", NULL, 0);
	if (doc == NULL)
	{
		DBGMSG("Failed to parse document");
	}
	cur = xmlDocGetRootElement(doc);
	//XMLParserP2P::print_element_names(cur);
	if(cur->name != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "mpdId"))
		{
			cur = cur->children;
			while (cur != NULL)
			{
				if ((!xmlStrcmp(cur->name, (const xmlChar *) "segment")))
				{
					//add the segmentsinformation stored in the MPDpeer
					xmlChar* segmentNo = xmlGetProp(cur,(const xmlChar*) "No");
					xmlChar* representationID = xmlGetProp(cur, (const xmlChar *) "RepresentationID");
					int seg = atoi((char*)segmentNo);
					int rep = atoi((char*)representationID);
					peers[vectorIndex].addSegmentInformations(seg , rep);
					DBGMSG("Peer has: %s worth: %s", segmentNo, representationID);
					xmlFree(segmentNo);
					segmentNo = NULL;
					xmlFree(representationID);
					representationID = NULL;
				}
				if ((!xmlStrcmp(cur->name, (const xmlChar *) "client")))
				{
					//find the right peer in the list and set the baseurl
					xmlChar* clientID = xmlGetProp(cur, (const xmlChar *) "ID");
					xmlChar* clientURL = xmlGetProp(cur, (const xmlChar *) "baseUrl");
					char* compare = NULL;
					while(xmlStrcmp(clientID, (const xmlChar*) compare) != 0)
					{
						delete compare;
						compare = NULL;
						compare = new char[peers[vectorIndex].getClientID().toString().length()+1];
						strcpy(compare,peers[vectorIndex].getClientID().toString().c_str());
						vectorIndex++;
					}
					DBGMSG("Update data from peer: %s baseURL: %s, actual peers has index: %i", clientID, clientURL,vectorIndex-1);
					xmlFree(clientID);
					delete compare;
					compare = NULL;
					--vectorIndex;
					peers[vectorIndex].setclientBaseURL(string((char*)clientURL));
					peers[vectorIndex].clearsegmentInformationVector();
					xmlFree(clientURL);
				}
				cur = cur->next;
			}
		}
	}
	else
	{
		DBGMSG("No valid PeerMPD");
	}
	delete cur;
	xmlFreeDoc(doc);
}


void XMLParserP2P::createMPDPeer(string segNr, string repId, const char* mpdId, xmlDocPtr pDoc, string myId, string myBaseUrl, string pathToMpdPeer)
{
	xmlNodePtr root_node = NULL;

	pDoc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "mpdId");
	xmlNewProp(root_node, BAD_CAST "ID", BAD_CAST mpdId);
	xmlDocSetRootElement(pDoc, root_node);

	xmlNodePtr pNode1 = xmlNewNode(0, BAD_CAST "client");
	xmlNewProp(pNode1, BAD_CAST "ID", BAD_CAST myId.c_str());
	xmlNewProp(pNode1, BAD_CAST "baseUrl", BAD_CAST myBaseUrl.c_str());
	xmlAddChild(root_node, pNode1);

	xmlNodePtr pNode2 = xmlNewNode(0, BAD_CAST "segment");
	xmlNewProp(pNode2, BAD_CAST "No", BAD_CAST segNr.c_str());
	xmlNewProp(pNode2, BAD_CAST "RepresentationID", BAD_CAST repId.c_str());
	xmlAddChild(root_node, pNode2);

	xmlSaveFormatFileEnc(pathToMpdPeer.c_str(), pDoc, "UTF-8", 1);

	xmlFreeDoc(pDoc);
}


void XMLParserP2P::updateMPDPeer(string segNr, string repId, xmlDocPtr pDoc, string pathToMpdPeer)
{
	xmlNodePtr root_node = NULL;

	root_node = xmlDocGetRootElement(pDoc);
	xmlNodePtr pNewNode = xmlNewNode(0, (xmlChar*)"segment");

	xmlNewProp(pNewNode, BAD_CAST "No", BAD_CAST segNr.c_str());
	xmlNewProp(pNewNode, BAD_CAST "RepresentationID", BAD_CAST repId.c_str());
	xmlAddChild(root_node, pNewNode);

	xmlSaveFileEnc(pathToMpdPeer.c_str(), pDoc, "UTF-8");

	xmlFreeDoc(pDoc);
}


void XMLParserP2P::print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            DBGMSG("node type: Element, name: %s\n", cur_node->name);
        }

        print_element_names(cur_node->children);
    }
}

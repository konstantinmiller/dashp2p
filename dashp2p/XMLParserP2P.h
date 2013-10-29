/****************************************************************************
 * XMLParserP2P.h                                                           *
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

#ifndef XMLPARSERP2P_H_
#define XMLPARSERP2P_H_


#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <DebugAdapter.h>
#include <vector>
#include <Peer.h>
#include <cstring>
using namespace std;


class XMLParserP2P
{

public:
	static void parseTracker(const char* content, int64_t size, vector<Peer> &peers, string myId);
	static void parseMPDPeer(const char* content, int64_t size, vector<Peer> &peers);
	static void createMPDPeer(string segNr, string repId, const char* mpdId, xmlDocPtr pDoc, string myId, string myBaseUrl, string pathToMpdPeer);
	static void updateMPDPeer(string segNr, string repId, xmlDocPtr pDoc, string pathToMpdPeer);

	//just for debugging porpuses
	static void print_element_names(xmlNode * a_node);

private:
	XMLParserP2P(){}
	virtual ~XMLParserP2P(){}
};


#endif /* XMLPARSERP2P_H_ */

/****************************************************************************
 * SourceManager.cpp                                                        *
 ****************************************************************************
 * Copyright (C) 2013 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Oct 25, 2013                                                 *
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

#include "SourceManager.h"
#include "DebugAdapter.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace dashp2p {

vector<SourceData*> SourceManager::srcVec;

SourceData::SourceData(const string& hostName, const int& port)
  : hostName(hostName), port(port), keepAliveMax(-1), keepAliveTimeout(-1), hostAddr()
{
	/* Resolve the address */
	struct addrinfo* result = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	char tmpChar[64];
	sprintf(tmpChar, "%d", port);
	dp2p_assert(0 == getaddrinfo(hostName.c_str(), tmpChar, &hints, &result) && result);
	memcpy(&hostAddr, result->ai_addr, sizeof(hostAddr));
	freeaddrinfo(result);
}

int SourceManager::add(const string& hostName, const int& port)
{
	SourceData* d = new SourceData(hostName, port);
	srcVec.push_back(d);

	return srcVec.size() - 1;
}

string SourceManager::sourceState2String(const int& srcId)
{
	const SourceData& sd = *srcVec.at(srcId);

	string ret;
	char tmp[2048];
	if(sd.keepAliveMax == -1) {
		dp2p_assert(sd.keepAliveTimeout == -1);
		sprintf(tmp, "Unknown.");
	} else {
		sprintf(tmp, "request limit: %d, timeout: %gs.", sd.keepAliveMax, sd.keepAliveTimeout / 1e6);
	}
	ret.append(tmp);
	return ret;
}

void SourceManager::cleanup()
{
	for(std::size_t i = 0; i < srcVec.size(); ++i)
		delete srcVec.at(i);
	srcVec.clear();
}

} /* namespace dashp2p */

/****************************************************************************
 * SourceManager.h                                                          *
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

#ifndef SOURCEMANAGER_H_
#define SOURCEMANAGER_H_

#include <netinet/in.h>
#include <vector>
#include <string>
using std::vector;
using std::string;

namespace dashp2p {

class SourceData
{
public: /* public methods */
	SourceData(const string& hostName, const int& port);
	virtual ~SourceData(){}
public: /* public fields */
	const string hostName;
	const int port;
	int keepAliveMax;
	int64_t keepAliveTimeout;
	struct sockaddr_in hostAddr;
};

class SourceManager
{
public:
	static void cleanup();
	static int add(const string& hostName, const int& port = 80);
	static SourceData& get(const int& srcId) {return *srcVec.at(srcId);}
	static string sourceState2String(const int& srcId);

private:
	SourceManager(){}
	virtual ~SourceManager(){}

private:
	static vector<SourceData*> srcVec;
};

} /* namespace dashp2p */
#endif /* SOURCEMANAGER_H_ */

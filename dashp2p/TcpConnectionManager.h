/****************************************************************************
 * TcpConnectionManager.cpp                                                 *
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

#ifndef TCPCONNECTIONMANAGER_H_
#define TCPCONNECTIONMANAGER_H_

#include "dashp2p.h"

#include <netinet/tcp.h>
#include <vector>
using std::vector;
using std::pair;

namespace dashp2p {

class TcpConnection
{
public: /* public methods */
	TcpConnection(const int& srcId, const int& port, const IfData& ifData, const int& maxPendingRequests);
	virtual ~TcpConnection();
	void connect();
	void disconnect();
	int read();
	void write(string& s);
	void updateTcpInfo();
	void assertSocketHealth() const;
	pair<int,int> getSocketBufferLengths() const;
	int state() const;
	string getIfString() const {return ifData.toString();}
public: /* public fields */
	const int srcId;
	const int port;
	IfData ifData;
	int maxPendingRequests;
    int fdSocket;
    struct tcp_info lastTcpInfo;
    //int numConnectEvents;
    int numReqsCompleted;
    int keepAliveMaxRemaining;
    int64_t keepAliveTimeoutNext;
    bool aHdrReceived;

    /* Buffer for reading data from the socket. */
    const int32_t recvBufSize = 10 * 1048576;
    int recvBufContent;
    char* recvBuf;
    int64_t recvTimestamp;
};

class TcpConnectionManager
{
public:
	static void cleanup();
	static int create(const int& srcId, const int& port = 80, const IfData& ifData = IfData(), const int& maxPendingRequests = 0);
	static TcpConnection& get(const int& connId) {return *connVec.at(connId);}
	static void logTCPState(const int& connId, const char* reason);
    static string tcpState2String(int tcpState);
    static string tcpCAState2String(int tcpCAState);

private:
	TcpConnectionManager(){}
	virtual ~TcpConnectionManager(){}

private:
	static vector<TcpConnection*> connVec;
};

} /* namespace dashp2p */
#endif /* TCPCONNECTIONMANAGER_H_ */

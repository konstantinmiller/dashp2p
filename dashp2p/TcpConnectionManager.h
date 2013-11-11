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
#include "SourceManager.h"

#include <netinet/tcp.h>
#include <vector>
using std::vector;
using std::pair;

namespace dashp2p {

// class TcpConnectionId
class TcpConnectionId {
public:
    TcpConnectionId(const int& id = -1): id(id) {}
    bool operator==(const TcpConnectionId& other) const {return other.id == id;}
    bool operator!=(const TcpConnectionId& other) const {return !(other == *this);}
    int numeric() const {return id;} // TODO: replace by casting operator
    operator string() const {return std::to_string(id);}
private:
    int id;
};

//class TcpConnectionManager;

// not thread safe
class TcpConnection
{
public: /* public methods */
	TcpConnection(const SourceId& srcId, const int& port, const IfData& ifData, const int& maxPendingRequests, const int64_t& connectTimeout);
	virtual ~TcpConnection();
	int connect(); // returns 0 of OK. Can be called again if fails.
	int read();
	void write(string& s);
	void updateTcpInfo();
	void assertSocketHealth() const;
	pair<int,int> getSocketBufferLengths() const;
	int state() const;
	string getIfString() const {return ifData.toString();}
private:
	void disconnect();
public: /* public fields */
	const SourceId srcId;
	const int port;
	IfData ifData;
	int maxPendingRequests;
	const int64_t connectTimeout;
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

/* friends */
    friend class TcpConnectionManager;
};

class TcpConnectionManager
{
public:
	static void cleanup();
	// Creates a TCP connection in unconnected state
	static int create(const SourceId& srcId, const int& port = 80, const IfData& ifData = IfData(), const int& maxPendingRequests = 0, const int64_t& connectTimeout = 60000000);
	static int connect(const int& connId) {return connVec.at(connId)->connect();}
	static void disconnect(const TcpConnectionId& tcpConnectionId);
	static TcpConnection& get(const TcpConnectionId& tcpConnectionId) {return *connVec.at(tcpConnectionId.numeric());}
	static void logTCPState(const TcpConnectionId& tcpConnectionId, const char* reason);
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

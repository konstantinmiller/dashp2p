/****************************************************************************
 * TcpConnectionManager.h                                                   *
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

#include "TcpConnectionManager.h"
#include "SourceManager.h"
#include "DebugAdapter.h"
#include "Utilities.h"
#include "Statistics.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

namespace dashp2p {

// static variables in TcpConnectionId
//int TcpConnectionId::nextId = 0;

// static variables in TcpConnectionManager
vector<TcpConnection*> TcpConnectionManager::connVec;

TcpConnection::TcpConnection(const SourceId& srcId, const int& port, const IfData& ifData, const int& maxPendingRequests, const int64_t& connectTimeout)
  : srcId(srcId),
    port(port),
    ifData(ifData),
    maxPendingRequests(maxPendingRequests),
    connectTimeout(connectTimeout),
    fdSocket(-1),
    lastTcpInfo(),
    /*numConnectEvents(0),*/
    numReqsCompleted(0),
    keepAliveMaxRemaining(-1),
    keepAliveTimeoutNext(-1),
    aHdrReceived(false),
    recvBufContent(0),
    recvBuf(nullptr),
    recvTimestamp(-1)
{
    /* Open the socket. */
    // TODO: increase outoing buffer size to something around 1 MB or more
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    dp2p_assert(fdSocket > 0);

    /* TODO: If want to increase socket buffer sizes, must do it here, before connect! */

    /* Bind the socket */
    if(ifData.initialized) {
        dp2p_assert(0 == bind(fdSocket, (struct sockaddr*)&ifData.sockaddr_in, sizeof(ifData.sockaddr_in)));
        DBGMSG("Binded socket to %s:%d.", inet_ntoa(ifData.sockaddr_in.sin_addr), ifData.sockaddr_in.sin_port);
    }
}

TcpConnection::~TcpConnection()
{
	disconnect();
}

int TcpConnection::connect()
{
    dp2p_assert(fdSocket > 0);

	/* Connect. */
	const int64_t tic = Utilities::getAbsTime();
	const int retVal = ::connect(fdSocket, (struct sockaddr*)&SourceManager::get(srcId).hostAddr, sizeof(SourceManager::get(srcId).hostAddr));
	char _errBuf[1024];
	char *errBuf = strerror_r(errno, _errBuf, sizeof(_errBuf)); // get the error string
	const int64_t toc = Utilities::getAbsTime();
	DBGMSG("Spent %gs in connect().", (toc - tic) / 1e6);
	if(retVal) {
		WARNMSG("Could not connect to %s. Error in connect(): %s", SourceManager::get(srcId).hostName.c_str(), errBuf);
		return 1;
	}

	/* Allocate buffer for reading from the socket */
	recvBuf = new char[recvBufSize];
	dp2p_assert(recvBuf);

	//++ numConnectEvents;
	numReqsCompleted = 0;
	if(SourceManager::get(srcId).keepAliveTimeout != -1)
		keepAliveTimeoutNext = Utilities::getAbsTime() + SourceManager::get(srcId).keepAliveTimeout;

	updateTcpInfo();
	dp2p_assert(lastTcpInfo.tcpi_state == TCP_ESTABLISHED);
	keepAliveMaxRemaining = SourceManager::get(srcId).keepAliveMax;

	const pair<int,int> socketBufferLengths = this->getSocketBufferLengths();
	DBGMSG("Socket snd buf size: %d, socket rcv buf size: %d.", socketBufferLengths.first,socketBufferLengths.second);

	return 0;
}

int TcpConnection::read()
{
	dp2p_assert_v(recvBufContent == 0, "recvBufContent: %" PRId32, recvBufContent);

	/* Log TCP state. Here, even if the TCP state is not TCP_ESTABLISHED, we call recv().
	 * We do it since we know that there are data available due to the return value of select(). */
	//logTCPState("before recv()");

	/* Read from the socket */
	const pair<int,int> socketBufferLengths = this->getSocketBufferLengths(); // TODO: remove after debugging
	dp2p_assert_v(socketBufferLengths.second / 2 <= (int)recvBufSize, "Socket rcv buf size: %d, recv buf size: %d.", socketBufferLengths.second, recvBufSize);
	recvBufContent = recv(fdSocket, recvBuf, recvBufSize, 0);
	if(recvBufContent == -1) {
		perror("select()");
		throw std::runtime_error("Error reading from socket.");
	}
	dp2p_assert_v(recvBufContent < (int)recvBufSize, "recvBufContent: %d, recvBufSize: %u", recvBufContent, recvBufSize);

	recvBuf[recvBufContent] = 0;
	recvTimestamp = Utilities::getTime();

	/* Check the health of the socket and log TCP state. */
	//assertSocketHealth();
	//logTCPState("after recv()");

	DBGMSG("Received %d bytes.", recvBufContent);

	return recvBufContent;
}

void TcpConnection::write(string& s)
{
	/* Assert socket health and health of TCP connection. */
	//dp2p_assert(fdSocket != -1);
	//assertSocketHealth();
	updateTcpInfo();
	if(lastTcpInfo.tcpi_state != TCP_ESTABLISHED) {
		ERRMSG("Wanted to send requests but TCP connection is %s.", TcpConnectionManager::tcpState2String(lastTcpInfo.tcpi_state).c_str());
		ERRMSG("Server state: %s", SourceManager::sourceState2String(srcId).c_str());
		//ERRMSG("reqQueue: %s", reqQueue2String().c_str());
		throw std::runtime_error("Unexpected termination of TCP connection.");
	}

	/* Send the request. */
	while(!s.empty())
	{
		int retVal = ::send(fdSocket, s.c_str(), s.size(), 0);//MSG_DONTWAIT);
		// TODO: react to connectivity interruptions here
		dp2p_assert(retVal > 0);
		s.erase(0, retVal);
	}
}

void TcpConnection::updateTcpInfo()
{
    dp2p_assert(fdSocket > 0);
    memset(&lastTcpInfo, 0, sizeof(lastTcpInfo));
    socklen_t len = sizeof(lastTcpInfo);
    dp2p_assert(0 == getsockopt(fdSocket, SOL_TCP, TCP_INFO, &lastTcpInfo, &len) && len == sizeof(lastTcpInfo));
    dp2p_assert_v(lastTcpInfo.tcpi_state == TCP_CLOSE_WAIT || lastTcpInfo.tcpi_state == TCP_ESTABLISHED || lastTcpInfo.tcpi_state == TCP_CLOSE || lastTcpInfo.tcpi_state == TCP_LAST_ACK,
    		"TCP is in state %s.", TcpConnectionManager::tcpState2String(lastTcpInfo.tcpi_state).c_str());
}

void TcpConnection::assertSocketHealth() const
{
    unsigned err = -1;
    socklen_t errSize = sizeof(err);
    dp2p_assert(0 == getsockopt(fdSocket, SOL_SOCKET, SO_ERROR, &err, &errSize));
    dp2p_assert(err == 0);
}

pair<int,int> TcpConnection::getSocketBufferLengths() const
{
    dp2p_assert(fdSocket > 0);
    int socketRcvBufferSize = -1;
    socklen_t socketRcvBufferSize_len = sizeof(socketRcvBufferSize);
    int ret = ::getsockopt(fdSocket, SOL_SOCKET, SO_RCVBUF, &socketRcvBufferSize, &socketRcvBufferSize_len);
    dp2p_assert_v(ret == 0, "ret == %d, while 0 was expected.", ret);
    int socketSndBufferSize = -1;
    socklen_t socketSndBufferSize_len = sizeof(socketSndBufferSize);
    ret = ::getsockopt(fdSocket, SOL_SOCKET, SO_SNDBUF, &socketSndBufferSize, &socketSndBufferSize_len);
    dp2p_assert_v(ret == 0, "ret == %d, while 0 was expected.", ret);
    return pair<int,int>(socketSndBufferSize,socketRcvBufferSize);
}

int TcpConnection::state() const
{
	return lastTcpInfo.tcpi_state;
}

int TcpConnectionManager::create(const SourceId& srcId, const int& port, const IfData& ifData, const int& maxPendingRequests, const int64_t& connectTimeout)
{
	TcpConnection* d = new TcpConnection(srcId, port, ifData, maxPendingRequests, connectTimeout);
	if(connVec.capacity() == 0)
	    connVec.reserve(1024);
	else if(connVec.capacity() == connVec.size())
	    connVec.reserve(2 * connVec.size());
	connVec.push_back(d);
	return connVec.size() - 1;
}

void TcpConnectionManager::cleanup()
{
	for(std::size_t i = 0; i < connVec.size(); ++i)
		delete connVec.at(i);
	connVec.clear();
}

void TcpConnectionManager::disconnect(const TcpConnectionId& tcpConnectionId)
{
    delete connVec.at(tcpConnectionId.numeric());
    connVec.at(tcpConnectionId.numeric()) = nullptr;
    Statistics::unregisterTcpConnection(tcpConnectionId);
}

void TcpConnectionManager::logTCPState(const TcpConnectionId& tcpConnectionId, const char* reason)
{
    connVec.at(tcpConnectionId.numeric())->updateTcpInfo();
    Statistics::recordTcpState(tcpConnectionId, reason, connVec.at(tcpConnectionId.numeric())->lastTcpInfo);
}

string TcpConnectionManager::tcpState2String(int tcpState)
{
    switch(tcpState)
    {
    case TCP_ESTABLISHED: return "TCP_ESTABLISHED";
    case TCP_SYN_SENT:    return "TCP_SYN_SENT";
    case TCP_SYN_RECV:    return "TCP_SYN_RECV";
    case TCP_FIN_WAIT1:   return "TCP_FIN_WAIT1";
    case TCP_FIN_WAIT2:   return "TCP_FIN_WAIT2";
    case TCP_TIME_WAIT:   return "TCP_TIME_WAIT";
    case TCP_CLOSE:       return "TCP_CLOSE";
    case TCP_CLOSE_WAIT:  return "TCP_CLOSE_WAIT";
    case TCP_LAST_ACK:    return "TCP_LAST_ACK";
    case TCP_LISTEN:      return "TCP_LISTEN";
    case TCP_CLOSING:     return "TCP_CLOSING";
    default:
        dp2p_assert(0);
        return string();
    }
}

string TcpConnectionManager::tcpCAState2String(int tcpCAState)
{
    switch(tcpCAState)
    {
    case TCP_CA_Open:     return "TCP_CA_Open";
    case TCP_CA_Disorder: return "TCP_CA_Disorder";
    case TCP_CA_CWR:      return "TCP_CA_CWR";
    case TCP_CA_Recovery: return "TCP_CA_Recovery";
    case TCP_CA_Loss:     return "TCP_CA_Loss";
    default:
        dp2p_assert(0);
        return string();
    }
}

void TcpConnection::disconnect()
{
    if(fdSocket > 0) {
        if(shutdown(fdSocket, SHUT_RDWR)) {
            char _errBuf[1024];
            char *errBuf = strerror_r(errno, _errBuf, sizeof(_errBuf)); // get the error string
            WARNMSG("Could not propertly shutdown() socket to %s: %s.", SourceManager::get(srcId).hostName.c_str(), errBuf);
        }
        if(close(fdSocket)) {
            char _errBuf[1024];
            char *errBuf = strerror_r(errno, _errBuf, sizeof(_errBuf)); // get the error string
            WARNMSG("Could not propertly close() socket to %s: %s.", SourceManager::get(srcId).hostName.c_str(), errBuf);
        }
        fdSocket = -2;
    }

    delete [] recvBuf;
    recvBuf = nullptr;
}

} /* namespace dashp2p */

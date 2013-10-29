/****************************************************************************
 * DashHttp.cpp                                                             *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
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


//#include "Statistics.h"
#include "DashHttp.h"
#include "HttpEventConnected.h"
#include "HttpEventDataReceived.h"
#include "HttpEventDisconnect.h"
#include "Statistics.h"
//#include "HttpEventKeepAliveMaxReached.h"

#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits>
#include <time.h>
#include <sys/eventfd.h>
#include <inttypes.h>
#include <sys/ioctl.h>


// TODO: when reconnecting, try to reuse port number
// TODO: assert that there are no transmission holes by checking if all received TCP segments have maximum segment size
// TODO: do HEAD requests to better predict when a segment download will finish
// TODO: properly handle keep-alive timeout of the HTTP server. do not exceed this value or reconnect if exceeded
// TODO: properly handle the max number of requests per connection of the HTTP server. open a second connection in order to maintain pipelining across the max requests boundary???
// TODO: handle disconnection events during sending or receiving


DashHttp::DashHttp(ConnectionId id, const std::string& hostName, int port, const IfData& ifData, int clientSideRequestLimit, HttpCb cb)
  : state(DashHttpState_Undefined),
    id(id),
    ifTerminating(false),
    fdWakeUpSelect(-1),
    reqQueue(),
    newReqs(),
    newReqsMutex(),
    fdNewReqs(-1),
    mainThread(),
    fdSocket(-1),
    lastTcpInfo(NULL),
    ifData(ifData),
    clientSideRequestLimit(clientSideRequestLimit),
    numConnectEvents(0),
    numReqsCompletedOverCurrentTcpConnection(0),
    serverInfo(hostName),
    recvBufSize(10 * 1048576),
    recvBufContent(0),
    recvBuf(NULL),
    recvTimestamp(-1),
    cb(cb)
{
    /* Initialize synchronization variables. */
    ThreadAdapter::mutexInit(&newReqsMutex);

    /* Store hostName and resolve the address */
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    char tmpChar[64];
    sprintf(tmpChar, "%d", port);
    dp2p_assert(0 == getaddrinfo(hostName.c_str(), tmpChar, &hints, &result) && result);
    memcpy(&serverInfo.hostAddr, result->ai_addr, sizeof(serverInfo.hostAddr));
    freeaddrinfo(result);

    /* Connect and bind */
    this->connect();

    /* Allocate buffer for reading from the socket */
    recvBuf = new char[recvBufSize];
    dp2p_assert(recvBuf);

    /* Open the socket for transmitting the wake up signal to select */
    fdWakeUpSelect = eventfd(0, 0);
    dp2p_assert(fdWakeUpSelect != -1);

    /* Open the socket for notifications upon new requests */
    fdNewReqs = eventfd(0, 0);
    dp2p_assert(fdNewReqs != -1);

    /* Start the thread. */
    dp2p_assert(0 == ThreadAdapter::startThread(&mainThread, DashHttp::startThread, this));

    state = DashHttpState_Constructed;
}

DashHttp::~DashHttp()
{
    DBGMSG("Terminating DashHttp.");

    {
        ifTerminating = true;
        uint64_t dummy = 1;
        dp2p_assert(sizeof(dummy) == ::write(fdWakeUpSelect, &dummy, sizeof(dummy)));
    }

    /* Wait for the mainThread to terminate */
    ThreadAdapter::joinThread(mainThread);

    DBGMSG("Thread terminated.");

    //dp2p_assert_v(state == DashHttpState_NotAcceptingRequests, "state: %d", state);

    /* Request queue and related. */
    dp2p_assert_v(reqQueue.empty(), "Still have %d queued requests.", reqQueue.size());
    //while(!reqQueue.empty()) {
    //    delete reqQueue.front();
    //    reqQueue.pop_front();
    //}
    ThreadAdapter::mutexLock(&newReqsMutex);
    dp2p_assert_v(newReqs.empty(), "Still have %d new requests.", newReqs.size());
    //while(!newReqs.empty()) {
    //    delete newReqs.front();
    //    newReqs.pop_front();
    //}
    ThreadAdapter::mutexUnlock(&newReqsMutex);
    ThreadAdapter::mutexDestroy(&newReqsMutex);

    DBGMSG("Mutex destroyed.");

    /* Close the wakeUpSelect socket */
    if(fdWakeUpSelect != -1)
        dp2p_assert(0 == close(fdWakeUpSelect));

    /* Close the event socket for new requests */
    if(fdNewReqs != -1)
        dp2p_assert(0 == close(fdNewReqs));

    /* Close socket. */
    if(fdSocket != -1)
        dp2p_assert(0 == close(fdSocket));
    delete lastTcpInfo;

    /* Delete the buffer */
    dp2p_assert(recvBuf);
    delete [] recvBuf;
    recvBuf = NULL;

    DBGMSG("DashHttp terminated.");
}

void DashHttp::threadMain()
{
	HttpEventConnected* e = new HttpEventConnected(id);
	DBGMSG("Before cb().");
	cb(e);
	DBGMSG("cb() returned.");

    while(!ifTerminating)
    {
    	struct timeval selectTimeout = calculateSelectTimeout();

    	waitSelect(selectTimeout);

        if(ifTerminating) {
        	//reportDisconnect();
        	return;
        }

        updateTcpInfo();
        const bool socketDisconnected = (lastTcpInfo->tcpi_state != TCP_ESTABLISHED);
        const int socketHasData = checkIfSocketHasData();

        if(socketHasData > 0) {
        	readFromSocket(socketHasData);
        	processNewData();
        }

        if(socketDisconnected || (serverInfo.keepAliveMaxRemaining == 0 && getNumPendingRequests() == 0)) {
        	reportDisconnect();
        	return;
        	//WARNMSG("[%s] Socket disconnected. Forced re-connect.", Utilities::getTimeString(0, false).c_str());
        	//forceReconnect();
        	//Statistics::recordReconnect(Utilities::getTime(), ReconnectReason_Other);
        }
#if 0
        else if (serverInfo.keepAliveMaxRemaining == 0 && getNumPendingRequests() == 0) {
        	reportKeepAliveMaxReached();
        	return;
        	//INFOMSG("[%s] Server would close the connection due to KeepAlive max. Preventively reconnecting.", Utilities::getTimeString(0, false).c_str());
        	//forceReconnect();
        	//Statistics::recordReconnect(Utilities::getTime(), ReconnectReason_HttpKeepAliveMaxPreventive);
        }
#endif

        /* Can we send more requests? */
        checkStartNewRequests();

        if(ifTerminating) {
        	//reportDisconnect();
            return;
        }
    }

    if(!ifTerminating)
    	reportDisconnect();

#if 0
        const Usec expectedHttpTimeout = calculateExpectedHttpTimeout();
        const Usec absNow = Utilities::getAbsTime();
        if(absNow > expectedHttpTimeout && getNumPendingRequests() == 0)
        {
        	INFOMSG("Server would close the connection at %s due to its HTTP timeout. Preventively reconnecting.",
        			Utilities::getTimeString(expectedHttpTimeout, false).c_str());
        	forceReconnect();
        	Statistics::recordReconnect(Utilities::getTime(), ReconnectReason_HttpKeepAliveTimeoutPreventive);
        }
        else
        {
        	DBGMSG("Server would close the connection in approx. %.3f sec tue to KeepAlive timeout. %d requests are pending. No need to reconnect yet.",
        			(expectedHttpTimeout - absNow) / 1e6, getNumPendingRequests());
        }
    }
#endif
}

bool DashHttp::newRequest(std::list<HttpRequest*> reqs)
{
    dp2p_assert(!reqs.empty());

    /* Lock mutex. */
    ThreadAdapter::mutexLock(&newReqsMutex);

    if(ifTerminating || state != DashHttpState_Constructed) {
    	DBGMSG("Rejecting %d new requests.", reqs.size());
    	ThreadAdapter::mutexUnlock(&newReqsMutex);
    	return false;
    }

    DBGMSG("Got %d new requests. Queue state: %s.", reqs.size(), reqQueue2String().c_str());

    /* Add new requests to the queue */
    for(std::list<HttpRequest*>::iterator it = reqs.begin(); it != reqs.end(); ++it) {
        newReqs.push_back(*it);
        // TODO: make usage of ifData information in DashHttp and in HttpRequest and in Control and in ControlLogic and Statistics consistent
        (*it)->devName = ifData.name;
    }

    /* If the downloader thread is waiting, signal the new request. */
    uint64_t numNewReqs = reqs.size();
    dp2p_assert(sizeof(numNewReqs) == ::write(fdNewReqs, &numNewReqs, sizeof(numNewReqs)));

    /* Unlock the mutex. */
    ThreadAdapter::mutexUnlock(&newReqsMutex);

    return true;
}

int DashHttp::hasRequests()
{
	int ret = 0;
	ThreadAdapter::mutexLock(&newReqsMutex);
	ret += newReqs.size();
	ret += reqQueue.size();
	ThreadAdapter::mutexUnlock(&newReqsMutex);
	return ret;
}

void* DashHttp::startThread(void* params)
{
    DashHttp* dashHttp = (DashHttp*)params;
    dashHttp->threadMain();
    return NULL;
}

struct timeval DashHttp::calculateSelectTimeout()
{
	struct timeval ret;

	/* Update info about the TCP connection */
	updateTcpInfo();

	/* Some Debug output */
	DBGMSG("New iteration.");
	DBGMSG("Queue state: %s", reqQueue2String().c_str());
	DBGMSG("Server state: %s", serverState2String().c_str());
	DBGMSG("Connection state: %s", connectionState2String().c_str());

	/* If disconnected, timeout will be 0 */
	if(lastTcpInfo->tcpi_state != TCP_ESTABLISHED) {
		ret.tv_sec = 0;
		ret.tv_usec = 0;
		return ret;
	}

	/* Calculate expected HTTP timeout */
	const Usec expectedHttpTimeout = calculateExpectedHttpTimeout();
	Usec nowAbs = Utilities::getAbsTime();

	/* Calculate select timeout */
	const Usec selectTimeout = max<Usec>(0, min<Usec>(1000000, expectedHttpTimeout - nowAbs));

	ret.tv_sec = selectTimeout / 1000000;
	ret.tv_usec = selectTimeout % 1000000;
	return ret;
}

pair<bool,bool> DashHttp::waitSelect(struct timeval selectTimeout)
{
	fd_set fdSetRead;
	FD_ZERO(&fdSetRead);
	int nfds = 0;
	FD_SET(fdSocket, &fdSetRead);
	nfds = max<int>(nfds, fdSocket);
	FD_SET(fdNewReqs, &fdSetRead);
	nfds = max<int>(nfds, fdNewReqs);
	FD_SET(fdWakeUpSelect, &fdSetRead);
	nfds = max<int>(nfds, fdWakeUpSelect);
	DBGMSG("Going to select() for up to %gs.", Utilities::getTimeString(0, false).c_str(), selectTimeout.tv_sec + selectTimeout.tv_usec / 1e6);
	const Usec ticBeforeSelect = Utilities::getAbsTime();
	const int retSelect = select(nfds + 1, &fdSetRead, NULL, NULL, &selectTimeout);
	const Usec tocAfterSelect = Utilities::getAbsTime();
	if(retSelect == -1) {
		printf("errno = %d\n", errno);
		abort();
	}

	DBGMSG("Was in select() for %gs, returning %d.", (tocAfterSelect - ticBeforeSelect) / 1e6, retSelect);

	return pair<bool,bool>(FD_ISSET(fdSocket, &fdSetRead), FD_ISSET(fdNewReqs, &fdSetRead));
}

int DashHttp::checkIfSocketHasData()
{
	int bytesAvailable = -1;
	dp2p_assert(-1 != ioctl(fdSocket, FIONREAD, &bytesAvailable));
	return bytesAvailable;
}

#if 0
bool DashHttp::checkIfHaveNewRequests()
{
	uint64_t bytesAvailable = 0;
	dp2p_assert(-1 != ioctl(fdNewReqs, FIONREAD, &bytesAvailable));
	dp2p_assert_v(bytesAvailable == 0 || bytesAvailable == 8, "bytesAvailable: %d", bytesAvailable);
	return (bytesAvailable == 8);
}
#endif

void DashHttp::reportDisconnect()
{
	ThreadAdapter::mutexLock(&newReqsMutex);
	dp2p_assert_v(state == DashHttpState_Constructed, "state: %d", state);
	state = DashHttpState_NotAcceptingRequests;
	if(!newReqs.empty()) {
		uint64_t numNewReqs = 0;
		dp2p_assert(sizeof(numNewReqs) == ::read(fdNewReqs, &numNewReqs, sizeof(numNewReqs)));
		dp2p_assert(numNewReqs == newReqs.size());
		reqQueue.splice(reqQueue.end(), newReqs);
	}
	ThreadAdapter::mutexUnlock(&newReqsMutex);

	WARNMSG("Socket disconnected. Queue state: %s.", reqQueue2String().c_str());
	HttpEventDisconnect* e = new HttpEventDisconnect(id, reqQueue);
	reqQueue.clear();
	DBGMSG("Before cb().");
	cb(e);
	DBGMSG("cb() returned.");
}

void DashHttp::connect()
{
    dp2p_assert(fdSocket == -1);

    /* Open the socket. */
    // TODO: increase outoing buffer size to something around 1 MB or more
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    dp2p_assert(fdSocket != -1);

    /* TODO: If want to increase socket buffer sizes, must do it here, before connect! */

    /* Bind the socket */
    if(ifData.initialized) {
        dp2p_assert(0 == bind(fdSocket, (struct sockaddr*)&ifData.sockaddr_in, sizeof(ifData.sockaddr_in)));
        DBGMSG("Binded socket to %s:%d.", inet_ntoa(ifData.sockaddr_in.sin_addr), ifData.sockaddr_in.sin_port);
    }

    /* Connect. */
    int retVal = -1;
    for(int i = 0; i < 5 && !ifTerminating && retVal != 0; ++i)
    {
    	Usec ticBeforeConnect = Utilities::getAbsTime();
    	retVal = ::connect(fdSocket, (struct sockaddr*)&serverInfo.hostAddr, sizeof(serverInfo.hostAddr));
    	int connectErrno = errno;
    	Usec tocAfterConnect = Utilities::getAbsTime();
    	DBGMSG("Spent %gs in connect().", (tocAfterConnect - ticBeforeConnect) / 1e6);
    	if(retVal != 0) {
    		ERRMSG("connect() returned %d, errno: %d", retVal, connectErrno);
    	}
    }
    if(retVal != 0) {
    	ERRMSG("Could not connect to %s.", serverInfo.hostName.c_str());
    	abort();
    }

    ++ numConnectEvents;
    numReqsCompletedOverCurrentTcpConnection = 0;
    if(serverInfo.keepAliveTimeout != -1)
        serverInfo.keepAliveTimeoutNext = Utilities::getAbsTime() + serverInfo.keepAliveTimeout;

    updateTcpInfo();
    dp2p_assert(lastTcpInfo->tcpi_state == TCP_ESTABLISHED);
    serverInfo.keepAliveMaxRemaining = serverInfo.keepAliveMax;

    const pair<int,int> socketBufferLengths = this->getSocketBufferLengths();
    DBGMSG("Socket snd buf size: %d, socket rcv buf size: %d.", socketBufferLengths.first,socketBufferLengths.second);
}

#if 0
void DashHttp::forceReconnect()
{
    //DBGMSG("Forced reconnect.");
    dp2p_assert(fdSocket != -1);
    updateTcpInfo();
    switch(lastTcpInfo->tcpi_state) {
    case TCP_CLOSE:
    	DBGMSG("TCP is in TCP_CLOSE");
    	break;
    case TCP_CLOSE_WAIT:
    case TCP_ESTABLISHED:
    {
    	DBGMSG("TCP is in %s before shutdown.", Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
    	dp2p_assert(0 == shutdown(fdSocket, SHUT_RDWR));
    	updateTcpInfo();
    	DBGMSG("TCP is in %s after shutdown.", Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
    	struct timeval timeval;
    	timeval.tv_sec = 0;
    	timeval.tv_usec = 500000; //TODO: implement opening a new TCP connection on a different socket on the controllogic level. should be faster.
    	pair<bool,bool> retSelect = waitSelect(timeval);
    	dp2p_assert(retSelect.first);
    	dp2p_assert(checkIfSocketHasData() == 0);
    	updateTcpInfo();
    	DBGMSG("TCP is in %s after select().", Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
    }
    break;
    default: abort(); break;
    }
    dp2p_assert(0 == close(fdSocket));
    fdSocket = -1;
    this->connect();
}
#endif

pair<int,int> DashHttp::getSocketBufferLengths() const
{
    dp2p_assert(fdSocket != -1);
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

void DashHttp::assertSocketHealth() const
{
    unsigned err = -1;
    socklen_t errSize = sizeof(err);
    dp2p_assert(0 == getsockopt(fdSocket, SOL_SOCKET, SO_ERROR, &err, &errSize));
    dp2p_assert(err == 0);
}

string DashHttp::getUnexpectedDisconnectMessage() const
{
    char buf[2048];
    if(!reqQueue.empty())
    {
        const HttpRequest* req = reqQueue.front();
        if(req->pldBytesReceived > 0) {
            sprintf(buf, "Server closed the connection in the middle of request %d. %"PRId64" bytes of payload (out of %"PRId64") were received.", req->reqId, req->pldBytesReceived, req->hdr.contentLength);
        } else if(req->hdrCompleted) {
            sprintf(buf, "Server closed the connection after receiving header of request %d. No payload was received.", req->reqId);
        } else if(req->hdrBytesReceived > 0){
            sprintf(buf, "Server closed the connection in the middle of request %d. %u bytes of header were received (header incomplete).", req->reqId, req->hdrBytesReceived);
        } else if(!req->sent()){
            sprintf(buf, "Server closed the connection while waiting for request %d. No header byte were received.", req->reqId);
        } else {
            sprintf(buf, "Server unexpectedily closed the connection. No requests are pending. %u requests are queued. %u requests are pre-queued.",
                    (unsigned)reqQueue.size(), (unsigned)newReqs.size());
        }
    }
    else
    {
        sprintf(buf, "Server unexpectedily closed the connection. No requests are pending. No requests are queued. %u requests are pre-queued.", (unsigned)newReqs.size());
    }
    return string(buf);
}

bool DashHttp::havePendingRequests() const
{
    for(list<HttpRequest*>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it) {
        if((*it)->sent()) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

int DashHttp::getNumPendingRequests() const
{
    int ret = 0;
    for(list<HttpRequest*>::const_iterator it = reqQueue.begin(); it != reqQueue.end() && (*it)->sent(); ++it)
        ++ret;
    return ret;
}

const HttpRequest& DashHttp::getQueuedRequest(ReqId reqId) const
{
    for(list<HttpRequest*>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it) {
        if((*it)->reqId == reqId)
            return **it;
    }
    dp2p_assert(false);
    return *reqQueue.front();
}

string DashHttp::reqQueue2String() const
{
	int numPending = 0;
	int first = -1;
	int last = -1;
	for(list<HttpRequest*>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it) {
		if((*it)->sent()) {
			if(first == -1)
				first = (*it)->reqId;
			last = (*it)->reqId;
			++numPending;
		} else {
			break;
		}
	}

	char tmp[2048];
	string ret;
	if(!reqQueue.empty())
		sprintf(tmp, "Queue length: %u (first: %d, last: %d). Pending: %d (first: %d, last: %d).",
				(unsigned)reqQueue.size(), reqQueue.front()->reqId, reqQueue.back()->reqId, numPending, first, last);
	else
		sprintf(tmp, "Queue empty.");
	ret.append(tmp);
	return ret;
}

void DashHttp::readFromSocket(int bytesExpected)
{
	dp2p_assert_v(recvBufContent == 0 && bytesExpected > 0, "recvBufContent: %"PRId32", bytesExpected: %"PRId32, recvBufContent, bytesExpected);

	/* Log TCP state. Here, even if the TCP state is not TCP_ESTABLISHED, we call recv().
	 * We do it since we know that there are data available due to the return value of select(). */
	logTCPState("before recv()");

	/* Read from the socket */
	const pair<int,int> socketBufferLengths = this->getSocketBufferLengths(); // TODO: remove after debugging
	dp2p_assert_v(socketBufferLengths.second / 2 <= (int)recvBufSize, "Socket rcv buf size: %d, DashHttp recv buf size: %d.", socketBufferLengths.second, recvBufSize);
	recvBufContent = recv(fdSocket, recvBuf, recvBufSize, 0);
	if(recvBufContent == -1) {
		perror("select()");
		abort();
	}
	dp2p_assert_v(bytesExpected <= recvBufContent && recvBufContent < (int)recvBufSize,
			"recvBufContent: %d, bytesExpected: %d, recvBufSize: %u", recvBufContent, bytesExpected, recvBufSize);

	recvBuf[recvBufContent] = 0;
	recvTimestamp = Utilities::now();

	/* Check the health of the socket and log TCP state. */
	assertSocketHealth();
	logTCPState("after recv()");

	DBGMSG("Received %d bytes. Expected: %d bytes.", recvBufContent, bytesExpected);
}

void DashHttp::processNewData()
{
    /* Process buffer content */
    int recvBufProcessed = 0;
    while(recvBufProcessed < recvBufContent)
    {
        //DBGMSG("%u bytes in buffer, %u bytes processed, %u bytes remaining.", recvBufContent, recvBufProcessed, recvBufContent - recvBufProcessed);

        /* Get the latest request from the request queue. */
        dp2p_assert(!reqQueue.empty());
        HttpRequest* req = reqQueue.front();

        //const int hdrBytesBefore = req->hdrBytesReceived;
        const int pldBytesBefore = req->pldBytesReceived;
        const int parsed = parseData(recvBuf + recvBufProcessed, recvBufContent - recvBufProcessed, req, recvTimestamp);

        /* If header completed */
        if(req->hdrCompleted)
        {
        	switch(req->hdr.statusCode) {
        	case HTTP_STATUS_CODE_OK:
        	{
        		/* If this was the first header from this server, initialize server info */
        		if(!serverInfo.aHdrReceived)
        		{
        			dp2p_assert(req->hdr.keepAliveMax > 0 && req->hdr.keepAliveTimeout > 0);
        			serverInfo.aHdrReceived = true;
        			serverInfo.keepAliveMax = req->hdr.keepAliveMax + 1;
        			serverInfo.keepAliveMaxRemaining = max<int>(0, serverInfo.keepAliveMax - getNumPendingRequests());
        			serverInfo.keepAliveTimeout = req->hdr.keepAliveTimeout;
        			serverInfo.keepAliveTimeoutNext = Utilities::getAbsTime() + serverInfo.keepAliveTimeout;
        			INFOMSG("Server accepts up to %d requests over a TCP connection. Timeout: %gs.", serverInfo.keepAliveMax, serverInfo.keepAliveTimeout / 1e6);

        			requeuePendingRequests(serverInfo.keepAliveMaxRemaining + 1);
        		}
        		else
        		{
        			/* Some server might suddently set Connection: close although Keep-Alive max was not exceeded. Issue a warning and react appropriately. */
        		    // fixme The next lines are commented out due to Windows IIS behavior (not sending keep-alive max and timeout
        		    //const int numPendingRequests = getNumPendingRequests(); // this includes the one currently being processed
        			//if(serverInfo.keepAliveMaxRemaining + numPendingRequests - 1 == 0) {
        			//	dp2p_assert(req->hdr.connectionClose == 1 && req->hdr.keepAliveMax == -1);
        			//	serverInfo.keepAliveMaxRemaining = 0;
        			//	requeuePendingRequests(serverInfo.keepAliveMaxRemaining + 1);
        			//} else if(serverInfo.keepAliveMaxRemaining + numPendingRequests - 1 != req->hdr.keepAliveMax) {
        			//	dp2p_assert_v(req->hdr.connectionClose == 1, "Very strange behavior of the HTTP server. Header: %s.", req->hdr.toString().c_str());
        			//	DBGMSG("Server is inconsistent wrt KeepAlive max. Sent 0 while expected %d. (Have %d pending requests.)",
        			//			serverInfo.keepAliveMaxRemaining + numPendingRequests - 1, numPendingRequests - 1);
        			//	serverInfo.keepAliveMaxRemaining = 0;
        			//	requeuePendingRequests(serverInfo.keepAliveMaxRemaining + 1);
        			//} else {
        			//	DBGMSG("Remaining requests: %d", serverInfo.keepAliveMaxRemaining);
        			//}
        		}
        	}
        	break;
        	case HTTP_STATUS_CODE_FOUND:
        	{
        		// TODO: Currently, we do not support 302 status code with pipelining.
        		dp2p_assert(getNumPendingRequests() == 1);
        	}
        	break;
        	default:
        		abort();
        		break;
        	}
        }

        recvBufProcessed += parsed;

        /* Record download progress  */
        req->recordDownloadProgress(DownloadProcessElement(Utilities::getAbsTime(), parsed));

        /* Act if current request completed */
        if(req->completed())
        {
            if(req->hdr.statusCode != HTTP_STATUS_CODE_FOUND && (serverInfo.keepAliveMaxRemaining == 0 || req->hdr.connectionClose == 1)) {
            	dp2p_assert_v(serverInfo.keepAliveMaxRemaining == 0 && req->hdr.connectionClose == 1, "Header: %s.", req->hdr.toString().c_str());
                dp2p_assert_v(recvBufProcessed == recvBufContent, "processed: %d, content: %d", recvBufProcessed, recvBufContent);
            }

            ++ numReqsCompletedOverCurrentTcpConnection;

            DBGMSG("Request %d completed. Total over current TCP connection: %d.", req->reqId, numReqsCompletedOverCurrentTcpConnection);

            reqQueue.pop_front();
        }

        /* If at least the header is completed, give data to the user. */
        if(req->hdrCompleted && req->pldBytesReceived > 0) {
        	HttpEventDataReceived* eventDataReceived = new HttpEventDataReceived(id, req, req->completed(), pldBytesBefore, req->pldBytesReceived - 1);
        	DBGMSG("Before cb().");
        	cb(eventDataReceived);
        	DBGMSG("cb() returned.");
        	//cb(req->reqId, ifData.name, recvBuf + recvBufProcessed + (req->hdrBytesReceived - hdrBytesBefore), pldBytesBefore, req->pldBytesReceived - 1, req->hdr.contentLength);
        } else if(req->hdrCompleted) {
        	HttpEventDataReceived* eventDataReceived = new HttpEventDataReceived(id, req, req->completed(), 0, 0);
        	DBGMSG("Before cb().");
        	cb(eventDataReceived);
        	DBGMSG("cb() returned.");
        	//cb(req->reqId, ifData.name, NULL, 0, 0, req->hdr.contentLength);
        }
    }
    recvBufContent = 0;

    if(serverInfo.keepAliveTimeout != -1)
    	serverInfo.keepAliveTimeoutNext = Utilities::getAbsTime() + serverInfo.keepAliveTimeout;
}

void DashHttp::requeuePendingRequests(int numAllowed)
{
    int cnt = 0;
    int cntToRepeat = 0;
    for(list<HttpRequest*>::iterator it = reqQueue.begin(); it != reqQueue.end() && (*it)->sent(); ++it)
    {
    	++cnt;
    	if(cnt > numAllowed) {
    		(*it)->markUnsent();
    		++cntToRepeat;
    	}
    }
    DBGMSG("Have to re-send %d requests.", cntToRepeat);
}

bool DashHttp::checkStartNewRequests()
{
    ThreadAdapter::mutexLock(&newReqsMutex);
    if(!newReqs.empty()) {
        uint64_t numNewReqs = 0;
        dp2p_assert(sizeof(numNewReqs) == ::read(fdNewReqs, &numNewReqs, sizeof(numNewReqs)));
        dp2p_assert(numNewReqs == newReqs.size());
        reqQueue.splice(reqQueue.end(), newReqs);
    }
    ThreadAdapter::mutexUnlock(&newReqsMutex);

    const int numPending = this->getNumPendingRequests();
    const int numQueuedNotSent = reqQueue.size() - numPending;

    /* If no more queued requests, return. */
    if(numQueuedNotSent == 0) {
        return false;
    }

    /* If client-side restriction on max number sent requests reached, return. */
    dp2p_assert_v(clientSideRequestLimit == 0 || numPending <= clientSideRequestLimit, "packets on the fly: %d, client-side limit: %d.", numPending, clientSideRequestLimit);
    if(clientSideRequestLimit > 0 && numPending >= clientSideRequestLimit) {
        return false;
    }

    /* If server-side restriction on max number sent requests reached, return. */
    if(serverInfo.keepAliveMaxRemaining == 0) {
        return false;
    }

    /* Get the first unsent request */
    list<HttpRequest*>::iterator it = reqQueue.begin();
    for(; it != reqQueue.end() && (*it)->sent(); ++it)
        continue;

    /* If there are issued requests and the next pending does not allow pipelining, return */
    if(numPending > 0 && (*it)->allowPipelining == false) {
        return false;
    }

    /* Now we know that we can issue at least one new request */

    /* Start new requests */
    list<ReqId> reqsToSend;
    if(serverInfo.keepAliveMaxRemaining != -1)
        DBGMSG("Server accepts up to %d more requests on the existing TCP connection.", serverInfo.keepAliveMaxRemaining);
    else
        DBGMSG("We do not know yet servers keep-alive request limit.");
    for( ; it != reqQueue.end(); ++it)
    {
        HttpRequest* req = *it;
        if(req->allowPipelining == false && (numPending > 0 || reqsToSend.size() > 0))
            break;
        if(serverInfo.keepAliveMaxRemaining == 0)
            break;
        reqsToSend.push_back(req->reqId);
        if(serverInfo.keepAliveMax != -1) // design choice: send unlimited num of requests if don't know servers limit
            --serverInfo.keepAliveMaxRemaining;
        req->tsSent = Utilities::now(); // TODO: might want to improve precision of this time stamp by moving it into sendHttpRequest() (but is not trivial)
        req->sentPipelined = (it == reqQueue.begin()) ? false : true;
        if(clientSideRequestLimit > 0 && numPending + (int)reqsToSend.size() == clientSideRequestLimit)
            break;
    }
    DBGMSG("Will send %u more requests. %d remain in the queue.", reqsToSend.size(), reqQueue.size() - reqsToSend.size());
    dp2p_assert(!reqsToSend.empty());

    /* Send the HTTP GET request. */
    sendHttpRequest(reqsToSend);

    return true;
}

void DashHttp::sendHttpRequest(const list<ReqId>& reqsToSend)
{
    /* Formulate GET requests */
    std::string reqBuf;
    for(list<ReqId>::const_iterator it = reqsToSend.begin(); it != reqsToSend.end(); ++it)
    {
        const HttpRequest& req = getQueuedRequest(*it);
        std::string methodString;
        switch (req.httpMethod) {
        case HttpMethod_GET:  methodString = "GET";  break;
        case HttpMethod_HEAD: methodString = "HEAD"; break;
        default: dp2p_assert(0); break;
        }

        DBGMSG("%s http://%s/%s", methodString.c_str(), serverInfo.hostName.c_str(), req.file.c_str());

        reqBuf.append(methodString); reqBuf.append(" /"); reqBuf.append(req.file); reqBuf.append(" HTTP/1.1\r\n");
        reqBuf.append("User-Agent: CUSTOM\r\n");
        reqBuf.append("Host: "); reqBuf.append(serverInfo.hostName); reqBuf.append("\r\n");
        reqBuf.append("Connection: Keep-Alive\r\n");
        reqBuf.append("\r\n");
    }

    /* Assert socket health and health of TCP connection. */
    dp2p_assert(fdSocket != -1);
    assertSocketHealth();
    updateTcpInfo();
    if(lastTcpInfo->tcpi_state != TCP_ESTABLISHED) {
    	ERRMSG("Wanted to send requests but TCP connection is %s.", Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
    	ERRMSG("Server state: %s", serverState2String().c_str());
    	ERRMSG("reqQueue: %s", reqQueue2String().c_str());
    	abort();
    }

    /* Send the request. */
    while(!reqBuf.empty())
    {
        int retVal = send(fdSocket, reqBuf.c_str(), reqBuf.size(), 0);//MSG_DONTWAIT);
        // TODO: react to connectivity interruptions here
        dp2p_assert(retVal > 0);
        reqBuf.erase(0, retVal);
    }

    if(serverInfo.keepAliveTimeout != -1)
    	serverInfo.keepAliveTimeoutNext = Utilities::getAbsTime() + serverInfo.keepAliveTimeout;

    /* Log TCP status. */
    dp2p_assert(fdSocket != -1);
    assertSocketHealth();
    updateTcpInfo();
    Statistics::recordTcpState(id, "after send()", *lastTcpInfo);
}

void DashHttp::updateTcpInfo()
{
    dp2p_assert(fdSocket != -1);
    if(lastTcpInfo == NULL)
        lastTcpInfo = new struct tcp_info;
    memset(lastTcpInfo, 0, sizeof(*lastTcpInfo));
    socklen_t len = sizeof(*lastTcpInfo);
    dp2p_assert(0 == getsockopt(fdSocket, SOL_TCP, TCP_INFO, lastTcpInfo, &len) && len == sizeof(*lastTcpInfo));
    dp2p_assert_v(lastTcpInfo->tcpi_state == TCP_CLOSE_WAIT || lastTcpInfo->tcpi_state == TCP_ESTABLISHED || lastTcpInfo->tcpi_state == TCP_CLOSE || lastTcpInfo->tcpi_state == TCP_LAST_ACK,
    		"TCP is in state %s.", Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
}

int DashHttp::parseData(const char* p, int size, HttpRequest* req, double recvTimestamp)
{
	dp2p_assert(p && size > 0 && req && !req->completed());

	int bytesParsed = 0;

	/* Receive header bytes and store it in the request object */
	if(!req->hdrCompleted)
	{
		/* Check if we now have the complete header */
		const char* endOfHeader = NULL;
		if(       req->hdrBytesReceived >= 1 && size >= 3 && 0 == strncmp(req->hdrBytes + (req->hdrBytesReceived - 1), "\r", 1)     && 0 == strncmp(p, "\n\r\n", 3)) {
			endOfHeader = p + 2;
		} else if(req->hdrBytesReceived >= 2 && size >= 2 && 0 == strncmp(req->hdrBytes + (req->hdrBytesReceived - 2), "\r\n", 2)   && 0 == strncmp(p, "\r\n", 2)) {
			endOfHeader = p + 1;
		} else if(req->hdrBytesReceived >= 3 && size >= 1 && 0 == strncmp(req->hdrBytes + (req->hdrBytesReceived - 3), "\r\n\r", 3) && 0 == strncmp(p, "\n", 1)) {
			endOfHeader = p + 0;
		} else if(size >= 4) {
			endOfHeader = strstr(p, "\r\n\r\n");
			if(endOfHeader)
				endOfHeader +=3 ;
		} else {
			// endOfHeader remains equal to NULL, reader not yet completely received
		}

		/* Number of new header bytes received */
		const unsigned newHdrBytes = endOfHeader ? endOfHeader - p + 1 : size;

		DBGMSG("Received %s %u bytes of header of request %u (%u in total). Header%s completed. %u bytes remain in the buffer.",
				req->hdrBytesReceived ? "further" : "first", newHdrBytes, req->reqId, req->hdrBytesReceived + newHdrBytes,
						endOfHeader ? "" : " NOT", size - newHdrBytes);

		/* Save the received header data. Might remove this later for optimization purposes. */
		const char* oldHdrData = req->hdrBytes;
		req->hdrBytes = new char[req->hdrBytesReceived + newHdrBytes + 1];
		memcpy(req->hdrBytes, oldHdrData, req->hdrBytesReceived);
		memcpy(req->hdrBytes + req->hdrBytesReceived, p, newHdrBytes);
		req->hdrBytes[req->hdrBytesReceived + newHdrBytes] = 0; // zero termination of the header string
		delete [] oldHdrData;

		/* Update header information */
		if(req->hdrBytesReceived == 0)
			req->tsFirstByte = recvTimestamp;
		req->hdrBytesReceived += newHdrBytes;
		req->hdrCompleted = (endOfHeader != NULL);

		// TODO: remove after debugging
		dp2p_assert_v(req->hdrBytesReceived <= 500, "[%.*s]", req->hdrBytesReceived, req->hdrBytes);

		bytesParsed = newHdrBytes;

		if(req->hdrCompleted)
			parseHeader(req);
	}

	if(bytesParsed == size || req->completed())
		return bytesParsed;
	else
		dp2p_assert(req->hdrCompleted);

	/* header is completed, we are parsing payload */
	dp2p_assert(req->hdr.contentLength > 0 && req->pldBytesReceived < req->hdr.contentLength);

	const unsigned newPldBytes = min<int64_t>(req->hdr.contentLength - req->pldBytesReceived, (int64_t)(size - bytesParsed));

	DBGMSG("Received bytes [%"PRId64", %"PRId64"] (out of %"PRId64") of request %u. Request%s completed. %u bytes left in the buffer.",
			req->pldBytesReceived, req->pldBytesReceived + newPldBytes - 1, req->hdr.contentLength, req->reqId,
			(req->pldBytesReceived + newPldBytes == req->hdr.contentLength) ? "" : " NOT", size - bytesParsed - newPldBytes);

	/* Update request information */
	if(req->pldBytesReceived + newPldBytes == req->hdr.contentLength)
		req->tsLastByte = recvTimestamp;
	if(!req->pldBytes)
		req->pldBytes = new char[req->hdr.contentLength];
	memcpy(req->pldBytes + req->pldBytesReceived, p + bytesParsed, newPldBytes);
	req->pldBytesReceived += newPldBytes;

	bytesParsed += newPldBytes;
	return bytesParsed;
}

void DashHttp::parseHeader(HttpRequest* req) const
{
    /* Parse each line of the header. */
    int cnt = 0;
    for(const char* pos = req->hdrBytes; pos[0] != '\r' || pos[1] != '\n'; pos = 2 + strstr(pos, "\r\n"))
    {
    	/* Debug: output next header line */
    	const int lineLength = strstr(pos, "\r\n") - pos;
    	DBGMSG("HTTP header line %02d: [%.*s]", cnt++, lineLength, pos);

    	/* Parse first line of the header (containing status code) */
        if(0 == strncmp(pos, "HTTP/1.1", 8))
        {
            /* Parse status code. */
            int32_t _httpStatusCode = 0;
            dp2p_assert(1 == sscanf(pos, "HTTP/1.1 %"SCNd32, &_httpStatusCode) && _httpStatusCode != 0);
            switch(_httpStatusCode) {
            case 200: req->hdr.statusCode = HTTP_STATUS_CODE_OK; break;
            case 302: req->hdr.statusCode = HTTP_STATUS_CODE_FOUND; break;
            default:
                ERRMSG("HTTP returned status code %"PRIu32" for %s/%s.", _httpStatusCode, serverInfo.hostName.c_str(), req->file.c_str());
                abort();
                break;
            }
        }

        /* Parse content length */
        else if(0 == strncmp(pos, "Content-Length:", 15))
        {
        	req->hdr.contentLength = -1;
            dp2p_assert(1 == sscanf(pos, "Content-Length: %"SCNd64, &req->hdr.contentLength) && req->hdr.contentLength != -1);
        }

        /* Parse "Connection:" line */
        else if(0 == strncmp(pos, "Connection:", 11))
        {
            if(0 == strncmp(pos, "Connection: close", 17)) {
            	req->hdr.connectionClose = 1;
            } else {
                dp2p_assert_v(0 == strncmp(pos, "Connection: Keep-Alive", 22), "Unknown HTTP header line: %.*s.", lineLength, pos);
                req->hdr.connectionClose = 0;
            }
        }

        /* Parse Keep-Alive: line */
        else if(0 == strncmp(pos, "Keep-Alive:", 11))
        {
            const char* posMax = strstr(pos, "max=");
            dp2p_assert(posMax && posMax - pos < lineLength);
            dp2p_assert_v(1 == sscanf(posMax, "max=%d", &req->hdr.keepAliveMax) && req->hdr.keepAliveMax > 0, "Error parsing HTTP header line: %.*s.", lineLength, pos);

            const char* posTimeout = strstr(pos, "timeout=");
            dp2p_assert(posTimeout && posTimeout - pos < lineLength);
            dp2p_assert_v(1 == sscanf(posTimeout, "timeout=%"SCNd64, &req->hdr.keepAliveTimeout) && req->hdr.keepAliveTimeout > 0, "Error parsing HTTP header line: %.*s.", lineLength, pos);
            req->hdr.keepAliveTimeout *= 1000000;
        }

        // TODO: parse the keep-alive parameter and limit the length of the pipeline. consider removing httpclosereceived after that.
        // TODO: even better: open a new TCP connection in advance before the other one gets closed!
    }

    /* sanity checks */
    switch(req->hdr.statusCode) {
    case HTTP_STATUS_CODE_OK:
    {
    	/* Sanity checks */
    	dp2p_assert(req->hdr.contentLength >= 0);
    	if(req->hdr.connectionClose == 1) {
    		dp2p_assert_v(req->hdr.keepAliveMax == -1 && req->hdr.keepAliveTimeout == -1, "max: %d, timeout: %"PRId64, req->hdr.keepAliveMax, req->hdr.keepAliveTimeout);
    	} else {
    	    // fixme Workaround for Windows IIS (doesn't have connection close parameter)
    	    if(req->hdr.connectionClose == -1)
    	        req->hdr.connectionClose = 0;
    	    if(req->hdr.keepAliveMax == -1)
    	        req->hdr.keepAliveMax = 10000;
    	    if(req->hdr.keepAliveTimeout == -1)
    	        req->hdr.keepAliveTimeout = 3600;
    	    // END
    		dp2p_assert(req->hdr.connectionClose == 0);
    		dp2p_assert_v(req->hdr.keepAliveMax > 0 && req->hdr.keepAliveTimeout > 0, "max: %d, timeout: %"PRId64, req->hdr.keepAliveMax, req->hdr.keepAliveTimeout);
    		if(serverInfo.keepAliveTimeout != -1) {
    			dp2p_assert_v(req->hdr.keepAliveTimeout == serverInfo.keepAliveTimeout, "Server changed keep-alive timeout from %"PRId64" to %"PRId64"??",
    					serverInfo.keepAliveTimeout, req->hdr.keepAliveTimeout);
    		}
    	}
    }
    break;
    case HTTP_STATUS_CODE_FOUND:
    {

    }
    break;
    default:
    	ERRMSG("Unexpected HTTP status code: %d.", req->hdr.statusCode);
    	abort();
    	break;
    }

    DBGMSG("Header parsed: %s", req->hdr.toString().c_str());
}

string DashHttp::serverState2String() const
{
	string ret;
	char tmp[2048];
	if(serverInfo.keepAliveMax == -1) {
		dp2p_assert(serverInfo.keepAliveMaxRemaining == -1 && serverInfo.keepAliveTimeout == -1 && serverInfo.keepAliveTimeoutNext == -1);
		sprintf(tmp, "Unknown.");
	} else {
		sprintf(tmp, "request limit: %d, remaining: %d, timeout: %gs, remaining: %gs.",
				serverInfo.keepAliveMax,
				serverInfo.keepAliveMaxRemaining,
				serverInfo.keepAliveTimeout / 1e6,
				(serverInfo.keepAliveTimeoutNext - Utilities::getAbsTime()) / 1e6);
	}
	ret.append(tmp);
	return ret;
}

string DashHttp::connectionState2String() const
{
	string ret;
	char tmp[2048];
	sprintf(tmp, "completed requests: %d, TCP state: %s.", numReqsCompletedOverCurrentTcpConnection, Statistics::tcpState2String(lastTcpInfo->tcpi_state).c_str());
	ret.append(tmp);
	return ret;
}

Usec DashHttp::calculateExpectedHttpTimeout() const
{
	Usec nowAbs = Utilities::getAbsTime();
	if(serverInfo.aHdrReceived) {
		dp2p_assert_v(serverInfo.keepAliveTimeout != -1 && serverInfo.keepAliveTimeoutNext != -1,
				"[%s] serverInfo.keepAliveTimeout: %"PRId64" && serverInfo.keepAliveTimeoutNext: %"PRId64,
				Utilities::getTimeString(nowAbs, false).c_str(), serverInfo.keepAliveTimeout, serverInfo.keepAliveTimeoutNext);
		DBGMSG("Expect HTTP timeout at %s (in %gs).", Utilities::getTimeString(serverInfo.keepAliveTimeoutNext, false).c_str(), (serverInfo.keepAliveTimeoutNext - nowAbs) / 1e6);
		return serverInfo.keepAliveTimeoutNext;
	} else {
		dp2p_assert_v(serverInfo.keepAliveTimeout == -1 && serverInfo.keepAliveTimeoutNext == -1,
				"serverInfo.keepAliveTimeout: %"PRId64" && serverInfo.keepAliveTimeoutNext: %"PRId64,
				serverInfo.keepAliveTimeout, serverInfo.keepAliveTimeoutNext);
		DBGMSG("Do not know servers KeepAlive timeout (no header received yet).");
		return std::numeric_limits<Usec>::max();
	}
}

void DashHttp::logTCPState(const char* reason)
{
    updateTcpInfo();
    Statistics::recordTcpState(id, reason, *lastTcpInfo);
}

#if 0
int64_t DashHttp::getOutstandingPldBytes() const
{
    if(!reqQueue.front()->sent())
        return 0;

    int64_t cnt = 0;
    for(std::list<HttpRequest*>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it)
    {
        const HttpRequest& req = **it;
        if(req.sent()) {
            if(req.pldSize > 0) {
                dp2p_assert(req.ifPldSizeApprox || req.pldBytesReceived < req.pldSize);
                cnt += max<int64_t>(0, req.pldSize - req.pldBytesReceived);
            }else {
                return -1;
            }
        } else {
            break;
        }
    }

    dp2p_assert(cnt > 0);
    return cnt;
}
#endif

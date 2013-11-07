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
#include "HttpEvent.h"
#include "TcpConnectionManager.h"
#include "SourceManager.h"

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
#include <sys/ioctl.h>


// TODO: when reconnecting, try to reuse port number
// TODO: assert that there are no transmission holes by checking if all received TCP segments have maximum segment size
// TODO: do HEAD requests to better predict when a segment download will finish
// TODO: properly handle keep-alive timeout of the HTTP server. do not exceed this value or reconnect if exceeded
// TODO: properly handle the max number of requests per connection of the HTTP server. open a second connection in order to maintain pipelining across the max requests boundary???
// TODO: handle disconnection events during sending or receiving

namespace dashp2p {

DashHttp::DashHttp(const TcpConnectionId& tcpConnectionId, HttpCb cb)
  : tcpConnectionId(tcpConnectionId),
    state(DashHttpState_Undefined),
    ifTerminating(false),
    fdWakeUpSelect(-1),
    reqQueue(),
    newReqs(),
    newReqsMutex(),
    fdNewReqs(-1),
    mainThread(),
    cb(cb)
{
    /* Initialize synchronization variables. */
    ThreadAdapter::mutexInit(&newReqsMutex);

    /* Connect and bind */
    //this->connect();

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

    /* Disconnect */
    //TcpConnectionManager::get(id).disconnect();

    DBGMSG("DashHttp terminated.");
}

void DashHttp::threadMain()
{
    TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);

    /* Establish TCP connection */
    const int64_t tic = Utilities::getAbsTime();
    int64_t toc = tic;
    while(!ifTerminating && tc.connect()) {
        toc = Utilities::getAbsTime();
        if(toc - tic >= tc.connectTimeout) {
            const SourceData& sd = SourceManager::get(tc.srcId);
            ERRMSG("Could not connect to %s within %f seconds.", sd.hostName.c_str(), tc.connectTimeout / 1e6);
            reportDisconnect();
            return;
        }
    }

	//HttpEventConnected* e = new HttpEventConnected(id);
	//DBGMSG("Before cb().");
	//cb(e);
	//DBGMSG("cb() returned.");

    while(!ifTerminating)
    {
    	/* wait for events */
    	const int64_t to = calculateWaitingTimeout();
    	const InternalEvent ev = waitForEvents(to);

        if(ifTerminating) {
        	//reportDisconnect();
        	break;
        }

        /* process events */
        if(ev.socketEvent) {
        	TcpConnectionManager::logTCPState(tcpConnectionId, "before recv");
        	const int bytesReceived = TcpConnectionManager::get(tcpConnectionId).read();
        	TcpConnectionManager::logTCPState(tcpConnectionId, "after recv");
        	const bool socketDisconnected = TcpConnectionManager::get(tcpConnectionId).state() != TCP_ESTABLISHED;

        	if(bytesReceived > 0)
        		processNewData();

        	if(socketDisconnected) { // || (serverInfo.keepAliveMaxRemaining == 0 && getNumPendingRequests() == 0)) {
        		//reportDisconnect();
        		break;
        		//WARNMSG("[%s] Socket disconnected. Forced re-connect.", Utilities::getTimeString(0, false).c_str());
        		//forceReconnect();
        		//Statistics::recordReconnect(Utilities::getTime(), ReconnectReason_Other);
        	}
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
        if(ev.newRequests)
        	checkStartNewRequests();
    }

    if(!ifTerminating)
    	reportDisconnect();

#if 0
        const int64_t expectedHttpTimeout = calculateExpectedHttpTimeout();
        const int64_t absNow = Utilities::getAbsTime();
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

bool DashHttp::newRequest(const list<int>& reqs)
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
    for(list<int>::const_iterator it = reqs.begin(); it != reqs.end(); ++it) {
        newReqs.push_back(*it);
        // TODO: make usage of ifData information in DashHttp and in HttpRequest and in Control and in ControlLogic and Statistics consistent
        //HttpRequestManager::setDevName(*it, ifData.name);
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

int64_t DashHttp::calculateWaitingTimeout()
{
	return 1000000;

#if 0
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
	const int64_t expectedHttpTimeout = calculateExpectedHttpTimeout();
	int64_t nowAbs = Utilities::getAbsTime();

	/* Calculate select timeout */
	const int64_t selectTimeout = max<int64_t>(0, min<int64_t>(1000000, expectedHttpTimeout - nowAbs));

	ret.tv_sec = selectTimeout / 1000000;
	ret.tv_usec = selectTimeout % 1000000;
	return ret;
#endif
}

DashHttp::InternalEvent DashHttp::waitForEvents(const int64_t& to)
{
	fd_set fdSetRead;
	FD_ZERO(&fdSetRead);
	int nfds = 0;

	FD_SET(TcpConnectionManager::get(tcpConnectionId).fdSocket, &fdSetRead);
	nfds = max<int>(nfds, TcpConnectionManager::get(tcpConnectionId).fdSocket);

	FD_SET(fdNewReqs, &fdSetRead);
	nfds = max<int>(nfds, fdNewReqs);

	FD_SET(fdWakeUpSelect, &fdSetRead);
	nfds = max<int>(nfds, fdWakeUpSelect);

	DBGMSG("Going to select() for up to %gs.", Utilities::getTimeString(0, false).c_str(), to / 1e6);

	const int64_t ticBeforeSelect = Utilities::getAbsTime();
	struct timeval _to = Utilities::usec2tv(to);
	const int retSelect = select(nfds + 1, &fdSetRead, NULL, NULL, &_to);
	const int64_t tocAfterSelect = Utilities::getAbsTime();
	if(retSelect < 0) {
		printf("errno = %d\n", errno);
		throw std::runtime_error("Problem while waiting in select().");
	} else if(retSelect == 0) {
		DBGMSG("Was in select() for %gs, returning due to timeout.", (tocAfterSelect - ticBeforeSelect) / 1e6);
		return InternalEvent();
	}

	InternalEvent ev;
	ev.socketEvent = FD_ISSET(TcpConnectionManager::get(tcpConnectionId).fdSocket, &fdSetRead);
	ev.newRequests = FD_ISSET(fdNewReqs, &fdSetRead);
	return ev;
}

/*int DashHttp::checkIfSocketHasData()
{
	int bytesAvailable = -1;
	dp2p_assert(-1 != ioctl(fdSocket, FIONREAD, &bytesAvailable));
	return bytesAvailable;
}*/

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
	HttpEventDisconnect* e = new HttpEventDisconnect(tcpConnectionId, reqQueue);
	reqQueue.clear();
	DBGMSG("Before cb().");
	cb(e);
	DBGMSG("cb() returned.");
}

#if 0
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
    	int64_t ticBeforeConnect = Utilities::getAbsTime();
    	retVal = ::connect(fdSocket, (struct sockaddr*)&serverInfo.hostAddr, sizeof(serverInfo.hostAddr));
    	int connectErrno = errno;
    	int64_t tocAfterConnect = Utilities::getAbsTime();
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
#endif

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

/*void DashHttp::assertSocketHealth() const
{
    unsigned err = -1;
    socklen_t errSize = sizeof(err);
    dp2p_assert(0 == getsockopt(fdSocket, SOL_SOCKET, SO_ERROR, &err, &errSize));
    dp2p_assert(err == 0);
}*/

string DashHttp::getUnexpectedDisconnectMessage() const
{
    char buf[2048];
    if(!reqQueue.empty())
    {
        const int reqId = reqQueue.front();
        if(HttpRequestManager::getPldBytesReceived(reqId) > 0) {
            sprintf(buf, "Server closed the connection in the middle of request %d. %" PRId64 " bytes of payload (out of %" PRId64 ") were received.", reqId,
            		HttpRequestManager::getPldBytesReceived(reqId), HttpRequestManager::getContentLength(reqId));
        } else if(HttpRequestManager::isHdrCompleted(reqId)) {
            sprintf(buf, "Server closed the connection after receiving header of request %d. No payload was received.", reqId);
        } else if(HttpRequestManager::getHdrBytesReceived(reqId) > 0){
            sprintf(buf, "Server closed the connection in the middle of request %d. %u bytes of header were received (header incomplete).",
            		reqId, HttpRequestManager::getHdrBytesReceived(reqId));
        } else if(!HttpRequestManager::isSent(reqId)){
            sprintf(buf, "Server closed the connection while waiting for request %d. No header byte were received.", reqId);
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
    for(list<int>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it) {
        if(HttpRequestManager::isSent(*it)) {
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
    for(list<int>::const_iterator it = reqQueue.begin(); it != reqQueue.end() && HttpRequestManager::isSent(*it); ++it)
        ++ret;
    return ret;
}

string DashHttp::reqQueue2String() const
{
	int numPending = 0;
	int first = -1;
	int last = -1;
	for(list<int>::const_iterator it = reqQueue.begin(); it != reqQueue.end(); ++it) {
		if(HttpRequestManager::isSent(*it)) {
			if(first == -1)
				first = (*it);
			last = (*it);
			++numPending;
		} else {
			break;
		}
	}

	char tmp[2048];
	string ret;
	if(!reqQueue.empty())
		sprintf(tmp, "Queue length: %u (first: %d, last: %d). Pending: %d (first: %d, last: %d).",
				(unsigned)reqQueue.size(), reqQueue.front(), reqQueue.back(), numPending, first, last);
	else
		sprintf(tmp, "Queue empty.");
	ret.append(tmp);
	return ret;
}

#if 0
void DashHttp::readFromSocket(int bytesExpected)
{
	dp2p_assert_v(recvBufContent == 0 && bytesExpected > 0, "recvBufContent: %" PRId32 ", bytesExpected: %" PRId32, recvBufContent, bytesExpected);

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
#endif

void DashHttp::processNewData()
{
	TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);
	SourceData& sd = SourceManager::get(tc.srcId);

    /* Process buffer content */
    int recvBufProcessed = 0;
    while(recvBufProcessed < tc.recvBufContent)
    {
        //DBGMSG("%u bytes in buffer, %u bytes processed, %u bytes remaining.", recvBufContent, recvBufProcessed, recvBufContent - recvBufProcessed);

        /* Get the latest request from the request queue. */
        dp2p_assert(!reqQueue.empty());
        int reqId = reqQueue.front();

        //const int hdrBytesBefore = req->hdrBytesReceived;
        const int pldBytesBefore = HttpRequestManager::getPldBytesReceived(reqId);
        const int parsed = parseData(tc.recvBuf + recvBufProcessed, tc.recvBufContent - recvBufProcessed, reqId, tc.recvTimestamp);

        /* If header completed */
        if(HttpRequestManager::isHdrCompleted(reqId))
        {
        	const HttpHdr& hdr = HttpRequestManager::getHdr(reqId);

            switch(hdr.statusCode) {
        	case HTTP_STATUS_CODE_OK:
        	{
        	    /* If this was the first header from this server, initialize server info */
        	    if(!tc.aHdrReceived)
        	    {
        		    dp2p_assert(hdr.keepAliveMax > 0 && hdr.keepAliveTimeout > 0);
            		tc.aHdrReceived = true;
            		sd.keepAliveMax = hdr.keepAliveMax + 1;
            		tc.keepAliveMaxRemaining = max<int>(0, sd.keepAliveMax - getNumPendingRequests());
        	    	sd.keepAliveTimeout = hdr.keepAliveTimeout;
        		    tc.keepAliveTimeoutNext = Utilities::getAbsTime() + sd.keepAliveTimeout;
        		    INFOMSG("Server accepts up to %d requests over a TCP connection. Timeout: %gs.", sd.keepAliveMax, sd.keepAliveTimeout / 1e6);

        		    requeuePendingRequests(tc.keepAliveMaxRemaining + 1);
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
        HttpRequestManager::recordDownloadProgress(reqId, DownloadProcessElement(Utilities::getAbsTime(), parsed));

        /* Act if current request completed */
        if(HttpRequestManager::isCompleted(reqId))
        {
        	const HttpHdr& hdr = HttpRequestManager::getHdr(reqId);

            if(hdr.statusCode != HTTP_STATUS_CODE_FOUND && (tc.keepAliveMaxRemaining == 0 || hdr.connectionClose == 1)) {
            	dp2p_assert_v(tc.keepAliveMaxRemaining == 0 && hdr.connectionClose == 1, "Header: %s.", hdr.toString().c_str());
                dp2p_assert_v(recvBufProcessed == tc.recvBufContent, "processed: %d, content: %d", recvBufProcessed, tc.recvBufContent);
            }

            ++ tc.numReqsCompleted;

            DBGMSG("Request %d completed. Total over current TCP connection: %d.", reqId, tc.numReqsCompleted);

            reqQueue.pop_front();
        }

        /* If at least the header is completed, give data to the user. */
        if(HttpRequestManager::isHdrCompleted(reqId) && HttpRequestManager::getPldBytesReceived(reqId) > 0) {
        	HttpEventDataReceived* eventDataReceived = new HttpEventDataReceived(tcpConnectionId, reqId, pldBytesBefore, HttpRequestManager::getPldBytesReceived(reqId) - 1);
        	DBGMSG("Before cb().");
        	cb(eventDataReceived);
        	DBGMSG("cb() returned.");
        	//cb(req->reqId, ifData.name, recvBuf + recvBufProcessed + (req->hdrBytesReceived - hdrBytesBefore), pldBytesBefore, req->pldBytesReceived - 1, req->hdr.contentLength);
        } else if(HttpRequestManager::isHdrCompleted(reqId)) {
        	HttpEventDataReceived* eventDataReceived = new HttpEventDataReceived(tcpConnectionId, reqId, 0, 0);
        	DBGMSG("Before cb().");
        	cb(eventDataReceived);
        	DBGMSG("cb() returned.");
        	//cb(req->reqId, ifData.name, NULL, 0, 0, req->hdr.contentLength);
        }
    }
    tc.recvBufContent = 0;

    if(sd.keepAliveTimeout != -1)
    	tc.keepAliveTimeoutNext = Utilities::getAbsTime() + sd.keepAliveTimeout;
}

void DashHttp::requeuePendingRequests(int numAllowed)
{
    int cnt = 0;
    int cntToRepeat = 0;
    for(list<int>::iterator it = reqQueue.begin(); it != reqQueue.end() && HttpRequestManager::isSent(*it); ++it)
    {
    	++cnt;
    	if(cnt > numAllowed) {
    		HttpRequestManager::markUnsent(*it);
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

    TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);
    SourceData& sd = SourceManager::get(tc.srcId);

    /* If client-side restriction on max number sent requests reached, return. */
    dp2p_assert_v(tc.maxPendingRequests == 0 || numPending <= tc.maxPendingRequests,
    		"packets on the fly: %d, client-side limit: %d.", numPending, tc.maxPendingRequests);
    if(tc.maxPendingRequests > 0 && numPending >= tc.maxPendingRequests) {
        return false;
    }

    /* If server-side restriction on max number sent requests reached, return. */
    if(tc.keepAliveMaxRemaining == 0) {
        return false;
    }

    /* Get the first unsent request */
    list<int>::iterator it = reqQueue.begin();
    for(; it != reqQueue.end() && HttpRequestManager::isSent(*it); ++it)
        continue;

    /* If there are issued requests and the next pending does not allow pipelining, return */
    if(numPending > 0 && HttpRequestManager::ifAllowsPipelining(*it) == false) {
        return false;
    }

    /* Now we know that we can issue at least one new request */

    /* Start new requests */
    list<int> reqsToSend;
    if(tc.keepAliveMaxRemaining != -1)
        DBGMSG("Server accepts up to %d more requests on the existing TCP connection.", tc.keepAliveMaxRemaining);
    else
        DBGMSG("We do not know yet servers keep-alive request limit.");
    for( ; it != reqQueue.end(); ++it)
    {
    	const int reqId = *it;
        if(HttpRequestManager::ifAllowsPipelining(reqId) == false && (numPending > 0 || reqsToSend.size() > 0))
            break;
        if(tc.keepAliveMaxRemaining == 0)
            break;
        reqsToSend.push_back(reqId);
        if(sd.keepAliveMax != -1) // design choice: send unlimited num of requests if don't know servers limit
            --tc.keepAliveMaxRemaining;
        // TODO: might want to improve precision of this time stamp by moving it into sendHttpRequest() (but is not trivial)
        HttpRequestManager::setTsSent(reqId, Utilities::getTime());
        HttpRequestManager::setSentPipelined(reqId, (it == reqQueue.begin()) ? false : true);
        if(tc.maxPendingRequests > 0 && numPending + (int)reqsToSend.size() == tc.maxPendingRequests)
            break;
    }
    DBGMSG("Will send %u more requests. %d remain in the queue.", reqsToSend.size(), reqQueue.size() - reqsToSend.size());
    dp2p_assert(!reqsToSend.empty());

    /* Send the HTTP GET request. */
    sendHttpRequest(reqsToSend);

    return true;
}

void DashHttp::sendHttpRequest(const list<int>& reqsToSend)
{
	TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);
	SourceData& sd = SourceManager::get(tc.srcId);

    /* Formulate GET requests */
    std::string reqBuf;
    for(list<int>::const_iterator it = reqsToSend.begin(); it != reqsToSend.end(); ++it)
    {
        const int reqId = (*it);
        std::string methodString;
        switch (HttpRequestManager::getHttpMethod(reqId)) {
        case HttpMethod_GET:  methodString = "GET";  break;
        case HttpMethod_HEAD: methodString = "HEAD"; break;
        default: dp2p_assert(0); break;
        }

        DBGMSG("%s http://%s/%s", methodString.c_str(), sd.hostName.c_str(), HttpRequestManager::getFileName(reqId).c_str());

        reqBuf.append(methodString); reqBuf.append(" /"); reqBuf.append(HttpRequestManager::getFileName(reqId)); reqBuf.append(" HTTP/1.1\r\n");
        reqBuf.append("User-Agent: CUSTOM\r\n");
        reqBuf.append("Host: "); reqBuf.append(sd.hostName); reqBuf.append("\r\n");
        reqBuf.append("Connection: Keep-Alive\r\n");
        reqBuf.append("\r\n");
    }

    tc.write(reqBuf);

    if(sd.keepAliveTimeout != -1)
    	tc.keepAliveTimeoutNext = Utilities::getAbsTime() + sd.keepAliveTimeout;

    /* Log TCP status. */
    TcpConnectionManager::logTCPState(tcpConnectionId, "after send()");
    //dp2p_assert(fdSocket != -1);
    //assertSocketHealth();
    //updateTcpInfo();
    //Statistics::recordTcpState(id, "after send()", *lastTcpInfo);
}

#if 0
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
#endif

int DashHttp::parseData(const char* p, int size, int reqId, int64_t recvTimestamp)
{
	dp2p_assert(p && size > 0 && !HttpRequestManager::isCompleted(reqId));

	int bytesParsed = 0;

	/* Receive header bytes and store it in the request object */
	if(!HttpRequestManager::isHdrCompleted(reqId))
	{
		const unsigned hdrBytesReceived = HttpRequestManager::getHdrBytesReceived(reqId);
		const char* hdrBytes = HttpRequestManager::getHdrBytes(reqId);

		/* Check if we now have the complete header */
		const char* endOfHeader = NULL;
		if(       hdrBytesReceived >= 1 && size >= 3 && 0 == strncmp(hdrBytes + (hdrBytesReceived - 1), "\r", 1)     && 0 == strncmp(p, "\n\r\n", 3)) {
			endOfHeader = p + 2;
		} else if(hdrBytesReceived >= 2 && size >= 2 && 0 == strncmp(hdrBytes + (hdrBytesReceived - 2), "\r\n", 2)   && 0 == strncmp(p, "\r\n", 2)) {
			endOfHeader = p + 1;
		} else if(hdrBytesReceived >= 3 && size >= 1 && 0 == strncmp(hdrBytes + (hdrBytesReceived - 3), "\r\n\r", 3) && 0 == strncmp(p, "\n", 1)) {
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
				hdrBytesReceived ? "further" : "first", newHdrBytes, reqId, hdrBytesReceived + newHdrBytes,
						endOfHeader ? "" : " NOT", size - newHdrBytes);

		/* Append header bytes */
		HttpRequestManager::appendHdrBytes(reqId, p, newHdrBytes, recvTimestamp);

		// TODO: remove after debugging
		dp2p_assert_v(HttpRequestManager::getHdrBytesReceived(reqId) <= 500, "[%.*s]", HttpRequestManager::getHdrBytesReceived(reqId), HttpRequestManager::getHdrBytes(reqId));

		bytesParsed = newHdrBytes;

		if(HttpRequestManager::isHdrCompleted(reqId))
			parseHeader(reqId);
	}

	if(bytesParsed == size || HttpRequestManager::isCompleted(reqId))
		return bytesParsed;
	else
		dp2p_assert(HttpRequestManager::isHdrCompleted(reqId));

	const int64_t contentLength = HttpRequestManager::getContentLength(reqId);
	const int64_t pldBytesReceived = HttpRequestManager::getPldBytesReceived(reqId);

	/* header is completed, we are parsing payload */
	dp2p_assert(contentLength > 0 && pldBytesReceived < contentLength);

	const unsigned newPldBytes = min<int64_t>(contentLength - pldBytesReceived, (int64_t)(size - bytesParsed));

	DBGMSG("Received bytes [%" PRId64 ", %" PRId64 "] (out of %" PRId64 ") of request %u. Request%s completed. %u bytes left in the buffer.",
			pldBytesReceived, pldBytesReceived + newPldBytes - 1, contentLength, reqId,
			(pldBytesReceived + newPldBytes == contentLength) ? "" : " NOT", size - bytesParsed - newPldBytes);

	/* Append payload bytes */
	HttpRequestManager::appendPldBytes(reqId, p + bytesParsed, newPldBytes, recvTimestamp);

	bytesParsed += newPldBytes;
	return bytesParsed;
}

void DashHttp::parseHeader(int reqId) const
{
	TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);
	SourceData& sd = SourceManager::get(tc.srcId);

	const HttpHdr& hdr = HttpRequestManager::parseHeader(reqId);

	switch(hdr.statusCode) {
	case HTTP_STATUS_CODE_OK: break;
	//case HTTP_STATUS_CODE_FOUND: break;
	default:
		ERRMSG("HTTP returned status code %" PRIu32 " for %s/%s.", hdr.statusCode, sd.hostName.c_str(), HttpRequestManager::getFileName(reqId).c_str());
		abort();
		break;
	}

	/* Sanity checks */
	if(hdr.connectionClose != 1) {
		// fixme Workaround for Windows IIS (doesn't have connection close parameter)
		HttpHdr _hdr = hdr;
		if(_hdr.connectionClose == -1)
			_hdr.connectionClose = 0;
		if(_hdr.keepAliveMax == -1)
			_hdr.keepAliveMax = 10000;
		if(_hdr.keepAliveTimeout == -1)
			_hdr.keepAliveTimeout = 3600;
		HttpRequestManager::replaceHeader(reqId, _hdr);
		// END
		dp2p_assert(hdr.connectionClose == 0);
		dp2p_assert_v(hdr.keepAliveMax > 0 && hdr.keepAliveTimeout > 0, "max: %d, timeout: %" PRId64, hdr.keepAliveMax, hdr.keepAliveTimeout);
		if(sd.keepAliveTimeout != -1) {
			dp2p_assert_v(hdr.keepAliveTimeout == sd.keepAliveTimeout, "Server changed keep-alive timeout from %" PRId64 " to %" PRId64 "??",
					sd.keepAliveTimeout, hdr.keepAliveTimeout);
		}
	}
}

#if 0
string DashHttp::connectionState2String() const
{
	TcpConnection& tc = TcpConnectionManager::get(id);

	string ret;
	char tmp[2048];
	sprintf(tmp, "completed requests: %d, TCP state: %s.", tc.numReqsCompleted, TcpConnectionManager::tcpState2String(tc.lastTcpInfo.tcpi_state).c_str());
	ret.append(tmp);
	return ret;
}
#endif

int64_t DashHttp::calculateExpectedHttpTimeout() const
{
	TcpConnection& tc = TcpConnectionManager::get(tcpConnectionId);
	SourceData& sd = SourceManager::get(tc.srcId);

	int64_t nowAbs = Utilities::getAbsTime();
	if(tc.aHdrReceived) {
		dp2p_assert_v(sd.keepAliveTimeout != -1 && tc.keepAliveTimeoutNext != -1,
				"[%s] serverInfo.keepAliveTimeout: %" PRId64 " && serverInfo.keepAliveTimeoutNext: %" PRId64,
				Utilities::getTimeString(nowAbs, false).c_str(), sd.keepAliveTimeout, tc.keepAliveTimeoutNext);
		DBGMSG("Expect HTTP timeout at %s (in %gs).", Utilities::getTimeString(tc.keepAliveTimeoutNext, false).c_str(), (tc.keepAliveTimeoutNext - nowAbs) / 1e6);
		return tc.keepAliveTimeoutNext;
	} else {
		dp2p_assert_v(sd.keepAliveTimeout == -1 && tc.keepAliveTimeoutNext == -1,
				"serverInfo.keepAliveTimeout: %" PRId64 " && serverInfo.keepAliveTimeoutNext: %" PRId64,
				sd.keepAliveTimeout, tc.keepAliveTimeoutNext);
		DBGMSG("Do not know servers KeepAlive timeout (no header received yet).");
		return std::numeric_limits<int64_t>::max();
	}
}

#if 0
void DashHttp::logTCPState(const char* reason)
{
    updateTcpInfo();
    Statistics::recordTcpState(id, reason, *lastTcpInfo);
}
#endif

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

}

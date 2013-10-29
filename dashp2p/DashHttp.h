/****************************************************************************
 * DashHttp.h                                                               *
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

#ifndef DASHHTTP_H_
#define DASHHTTP_H_

#include "Dashp2pTypes.h"
#include "Utilities.h"
//#include "Statistics.h"
#include "ThreadAdapter.h"
#include "DebugAdapter.h"
#include "HttpRequest.h"
#include <semaphore.h>
#include <string>
#include <list>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

using std::list;
using std::max;
using std::min;
using dash::Usec;
using dash::Utilities;


class DashHttp
{
/* Public methods. */
public:
    /** Constructor that also initializes DashHttp.
     *  @parameter id                      Identifier, provided by caller, used later in call-back functions to identify the connection.
     *  @parameter clientSideRequestLimit  Maximum number of pipelined (not queued) requests. 0 means unlimited.
     *  @parameter cb                      Call-back function for all kinds of events. */
    DashHttp(ConnectionId id, const string& hostName, int port, const IfData& ifData, int clientSideRequestLimit, HttpCb cb);

    virtual ~DashHttp();

    /** Registers new GET or HEAD request(s).
     *  @param reqs  List of HttpRequests. We take over the responsibility to delete them later.
     *  @return      True if the requests were accepted (queued, not sent), false otherwise. It does not tell you if the requests were actually sent. */
    bool newRequest(list<HttpRequest*> reqs);

    /** Convenience method to register one GET or HEAD request. Internally calls the more general method accepting a list of requests. */
    bool newRequest(HttpRequest* req) {return newRequest(list<HttpRequest*>(1, req));}

    int hasRequests();

    //string getIfName() const {return ifData.name;}
    //string getIfAddr() const {return ifData.printAddress();}
    string getIfString() const {return ifData.toString();}

/* Protected methods. */
protected:
    /***** main thread and its components */
    /* Main thread function. */
    void threadMain();
    static void* startThread(void* params);
    struct timeval calculateSelectTimeout();
    pair<bool,bool> waitSelect(struct timeval selectTimeout);
    int checkIfSocketHasData();
    //bool checkIfHaveNewRequests();

    /**** Output to higher instances *****/
    void reportDisconnect();
    void reportKeepAliveMaxReached();

    /* Socket related. */
    void connect();
    //void keepAliveSocket();
    //void forceReconnect();
    pair<int,int> getSocketBufferLengths() const;
    void assertSocketHealth() const;
    string getUnexpectedDisconnectMessage() const;

    /* Queue related stuff. Must be protected by reqQueueMutex */
    bool havePendingRequests() const;
    int getNumPendingRequests() const;
    const HttpRequest& getQueuedRequest(ReqId reqId) const;
    string reqQueue2String() const;

    /***** HTTP related stuff *****/
    void readFromSocket(int bytesExpected);
    void processNewData();
    void requeuePendingRequests(int numAllowed);
    /* Check if new requests can be send and send the appropriate number of them.
     * Must run in the main thread!
     * Returns true, if new GETs were issued. Otherwise false. */
    bool checkStartNewRequests();
    /* Must only be called from checkStartNewRequests(). Must run in the main thread! */
    void sendHttpRequest(const list<ReqId>& reqsToSend);
    int parseData(const char* p, int size, HttpRequest* req, double recvTimestamp);
    void parseHeader(HttpRequest* req) const;
    string serverState2String() const;
    string connectionState2String() const;
    dash::Usec calculateExpectedHttpTimeout() const;

    /* TCP related stuff */
    void updateTcpInfo();
    void logTCPState(const char* reason);

    /***** throughput related stuff *****/
    //int64_t getOutstandingPldBytes() const;

/* Private variables. */
private:
    enum DashHttpState {DashHttpState_Undefined = 0, DashHttpState_Constructed = 1, DashHttpState_NotAcceptingRequests = 2} state;
    ConnectionId id;

    /* Termination flag */
    bool ifTerminating;
    int fdWakeUpSelect;

    /* Request queue, mutex and semaphore for the request queue. */
    list<HttpRequest*> reqQueue;
    list<HttpRequest*> newReqs;
    Mutex newReqsMutex;
    int fdNewReqs;

    /* Main thread. */
    Thread mainThread;

    /* Connection related data. */
    int fdSocket;
    struct tcp_info* lastTcpInfo;
    IfData ifData;
    int clientSideRequestLimit;
    int numConnectEvents;
    int numReqsCompletedOverCurrentTcpConnection;

    /* Server related data */
    class ServerInfo
    {
    public:
    	ServerInfo(string hostName) :
    		hostName(hostName),
    		hostAddr(),
    		keepAliveMax(-1),
    		keepAliveMaxRemaining(-1),
    		keepAliveTimeout(-1),
    		keepAliveTimeoutNext(-1),
    		aHdrReceived(false) {}
    	string hostName;
    	struct sockaddr_in hostAddr;
    	int keepAliveMax;
    	int keepAliveMaxRemaining;
    	Usec keepAliveTimeout;
    	Usec keepAliveTimeoutNext;
    	bool aHdrReceived;
    } serverInfo;

    /* Buffer for reading data from the socket. */
    const int32_t recvBufSize;
    int32_t recvBufContent;
    char* recvBuf;
    double recvTimestamp;

    /* Callback for downloaded data. */
    HttpCb cb;
};

#endif /* DASHHTTP_H_ */

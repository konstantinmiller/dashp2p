/****************************************************************************
 * Dashp2pTypes.h                                                           *
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

#ifndef DASHP2PTYPES_H_
#define DASHP2PTYPES_H_

#define __STDC_FORMAT_MACROS

#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <utility>
#include <string>
#include <sstream>
using std::string;
using std::pair;
using std::ostringstream;

#include "DebugAdapter.h"


/**********
 * Macros *
 **********/

# define dp2p_assert(x)               do {\
                                             if(!(x)) {\
                                                 DebugAdapter::errPrintfWl (__FILE__, __func__, __LINE__, "assert(%s) failed", #x);\
                                                 abort();\
                                             }\
                                      } while(false)

# define dp2p_assert_v(x, msg, ...)   do {\
	                                         if(!(x)) {\
                                                 DebugAdapter::errPrintfWl (__FILE__, __func__, __LINE__, "assert(%s) failed", #x);\
                                                 DebugAdapter::errPrintfWl (__FILE__, __func__, __LINE__, msg, __VA_ARGS__);\
                                                 abort();\
                                             }\
	                                  } while(false)


/**********
 * ... *
 **********/

namespace dashp2p {
enum DASHSegmentType {SEG_MPD, SEG_REG, SEG_INIT};
}

typedef int32_t ConnectionId;

enum ControlType {ControlType_ST = 0, ControlType_pipelinedST = 1, ControlType_MH = 2, ControlType_P2P = 3};

enum ControlLogicEventType {
	Event_ActionRejected,
	Event_Connected,
	Event_DataPlayed,
	Event_DataReceived,
	Event_Disconnect,
	//Event_MpdReceived,
	//Event_KeepAliveMaxReached,
	Event_Pause,
	Event_ResumePlayback,
	Event_StartPlayback
};

enum ControlLogicActionType {
	Action_CloseTcpConnection,
	Action_CreateTcpConnection,
	//Action_FetchHead,
	Action_StartDownload,
	//Action_UpdateContour
};

enum HttpEventType {HttpEvent_DataReceived, HttpEvent_Disconnect, HttpEvent_Connected};

/* Type for the callback for downloaded data. segNr is -1 for MPD, otherwise >= 0.
 * Arguments: (reqId, devName, buffer, bytesFrom, bytesTo, bytesTotal, RTT) */
//typedef void(*CallbackForDownloadedData)(unsigned, string, const char*, unsigned, unsigned, unsigned);
class HttpEvent;
typedef void(*HttpCb)(HttpEvent* e);

#if 0
/* Type for storing information about a representation. */
class Representation{
public:
    Representation(): ID(), bandwidth(0), _nominalSegmentDuration(0) {}
    string ID;
    int bandwidth; // [bit/sec]
    dash::Usec _nominalSegmentDuration;
    //int segmentDuration; // [sec]
};
#endif

/* Network interface information */
class IfData {
public:
    IfData(): initialized(false), name(), sockaddr_in(), primary(false) {}
    ~IfData(){}
    bool hasAddr() const {return (sockaddr_in.sin_addr.s_addr != 0);}
    string toString() const {
    	ostringstream ret;
    	if(initialized)
    		ret << (name.empty() ? "NoName" : name) << "," << (primary ? "primary" : "secondary") << "," << (hasAddr() ? inet_ntoa(sockaddr_in.sin_addr) : "0.0.0.0");
    	else
    		ret << "<interface not initialized>";
    	return ret.str();
    }
    bool initialized;
    std::string name;
    struct sockaddr_in sockaddr_in;
    bool primary;
};

/* HTTP methods */
enum HttpMethod {HttpMethod_GET, HttpMethod_HEAD};

/* HTTP status codes. */
enum HTTPStatusCode {HTTP_STATUS_CODE_UNDEFINED = 0, HTTP_STATUS_CODE_OK = 200, HTTP_STATUS_CODE_FOUND = 302};

enum ReconnectReason {
	ReconnectReason_HttpKeepAliveMax = 1,
	ReconnectReason_HttpKeepAliveMaxPreventive = 2,
	ReconnectReason_HttpKeepAliveTimeout = 3,
	ReconnectReason_HttpKeepAliveTimeoutPreventive = 4,
	ReconnectReason_Other = 5
};

/* Type to represent TCP states that are missing on the Android system. Copied from netinet/tcp.h on Linux. */
#ifdef __ANDROID__
enum {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING   /* now a valid state */
};
#endif

#endif /* DASHP2PTYPES_H_ */

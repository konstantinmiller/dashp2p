/****************************************************************************
 * dashp2p.h                                                                *
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

#ifndef DASHP2P_H_
#define DASHP2P_H_

#include <cinttypes>
//#include <cstdint>
//#include <utility>
#include <string>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using std::string;
using std::pair;
using std::ostringstream;

namespace dashp2p {

/* HTTP status codes. */
enum HTTPStatusCode {HTTP_STATUS_CODE_UNDEFINED = 0, HTTP_STATUS_CODE_OK = 200, HTTP_STATUS_CODE_FOUND = 302};

/* HTTP methods */
enum HttpMethod {HttpMethod_GET, HttpMethod_HEAD};

enum HttpEventType {HttpEvent_DataReceived, HttpEvent_Disconnect, HttpEvent_Connected};

/* Segment type */
enum DASHSegmentType {SEG_MPD, SEG_REG, SEG_INIT};

/* Callback for receiving events from a HTTP connection */
class HttpEvent;
typedef void(*HttpCb)(HttpEvent* e);

/* Connection ID */
typedef int ConnectionId;

/* Interface information */
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

/* Type of control logic used */
enum ControlType {ControlType_ST = 0, ControlType_pipelinedST = 1, ControlType_MH = 2, ControlType_P2P = 3};

enum ControlLogicEventType {
	Event_ActionRejected = 0,
	Event_Connected = 1,
	Event_DataPlayed = 2,
	Event_DataReceived = 3,
	Event_Disconnect = 4,
	//Event_MpdReceived = 5,
	//Event_KeepAliveMaxReached = 6,
	//Event_Pause = 7,
	//Event_ResumePlayback = 8,
	Event_StartPlayback = 9
};

enum ControlLogicActionType {
	Action_CloseTcpConnection,
	Action_CreateTcpConnection,
	//Action_FetchHead,
	Action_StartDownload,
	//Action_UpdateContour
};

enum ReconnectReason {
	ReconnectReason_HttpKeepAliveMax = 1,
	ReconnectReason_HttpKeepAliveMaxPreventive = 2,
	ReconnectReason_HttpKeepAliveTimeout = 3,
	ReconnectReason_HttpKeepAliveTimeoutPreventive = 4,
	ReconnectReason_Other = 5
};

}

#endif /* DASHP2P_H_ */

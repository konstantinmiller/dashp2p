/****************************************************************************
 * HttpEvent.h                                                              *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 09, 2012                                                 *
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

#ifndef HTTPEVENT_H_
#define HTTPEVENT_H_

//#include "Dashp2pTypes.h"
#include "dashp2p.h"
#include <list>
using std::list;

namespace dashp2p {

class HttpEvent {
public:
	HttpEvent(const TcpConnectionId& tcpConnectionId): tcpConnectionId(tcpConnectionId) {}
	virtual ~HttpEvent(){}
	virtual HttpEventType getType() const = 0;
	virtual const TcpConnectionId& getConnId() const {return tcpConnectionId;}

	virtual string toString() const
	{
		switch(getType()) {
		//case HttpEvent_Connected:    return "Connected";
		case HttpEvent_DataReceived: return "DataReceived";
		case HttpEvent_Disconnect:   return "Disconnect";
		default:
			ERRMSG("Unknown HttpEvent type.");
			abort();
			return string();
		}
	}
private:
	const TcpConnectionId tcpConnectionId;
};


/*class HttpEventConnected: public HttpEvent
{
public:
	HttpEventConnected(int connId): HttpEvent(connId) {}
	virtual ~HttpEventConnected(){}
	virtual HttpEventType getType() const {return HttpEvent_Connected;}
};*/


class HttpEventDataReceived: public HttpEvent
{
public:
	HttpEventDataReceived(const TcpConnectionId& tcpConnectionId, int reqId, int64_t byteFrom, int64_t byteTo)
	  : HttpEvent(tcpConnectionId),
	    byteFrom(byteFrom),
	    byteTo(byteTo),
	    reqId(reqId) {}
	virtual ~HttpEventDataReceived() {}
	virtual HttpEventType getType() const {return HttpEvent_DataReceived;}

public:
	const int64_t byteFrom;
	const int64_t byteTo;
	const int reqId;

};


class HttpEventDisconnect: public HttpEvent
{
public:
	HttpEventDisconnect(const TcpConnectionId& tcpConnectionId, list<int> reqs): HttpEvent(tcpConnectionId), reqs(reqs) {}
	virtual ~HttpEventDisconnect(){}
	virtual HttpEventType getType() const {return HttpEvent_Disconnect;}

public:
	const list<int> reqs;
};

}

#endif /* HTTPEVENT_H_ */

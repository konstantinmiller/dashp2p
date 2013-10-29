/****************************************************************************
 * ControlLogicEvent.h                                                      *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 24, 2012                                                 *
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

#ifndef CONTROLLOGICEVENT_H_
#define CONTROLLOGICEVENT_H_

#include "dashp2p.h"
#include "Utilities.h"
#include <vector>
#include <list>
using std::vector;
using std::list;

namespace dashp2p {

class ControlLogicEvent
{
public:
    ControlLogicEvent() {}
    virtual ~ControlLogicEvent(){}
    virtual ControlLogicEventType getType() const = 0;
    virtual string toString() const {
    	switch(getType()) {
    	case Event_ActionRejected: return "ActionRejected";
    	case Event_Connected:      return "Connected";
    	case Event_DataPlayed:     return "DataPlayed";
    	case Event_DataReceived:   return "DataReceived";
    	case Event_Disconnect:     return "Disconnect";
    	//case Event_Pause:          return "Pause";
    	//case Event_ResumePlayback: return "ResumePlayback";
    	case Event_StartPlayback:  return "StartPlayback";
    	default: ERRMSG("Unknown ControlLogicEvent type."); throw std::runtime_error("Unknown event type.");
    	}
    	return "";
    }
};


class ControlLogicEventActionRejected: public ControlLogicEvent
{
public:
	ControlLogicEventActionRejected(ControlLogicAction* a): a(a) {}
    virtual ~ControlLogicEventActionRejected() {delete a;}
    virtual ControlLogicEventType getType() const {return Event_ActionRejected;}

public:
    ControlLogicAction* a;
};


class ControlLogicEventConnected: public ControlLogicEvent
{
public:
	ControlLogicEventConnected(int connId): connId(connId) {}
    virtual ~ControlLogicEventConnected() {}
    virtual ControlLogicEventType getType() const {return Event_Connected;}

public:
    const ConnectionId connId;
};


class ControlLogicEventDataPlayed: public ControlLogicEvent
{
public:
    ControlLogicEventDataPlayed(pair<int64_t, int64_t> availableContigInterval): ControlLogicEvent(), availableContigInterval(availableContigInterval) {}
    virtual ~ControlLogicEventDataPlayed(){}
    virtual ControlLogicEventType getType() const {return Event_DataPlayed;}

public:
    pair<int64_t, int64_t> availableContigInterval;
};

// TODO: actually, we only need to have the reqId. Everything else is now stored in the HttpRequestManager centrally.
class ControlLogicEventDataReceived: public ControlLogicEvent
{
public:
    ControlLogicEventDataReceived(int32_t connId, int reqId, int64_t byteFrom, int64_t byteTo, pair<int64_t, int64_t> availableContigInterval)
      : ControlLogicEvent(),
        connId(connId),
        reqId(reqId),
        byteFrom(byteFrom),
        byteTo(byteTo),
        availableContigInterval(availableContigInterval)
    {}
    virtual ~ControlLogicEventDataReceived() {}
    virtual ControlLogicEventType getType() const {return Event_DataReceived;}

public:
    int32_t connId;
    int reqId;
    const int64_t byteFrom;
    const int64_t byteTo;
    const pair<int64_t, int64_t> availableContigInterval;
};


class ControlLogicEventDisconnect: public ControlLogicEvent
{
public:
	ControlLogicEventDisconnect(int connId, list<int> reqs): connId(connId), reqs(reqs) {}
    virtual ~ControlLogicEventDisconnect() {}
    virtual ControlLogicEventType getType() const {return Event_Disconnect;}

public:
    const int connId;
    const list<int> reqs;
};


/*class ControlLogicEventPause: public ControlLogicEvent
{
public:
	ControlLogicEventPause(): ControlLogicEvent() {}
    virtual ~ControlLogicEventPause() {}
    virtual ControlLogicEventType getType() const {return Event_Pause;}
};


class ControlLogicEventResumePlayback: public ControlLogicEvent
{
public:
    ControlLogicEventResumePlayback(int64_t startPosition)
      : ControlLogicEvent(),
        startPosition(startPosition) {}
    virtual ~ControlLogicEventResumePlayback(){}
    virtual ControlLogicEventType getType() const {return Event_ResumePlayback;}

public:
    int64_t startPosition;
};*/


class ControlLogicEventStartPlayback: public ControlLogicEvent
{
public:
    ControlLogicEventStartPlayback(const dashp2p::URL& mpdUrl): ControlLogicEvent(), mpdUrl(mpdUrl) {}
    virtual ~ControlLogicEventStartPlayback(){}
    virtual ControlLogicEventType getType() const {return Event_StartPlayback;}

public:
    const dashp2p::URL mpdUrl;
};

}

#endif /* CONTROLLOGICEVENT_H_ */

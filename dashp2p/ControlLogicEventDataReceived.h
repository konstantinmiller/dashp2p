/****************************************************************************
 * ControlLogicEventDataReceived.h                                          *
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

#ifndef CONTROLLOGICEVENTDATARECEIVED_H_
#define CONTROLLOGICEVENTDATARECEIVED_H_


#include "ControlLogicEvent.h"
#include "HttpRequest.h"


class ControlLogicEventDataReceived: public ControlLogicEvent
{
public:
    ControlLogicEventDataReceived(int32_t connId, HttpRequest* req, bool takeOwnership, int64_t byteFrom, int64_t byteTo, pair<dash::Usec, int64_t> availableContigInterval)
      : ControlLogicEvent(),
        takeOwnership(takeOwnership),
        byteFrom(byteFrom),
        byteTo(byteTo),
        availableContigInterval(availableContigInterval),
        connId(connId),
        req(req)
    {}
    virtual ~ControlLogicEventDataReceived(){if(takeOwnership) delete req;}
    virtual ControlLogicEventType getType() const {return Event_DataReceived;}
    virtual int32_t getConnId() const {return connId;}
    virtual const ContentId& getContentId() const {return req->getContentId();}
    virtual ContentType getContentType() const {return req->getContentType();}
    virtual int64_t getContentLength() const {return req->hdr.contentLength;}
    virtual const char* getPldBytes() const {return req->pldBytes;}
    virtual const HttpRequest& peekRequest() const {return *req;}
    virtual HttpRequest* takeRequest() {dp2p_assert(takeOwnership); HttpRequest* ret = req; req = NULL; return ret;}

public:
    const bool takeOwnership;
    const int64_t byteFrom;
    const int64_t byteTo;
    const pair<dash::Usec, int64_t> availableContigInterval;

private:
    int32_t connId;
    HttpRequest* req;
};

#endif /* CONTROLLOGICEVENTDATARECEIVED_H_ */

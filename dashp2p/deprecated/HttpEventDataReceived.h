/****************************************************************************
 * HttpEventDataReceived.h                                                  *
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

#ifndef HTTPEVENTDATARECEIVED_H_
#define HTTPEVENTDATARECEIVED_H_

#include "HttpEvent.h"

class HttpEventDataReceived: public HttpEvent
{
public:
	HttpEventDataReceived(int32_t connId, HttpRequest* req, bool takeOwnership, int64_t byteFrom, int64_t byteTo)
	  : HttpEvent(connId),
		takeOwnership(takeOwnership),
	    byteFrom(byteFrom),
	    byteTo(byteTo),
	    req(req) {}
	virtual ~HttpEventDataReceived(){if(takeOwnership) delete req;}
	virtual HttpEventType getType() const {return HttpEvent_DataReceived;}
	virtual pair<HttpRequest*,bool> takeReq() {HttpRequest* ret = req; req = NULL; return pair<HttpRequest*,bool>(ret,takeOwnership);}
	virtual ContentType getContentType() const {return req->getContentType();}
	virtual HttpMethod getHttpMethod() const {return req->httpMethod;}
	virtual int64_t getContentLength() const {return req->hdr.contentLength;}
	virtual const ContentId& getContentId() const {return req->getContentId();}
	virtual const ReqId& getReqId() const {return req->reqId;}
	virtual const char* getPldBytes() const {return req->pldBytes;}

public:
	/** If true, you MUST delete the HttpRequest embedded in this event! If false, you MUST NOT! */
	const bool takeOwnership;
	const int64_t byteFrom;
	const int64_t byteTo;

private:
	HttpRequest* req;
};

#endif /* HTTPEVENTDATARECEIVED_H_ */

/****************************************************************************
 * HttpRequest.h                                                            *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 5, 2012                                                  *
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

#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_


#include "ContentId.h"
#include <list>
#include <vector>
#include <string>
#include <limits>
using std::string;
using std::vector;
using std::pair;
using std::list;

typedef int ReqId;
class DownloadProcessElement {
public:
	DownloadProcessElement(dash::Usec ts_us, int byte): ts_us(ts_us), byte(byte) {}
	dash::Usec ts_us;
	int  byte;
};
typedef list<vector<DownloadProcessElement> > DownloadProcess;


/* Type to hold information about an HTTP request. */
class HttpRequest
{
public:
	/** Constructor.
	 *  @param contentId  Provided by the caller for later identification of the downloaded data in the call-back. We take over the memory management. */
    HttpRequest(const ContentId* contentId, const string& hostName, const string& file, bool withPipelining, HttpMethod httpMethod);

    virtual ~HttpRequest();

    const ContentId& getContentId() const {return *contentId;}
    ContentType getContentType() const {return contentId->getType();}
    char* getContentCopy() const;

    void recordDownloadProgress(DownloadProcessElement el);
    bool sent() const {return (tsSent != -1);}
    void markUnsent() {tsSent = -1;} // TODO: assert consistency here?
    bool completed() const;

public:
    const ReqId reqId;

    const string hostName;
    const string file;
    string devName;

    const bool allowPipelining; // if yes, this request might get sent out before the previous one was completed. this has no impact on pipelining behavior of the following request.
    bool sentPipelined;  // if yes, this request was sent before the previous one was completed

    const HttpMethod httpMethod;

    class HttpHdr {
    public:
    	HttpHdr() : statusCode(HTTP_STATUS_CODE_UNDEFINED), keepAliveMax(-1), keepAliveTimeout(-1), contentLength(-1), connectionClose(-1) {}
    	string toString() const;
    	HTTPStatusCode statusCode;
    	int keepAliveMax;
    	dash::Usec keepAliveTimeout;
    	int64_t contentLength;
    	int connectionClose;
    };
    HttpHdr hdr;

    unsigned hdrBytesReceived;
    char*    hdrBytes;
    bool     hdrCompleted;

    int64_t pldBytesReceived;
    char* pldBytes; // TODO: make pldBytes pointing directly into SegmentStorage in order to avoid copying
    DownloadProcess* downloadProcess; // ([us],[byte])

    double tsSent;
    double tsFirstByte;
    double tsLastByte;

private:
    const ContentId* const contentId;

    static unsigned nextReqId;
};

#endif /* HTTPREQUEST_H_ */

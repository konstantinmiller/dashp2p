/****************************************************************************
 * HttpRequest.cpp                                                          *
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


#include "Dashp2pTypes.h"
#include "HttpRequest.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>


unsigned HttpRequest::nextReqId = 0;


HttpRequest::HttpRequest(const ContentId* contentId, const string& hostName, const string& file, bool allowPipelining, HttpMethod httpMethod)
  : reqId(nextReqId),
    hostName(hostName),
    file(file),
    devName(),
    allowPipelining(allowPipelining),
    sentPipelined(false),
    httpMethod(httpMethod),
    hdr(),
    hdrBytesReceived(0),
    hdrBytes(NULL),
    hdrCompleted(false),
    pldBytesReceived(0),
    pldBytes(NULL),
    downloadProcess(NULL),
    tsSent(-1),
    tsFirstByte(-1),
    tsLastByte(-1),
    contentId(contentId)
{
	dp2p_assert(contentId);

    downloadProcess = new DownloadProcess;
    downloadProcess->push_back(std::vector<DownloadProcessElement>());
    downloadProcess->back().reserve(1024);

    ++nextReqId;
}

HttpRequest::~HttpRequest()
{
	delete contentId;

    if(hdrBytes)
        delete [] hdrBytes;

    if(pldBytes)
    	delete [] pldBytes;

    if(downloadProcess)
        delete downloadProcess;
}

char* HttpRequest::getContentCopy() const
{
	dp2p_assert(completed());
	char* buf = new char[pldBytesReceived];
	dp2p_assert(buf);
	memcpy(buf, pldBytes, pldBytesReceived);
	return buf;
}

void HttpRequest::recordDownloadProgress(DownloadProcessElement el)
{
    if(downloadProcess->back().size() == downloadProcess->back().capacity()) {
        downloadProcess->push_back(std::vector<DownloadProcessElement>());
        downloadProcess->back().reserve(1024);
    }
    downloadProcess->back().push_back(el);
}

bool HttpRequest::completed() const
{
	if(hdr.statusCode == HTTP_STATUS_CODE_FOUND)
		return hdrCompleted;

    switch(httpMethod) {
    case HttpMethod_GET:
        return (hdrCompleted && hdr.contentLength == pldBytesReceived);
    case HttpMethod_HEAD:
        return hdrCompleted;
    default:
        dp2p_assert_v(false, "Unknown HTTP method. Bug?", NULL);
        return false;
    }
}

string HttpRequest::HttpHdr::toString() const
{
	static char tmp[2048];
	sprintf(tmp, "Status: %d. Keep-Alive max: %d, timeout: %"PRId64". Content-length: %"PRId64". Connection: %d.",
			statusCode, keepAliveMax, keepAliveTimeout, contentLength, connectionClose);
	return string(tmp);
}

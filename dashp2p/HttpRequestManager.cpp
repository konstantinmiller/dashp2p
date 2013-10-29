/****************************************************************************
 * HttpRequestManager.cpp                                                   *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jul 07, 2012                                                 *
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

#include "HttpRequestManager.h"
//#include "Dashp2pTypes.h"

#include <cassert>
//#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace dashp2p {

const int HttpRequestManager::s = 1024;
vector<vector<HttpRequestManager::HttpRequest*>* > HttpRequestManager::reqs;
Mutex HttpRequestManager::mutex;

int HttpRequestManager::newHttpRequest(const int& connId, const ContentId* contentId, /*const string& hostName,*/ const string& file, bool withPipelining, HttpMethod httpMethod)
{
	ThreadAdapter::mutexLock(&mutex);

	if(reqs.empty() || reqs.back()->size() == reqs.back()->capacity()) {
		reqs.push_back(new vector<HttpRequest*>);
		reqs.back()->reserve(s);
	}

	const int reqId = s * (reqs.size() - 1) + reqs.back()->size();
	reqs.back()->push_back(new HttpRequest(connId, contentId, file, withPipelining, httpMethod));

	ThreadAdapter::mutexUnlock(&mutex);

	return reqId;
}

void HttpRequestManager::appendHdrBytes(int reqId, const void* p, int newHdrBytes, int64_t recvTimestamp)
{
	HttpRequest* req = reqs.at(reqId / s)->at(reqId % s);

	/* Save the received header data. Might remove this later for optimization purposes. */
	const char* oldHdrData = req->hdrBytes;
	req->hdrBytes = new char[req->hdrBytesReceived + newHdrBytes + 1];
	memcpy(req->hdrBytes, oldHdrData, req->hdrBytesReceived);
	memcpy(req->hdrBytes + req->hdrBytesReceived, p, newHdrBytes);
	req->hdrBytes[req->hdrBytesReceived + newHdrBytes] = 0; // zero termination of the header string
	delete [] oldHdrData;

	/* Update header information */
	if(req->hdrBytesReceived == 0)
		req->tsFirstByte = recvTimestamp;
	req->hdrBytesReceived += newHdrBytes;
	req->hdrCompleted = (0 == memcmp(req->hdrBytes + (req->hdrBytesReceived - 4), "\r\n\r\n", 4));
}

void HttpRequestManager::appendPldBytes(int reqId, const void* p, int newPldBytes, int64_t recvTimestamp)
{
	HttpRequest* req = reqs.at(reqId / s)->at(reqId % s);

	if(req->pldBytesReceived + newPldBytes == req->hdr.contentLength)
		req->tsLastByte = recvTimestamp;
	if(!req->pldBytes)
		req->pldBytes = new char[req->hdr.contentLength];
	memcpy(req->pldBytes + req->pldBytesReceived, p, newPldBytes);
	req->pldBytesReceived += newPldBytes;
}

const HttpHdr& HttpRequestManager::parseHeader(int reqId)
{
	HttpRequest* req = reqs.at(reqId / s)->at(reqId % s);

	/* Parse each line of the header. */
	int cnt = 0;
	for(const char* pos = req->hdrBytes; pos[0] != '\r' || pos[1] != '\n'; pos = 2 + strstr(pos, "\r\n"))
	{
		/* Debug: output next header line */
		const int lineLength = strstr(pos, "\r\n") - pos;
		DBGMSG("HTTP header line %02d: [%.*s]", cnt++, lineLength, pos);

		/* Parse first line of the header (containing status code) */
		if(0 == strncmp(pos, "HTTP/1.1", 8))
		{
			/* Parse status code. */
			int32_t _httpStatusCode = 0;
			dp2p_assert(1 == sscanf(pos, "HTTP/1.1 %" SCNd32, &_httpStatusCode) && _httpStatusCode != 0);
			switch(_httpStatusCode) {
			case 200: req->hdr.statusCode = HTTP_STATUS_CODE_OK; break;
			case 302: req->hdr.statusCode = HTTP_STATUS_CODE_FOUND; break;
			default:
				ERRMSG("HTTP returned status code %" PRIu32 " for %s.", _httpStatusCode, req->file.c_str());
				throw std::runtime_error("HTTP returned bad status code.");
			}
		}

		/* Parse content length */
		else if(0 == strncmp(pos, "Content-Length:", 15))
		{
			req->hdr.contentLength = -1;
			dp2p_assert(1 == sscanf(pos, "Content-Length: %" SCNd64, &req->hdr.contentLength) && req->hdr.contentLength >= 0);
		}

		/* Parse "Connection:" line */
		else if(0 == strncmp(pos, "Connection:", 11))
		{
			if(0 == strncmp(pos, "Connection: close", 17)) {
				req->hdr.connectionClose = 1;
			} else {
				dp2p_assert_v(0 == strncmp(pos, "Connection: Keep-Alive", 22), "Unknown HTTP header line: %.*s.", lineLength, pos);
				req->hdr.connectionClose = 0;
			}
		}

		/* Parse Keep-Alive: line */
		else if(0 == strncmp(pos, "Keep-Alive:", 11))
		{
			const char* posMax = strstr(pos, "max=");
			dp2p_assert(posMax && posMax - pos < lineLength);
			dp2p_assert_v(1 == sscanf(posMax, "max=%d", &req->hdr.keepAliveMax) && req->hdr.keepAliveMax > 0, "Error parsing HTTP header line: %.*s.", lineLength, pos);

			const char* posTimeout = strstr(pos, "timeout=");
			dp2p_assert(posTimeout && posTimeout - pos < lineLength);
			dp2p_assert_v(1 == sscanf(posTimeout, "timeout=%" SCNd64, &req->hdr.keepAliveTimeout) && req->hdr.keepAliveTimeout > 0, "Error parsing HTTP header line: %.*s.", lineLength, pos);
			req->hdr.keepAliveTimeout *= 1000000;
		}

		// TODO: parse the keep-alive parameter and limit the length of the pipeline. consider removing httpclosereceived after that.
		// TODO: even better: open a new TCP connection in advance before the other one gets closed!
	}

	/* More sanity checks */
	if(req->hdr.connectionClose == 1) {
		dp2p_assert_v(req->hdr.keepAliveMax == -1 && req->hdr.keepAliveTimeout == -1, "max: %d, timeout: %" PRId64, req->hdr.keepAliveMax, req->hdr.keepAliveTimeout);
	}

	DBGMSG("Header parsed: %s", req->hdr.toString().c_str());

	return req->hdr;
}

// TODO: remove this function once no longer needed
void HttpRequestManager::replaceHeader(int reqId, const HttpHdr& newHdr)
{
	HttpRequest* req = reqs.at(reqId / s)->at(reqId % s);
	req->hdr = newHdr;
}

void HttpRequestManager::recordDownloadProgress(int reqId, const DownloadProcessElement& el)
{
	reqs.at(reqId / s)->at(reqId % s)->recordDownloadProgress(el);
}

void HttpRequestManager::markUnsent(int reqId)
{
	reqs.at(reqId / s)->at(reqId % s)->markUnsent();
}

/*const string& HttpRequestManager::getDevName(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->devName;
}*/

/*const string& HttpRequestManager::getHostName(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hostName;
}*/

const string& HttpRequestManager::getFileName(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->file;
}

ContentType HttpRequestManager::getContentType(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->getContentType();
}

HttpMethod HttpRequestManager::getHttpMethod(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->httpMethod;
}

int64_t HttpRequestManager::getContentLength(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hdr.contentLength;
}

const ContentId& HttpRequestManager::getContentId(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->getContentId();
}

const char* HttpRequestManager::getPldBytes(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->pldBytes;
}

int64_t HttpRequestManager::getPldBytesReceived(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->pldBytesReceived;
}

const char* HttpRequestManager::getHdrBytes(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hdrBytes;
}

unsigned HttpRequestManager::getHdrBytesReceived(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hdrBytesReceived;
}

const HttpHdr& HttpRequestManager::getHdr(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hdr;
}

int64_t HttpRequestManager::getTsSent(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->tsSent;
}

int64_t HttpRequestManager::getTsFirstByte(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->tsFirstByte;
}

int64_t HttpRequestManager::getTsLastByte(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->tsLastByte;
}

const DownloadProcess& HttpRequestManager::getDownloadProcess(int reqId)
{
	return *reqs.at(reqId / s)->at(reqId % s)->downloadProcess;
}

bool HttpRequestManager::isHdrCompleted(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->hdrCompleted;
}

bool HttpRequestManager::isCompleted(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->completed();
}

bool HttpRequestManager::isSentPipelined(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->sentPipelined;
}

bool HttpRequestManager::isSent(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->sent();
}

bool HttpRequestManager::ifAllowsPipelining(int reqId)
{
	return reqs.at(reqId / s)->at(reqId % s)->allowPipelining;
}

/*void HttpRequestManager::setDevName(const int reqId, const string& devName)
{
	reqs.at(reqId / s)->at(reqId % s)->devName = devName;
}*/

void HttpRequestManager::setTsSent(const int reqId, const int64_t ts)
{
	reqs.at(reqId / s)->at(reqId % s)->tsSent = ts;
}

void HttpRequestManager::setSentPipelined(const int reqId, const bool b)
{
	reqs.at(reqId / s)->at(reqId % s)->sentPipelined = b;
}

void HttpRequestManager::init()
{
	ThreadAdapter::mutexInit(&mutex);
}

void HttpRequestManager::cleanup()
{
	ThreadAdapter::mutexLock(&mutex);

	for(size_t i = 0; i < reqs.size(); ++i) {
		for(size_t j = 0; j < reqs.at(i)->size(); ++j) {
			delete reqs.at(i)->at(j);
		}
	}

	ThreadAdapter::mutexUnlock(&mutex);
	ThreadAdapter::mutexDestroy(&mutex);
}

unsigned HttpRequestManager::HttpRequest::nextReqId = 0;

HttpRequestManager::HttpRequest::HttpRequest(const int& connId, const ContentId* contentId, /*const string& hostName,*/ const string& file, bool allowPipelining, HttpMethod httpMethod)
  : connId(connId),
    reqId(nextReqId),
    //hostName(hostName),
    file(file),
    //devName(),
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

HttpRequestManager::HttpRequest::~HttpRequest()
{
	delete contentId;

    if(hdrBytes)
        delete [] hdrBytes;

    if(pldBytes)
    	delete [] pldBytes;

    if(downloadProcess)
        delete downloadProcess;
}

char* HttpRequestManager::HttpRequest::getContentCopy() const
{
	dp2p_assert(completed());
	char* buf = new char[pldBytesReceived];
	dp2p_assert(buf);
	memcpy(buf, pldBytes, pldBytesReceived);
	return buf;
}

void HttpRequestManager::HttpRequest::recordDownloadProgress(const DownloadProcessElement& el)
{
    if(downloadProcess->back().size() == downloadProcess->back().capacity()) {
        downloadProcess->push_back(std::vector<DownloadProcessElement>());
        downloadProcess->back().reserve(1024);
    }
    downloadProcess->back().push_back(el);
}

bool HttpRequestManager::HttpRequest::completed() const
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

string HttpHdr::toString() const
{
	static char tmp[2048];
	sprintf(tmp, "Status: %d. Keep-Alive max: %d, timeout: %" PRId64 ". Content-length: %" PRId64 ". Connection: %d.",
			statusCode, keepAliveMax, keepAliveTimeout, contentLength, connectionClose);
	return string(tmp);
}


} /* namespace dp2p */

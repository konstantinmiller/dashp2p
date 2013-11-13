/****************************************************************************
 * HttpRequestManager.h                                                     *
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

#ifndef HTTPREQUESTMANAGER_H_
#define HTTPREQUESTMANAGER_H_

//#include "Dashp2pTypes.h"
#include "dashp2p.h"
#include "ThreadAdapter.h"
#include "ContentId.h"
#include "HttpClientManager.h"

#include <list>
#include <string>
#include <vector>
#include <limits>
using std::vector;
using std::string;
using std::pair;
using std::list;

namespace dashp2p {

class HttpHdr {
public:
	HttpHdr() : statusCode(HTTP_STATUS_CODE_UNDEFINED), keepAliveMax(-1), keepAliveTimeout(-1), contentLength(-1), connectionClose(-1) {}
	string toString() const;
	HTTPStatusCode statusCode;
	int keepAliveMax;
	int64_t keepAliveTimeout;
	int64_t contentLength;
	int connectionClose;
};

class DownloadProcessElement {
public:
	DownloadProcessElement(int64_t ts_us, int byte): ts_us(ts_us), byte(byte) {}
	int64_t ts_us;
	int  byte;
};
typedef list<vector<DownloadProcessElement> > DownloadProcess;


class HttpRequestManager {

/* Public methods */
public:
	static void init();
	static void cleanup();

	/** Creates a new HTTP request.
	 *  @param contentId  Provided by the caller for later identification of the downloaded data in the call-back. We take over the memory management. */
	static int newHttpRequest(const TcpConnectionId& tcpConnectionId, const ContentId* contentId, /*const string& hostName,*/ const string& file, bool withPipelining, HttpMethod httpMethod);

	static void appendHdrBytes(int reqId, const void* p, int newHdrBytes, int64_t recvTimestamp);
	static void appendPldBytes(int reqId, const void* p, int newPldBytes, int64_t recvTimestamp);
	static const HttpHdr& parseHeader(int reqId);
	static void replaceHeader(int reqId, const HttpHdr& newHdr);
	static void recordDownloadProgress(int reqId, const DownloadProcessElement& el);
	static void markUnsent(int reqId);

	// TODO: check if functions below need mutex synchro
	//static const string& getDevName(int reqId);
	//static const string& getHostName(int reqId);
	static const string& getFileName(int reqId);
	static ContentType getContentType(int reqId);
	static HttpMethod getHttpMethod(int reqId);
	static int64_t getContentLength(int reqId);
	static const ContentId& getContentId(int reqId);
	//static const char* getPldBytes(int reqId);
	static int64_t getPldBytesReceived(int reqId);
	static const char* getHdrBytes(int reqId);
    static unsigned getHdrBytesReceived(int reqId);
    static const HttpHdr& getHdr(int reqId);
    static int64_t getTsSent(int reqId);
    static int64_t getTsFirstByte(int reqId);
    static int64_t getTsLastByte(int reqId);
    static const DownloadProcess& getDownloadProcess(int reqId);

    static bool isHdrCompleted(int reqId);
    static bool isCompleted(int reqId);
    static bool isSent(int reqId);
    static bool isSentPipelined(int reqId);
    static bool ifAllowsPipelining(int reqId);

	// TODO: do we need this function?
	//static void setDevName(const int reqId, const string& devName);
	static void setTsSent(const int reqId, const int64_t ts);
	static void setSentPipelined(const int reqId, const bool b);

/* Private methods */
private:
	HttpRequestManager(){}
	virtual ~HttpRequestManager(){}

/* Private types */
private:
	/* Type to hold information about an HTTP request. */
	class HttpRequest
	{
	public:
		/** Constructor.
		 *  @param contentId  Provided by the caller for later identification of the downloaded data in the call-back. We take over the memory management. */
		HttpRequest(const TcpConnectionId& tcpConnectionId, const ContentId* contentId, /*const string& hostName,*/ const string& file, bool withPipelining, HttpMethod httpMethod);

		virtual ~HttpRequest();

		//ContentId& getContentId() {return contentId;}
		ContentType getContentType() const {return contentId.getType();}
		//char* getContentCopy() const;

		void recordDownloadProgress(const DownloadProcessElement& el);
		bool sent() const {return (tsSent != -1);}
		void markUnsent() {tsSent = -1;} // TODO: assert consistency here?
		bool completed() const;

	public:
		const TcpConnectionId tcpConnectionId;
		const int reqId;

		//const string hostName;
		const string file;
		//string devName;

		const bool allowPipelining; // if yes, this request might get sent out before the previous one was completed. this has no impact on pipelining behavior of the following request.
		bool sentPipelined;  // if yes, this request was sent before the previous one was completed

		const HttpMethod httpMethod;

		HttpHdr hdr;

		unsigned hdrBytesReceived;
		char*    hdrBytes;
		bool     hdrCompleted;

		int64_t pldBytesReceived;
		//char* pldBytes; // TODO: make pldBytes pointing directly into SegmentStorage in order to avoid copying
		DownloadProcess* downloadProcess; // ([us],[byte])

		int64_t tsSent;
		int64_t tsFirstByte;
		int64_t tsLastByte;

		const ContentId& contentId;

	private:
		static unsigned nextReqId;
	};

/* Private members */
private:
	static const int s;
	static vector<vector<HttpRequest*>* > reqs;
	static Mutex mutex;
};

} /* namespace dp2p */
#endif /* HTTPREQUESTMANAGER_H_ */

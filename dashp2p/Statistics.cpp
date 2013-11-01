/****************************************************************************
 * Statistics.cpp                                                           *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Aug 1, 2012                                                  *
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

#include "Statistics.h"
#include "Utilities.h"
#include "DebugAdapter.h"
#include "Control.h"
#include "TcpConnectionManager.h"
#include "SourceManager.h"

#include <cassert>
#include <cstdio>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <cmath>
//#include <cinttypes>
#include <fstream>
#include <iostream>

namespace dashp2p {

const MpdWrapper* Statistics::mpdWrapper = NULL;
std::string Statistics::logDir;
//std::list<HttpRequest*> Statistics::rsList;
map<int, list<int> > Statistics::httpRequests;
//TcpConnectionId Statistics::lastTcpConnectionId = -1;
bool  Statistics::logTcpState = false;
map<int, FILE*> Statistics::filesTcpState;
bool  Statistics::logScalarValues = false;
FILE* Statistics::fileScalarValues = NULL;
bool  Statistics::logAdaptationDecision = false;
FILE* Statistics::fileAdaptationDecision = NULL;
bool  Statistics::logGiveDataToVlc = false;
FILE* Statistics::fileGiveDataToVlc = NULL;
bool  Statistics::logBytesStored = false;
FILE* Statistics::fileBytesStored = NULL;
bool  Statistics::logSecStored = false;
FILE* Statistics::fileSecStored = NULL;
//FILE* Statistics::fileSecDownloaded = NULL;
bool  Statistics::logUnderruns = false;
FILE* Statistics::fileUnderruns = NULL;
bool  Statistics::logReconnects = false;
FILE* Statistics::fileReconnects = NULL;
bool  Statistics::logSegmentSizes = false;
FILE* Statistics::fileSegmentSizes = NULL;
bool  Statistics::logRequestStatistics = false;
bool  Statistics::logRequestDownloadProgress = false;

void Statistics::init(const std::string& logDir, const bool logTcpState, const bool logScalarValues, const bool logAdaptationDecision,
		const bool logGiveDataToVlc, const bool logBytesStored, const bool logSecStored, const bool logUnderruns,
		const bool logReconnects, const bool logSegmentSizes, const bool logRequestStatistics,
		const bool logRequestDownloadProgress)
{
    if(logDir.empty())
        return;

    dp2p_assert(mpdWrapper == NULL);

    dp2p_assert(Statistics::logDir.empty());
    Statistics::logDir = logDir;

    //std::list<HttpRequest*> Statistics::rsList;
    dp2p_assert(Statistics::httpRequests.empty());

    //Statistics::lastTcpConnectionId = -1;
    Statistics::logTcpState = logTcpState;
    dp2p_assert(Statistics::filesTcpState.empty());

    Statistics::logScalarValues = logScalarValues;
    dp2p_assert(Statistics::fileScalarValues == NULL);

    Statistics::logAdaptationDecision = logAdaptationDecision;
    dp2p_assert(Statistics::fileAdaptationDecision == NULL);

    Statistics::logGiveDataToVlc = logGiveDataToVlc;
    dp2p_assert(Statistics::fileGiveDataToVlc == NULL);

    Statistics::logBytesStored = logBytesStored;
    dp2p_assert(Statistics::fileBytesStored == NULL);

    Statistics::logSecStored = logSecStored;
    dp2p_assert(Statistics::fileSecStored == NULL);

    //FILE* Statistics::fileSecDownloaded = NULL;

    Statistics::logUnderruns = logUnderruns;
    dp2p_assert(Statistics::fileUnderruns == NULL);

    Statistics::logReconnects = logReconnects;
    dp2p_assert(Statistics::fileReconnects == NULL);

    Statistics::logSegmentSizes = logSegmentSizes;
    dp2p_assert(Statistics::fileSegmentSizes == NULL);

    Statistics::logRequestStatistics = logRequestStatistics;
    Statistics::logRequestDownloadProgress = logRequestDownloadProgress;
}

void Statistics::cleanUp()
{
	mpdWrapper = NULL;

	logDir.erase();

	httpRequests.clear();

	//Statistics::lastTcpConnectionId = -1;
	Statistics::logTcpState = false;
	while(!filesTcpState.empty()) {
		FILE* f = filesTcpState.begin()->second;
		dp2p_assert(f != NULL);
		dp2p_assert(0 == fclose(f));
		filesTcpState.erase(filesTcpState.begin());
	}

	Statistics::logScalarValues = false;
	if(fileScalarValues != NULL) {
		fprintf(fileScalarValues, "</ScalarValues>\n");
		dp2p_assert(0 == fclose(fileScalarValues));
		fileScalarValues = NULL;
	}

	logAdaptationDecision = false;
	if(fileAdaptationDecision != NULL) {
		dp2p_assert(0 == fclose(fileAdaptationDecision));
		fileAdaptationDecision = NULL;
	}

	logGiveDataToVlc = false;
	if(fileGiveDataToVlc != NULL) {
		dp2p_assert(0 == fclose(fileGiveDataToVlc));
		fileGiveDataToVlc = NULL;
	}

	logBytesStored = false;
	if(fileBytesStored != NULL) {
		dp2p_assert(0 == fclose(fileBytesStored));
		fileBytesStored = NULL;
	}

	logSecStored = false;
	if(fileSecStored != NULL) {
		dp2p_assert(0 == fclose(fileSecStored));
		fileSecStored = NULL;
	}

#if 0
if(fileSecDownloaded != NULL) {
	dp2p_assert(0 == fclose(fileSecDownloaded));
	fileSecDownloaded = NULL;
}
#endif

    logUnderruns = false;
    if(fileUnderruns != NULL) {
    	dp2p_assert(0 == fclose(fileUnderruns));
    	fileUnderruns = NULL;
    }

    logReconnects = false;
    if(fileReconnects != NULL) {
    	dp2p_assert(0 == fclose(fileReconnects));
    	fileReconnects = NULL;
    }

    logSegmentSizes = false;
    if(fileSegmentSizes != NULL) {
    	dp2p_assert(0 == fclose(fileSegmentSizes));
    	fileSegmentSizes = NULL;
    }

    Statistics::logRequestStatistics = false;
    Statistics::logRequestDownloadProgress = false;
}

#if 0
int32_t Statistics::registerTcpConnection()
{
	const int32_t tcpConnId = ++lastTcpConnectionId;
	return tcpConnId;
}

void Statistics::unregisterTcpConnection(const int32_t tcpConnId)
{
	if(0 != filesTcpState.count(tcpConnId)) {
		if(filesTcpState.at(tcpConnId) != NULL) {
			dp2p_assert(0 == fclose(filesTcpState.at(tcpConnId)));
		}
		filesTcpState.erase(tcpConnId);
	}
}
#endif

// fixme: change the usage of this functions. is should not be necessary to call it explicitely. all dat amust go statistics immediately.
void Statistics::recordRequestStatistics(const TcpConnectionId& tcpConnectionId, int reqId)
{
	if(0 == httpRequests.count(tcpConnectionId.numeric())) {
		httpRequests.insert(pair<int, list<int> >(tcpConnectionId.numeric(), list<int>()));
	}

    httpRequests.at(tcpConnectionId.numeric()).push_back(reqId);

    if(HttpRequestManager::getContentId(reqId).getType() == ContentType_Segment)
        Control::displayThroughputOverlay(
                dynamic_cast<const ContentIdSegment&>(HttpRequestManager::getContentId(reqId)).segmentIndex(),
                HttpRequestManager::getTsLastByte(reqId),
                Statistics::getThroughputLastRequest(tcpConnectionId));
    else
        Control::displayThroughputOverlay(-1, HttpRequestManager::getTsLastByte(reqId), Statistics::getThroughputLastRequest(tcpConnectionId));
}

int Statistics::numCompletedRequests(const TcpConnectionId& tcpConnectionId)
{
	dp2p_assert(0 < httpRequests.count(tcpConnectionId.numeric()));
	return httpRequests.at(tcpConnectionId.numeric()).size();
}

int Statistics::getLastRequest(const TcpConnectionId& tcpConnectionId)
{
	dp2p_assert(0 < httpRequests.count(tcpConnectionId.numeric()) && !httpRequests.at(tcpConnectionId.numeric()).empty());
    return httpRequests.at(tcpConnectionId.numeric()).back();
}

// TODO: validate throughput calculations (no bugs noticed but just to be sure)
double Statistics::getThroughput(const TcpConnectionId& tcpConnectionId, double delta, string devName)
{
    const double now = dashp2p::Utilities::now();

    dp2p_assert(0 < httpRequests.count(tcpConnectionId.numeric()));
    list<int>& rsList = httpRequests.at(tcpConnectionId.numeric());
    dp2p_assert(!rsList.empty());

    if(delta > now) {
        //printf("delta = %17.17f, now = %17.17f\n", delta, now);fflush(NULL);
        abort();
    }

    double seconds = 0;
    double bits = 0;
    std::list<int>::const_iterator it = --rsList.end();
    while(true)
    {
        const int reqId = *it;
        if(devName.empty() || TcpConnectionManager::get(tcpConnectionId).ifData.name.compare(devName) == 0)
        {
            const double startTime = HttpRequestManager::getTsSent(reqId);
            const double complTime = HttpRequestManager::getTsLastByte(reqId);
            const double bytes = HttpRequestManager::getContentLength(reqId);

            if(complTime <= now - delta) {
                break;
            } else if(startTime >= now - delta) {
                bits += 8.0 * bytes;
                seconds += complTime - startTime;
                //dbgPrintf("Accounting for complete segment with %f bits from %f to %f.", 8.0 * bytes, startTime, complTime);
            } else {
                seconds += complTime - (now - delta);
                bits += (8.0 * bytes) * (complTime - (now - delta)) / (complTime - startTime);
                //dbgPrintf("Accounting for PART of the segment with %f (%f) bits from %f (%f) to %f.",
                //        (8.0 * bytes) * (complTime - (now - delta)) / (complTime - startTime), 8.0 * bytes,
                //        now - delta, startTime, complTime);
            }
        }

        if(it == rsList.begin()) {
            break;
        } else {
            --it;
        }
    }

    //dbgPrintf("In total: %f bits in %f seconds (%f Mbit/sec)", bits, seconds, bits / seconds / 1048576.0);

    if(seconds) {
        DBGMSG("Average segment throughput in [%f, %f] is %.3f Mbit/sec.", now - delta, now, bits / seconds / 1e6);
        return bits / seconds;
    } else {
        DBGMSG("Average segment throughput in [%f, %f] is 0 Mbit/sec.", now - delta, now);
        return 0;
    }
}

double Statistics::getThroughputLastRequest(const TcpConnectionId& tcpConnectionId)
{
    dp2p_assert(0 < httpRequests.count(tcpConnectionId.numeric()));
    list<int>& rsList = httpRequests.at(tcpConnectionId.numeric());
    dp2p_assert(!rsList.empty());

    const int reqId = rsList.back();
    return (8.0 * HttpRequestManager::getContentLength(reqId)) / (HttpRequestManager::getTsLastByte(reqId) - HttpRequestManager::getTsSent(reqId));
}

// TODO: assumes that requests are received in chronological order. fails if this is not the case!
// TODO: add input argument for the tcp flow or interface or something in order to handle the situation, where requests are issued in parallel.
std::vector<double> Statistics::getReceivedBytes(const TcpConnectionId& tcpConnectionId, std::vector<double> tVec)
{
    dp2p_assert(tVec.size() > 1);

    dp2p_assert(0 < httpRequests.count(tcpConnectionId.numeric()));
    list<int>& rsList = httpRequests.at(tcpConnectionId.numeric());
    dp2p_assert(!rsList.empty());

    /* Prepare the return value */
    std::vector<double> retVal(tVec.size() - 1, 0);

    /* Last stored value processed */
    double t2 = -1; // [s]
    int    b2 = -1; // [byte]

    bool done = false;

    /* Iterate over requests, beginning with the most recent */
    std::list<int>::const_iterator it = rsList.end(); --it;
    while(!done)
    {
        /* Get the most recent stored HTTP request */
        const int reqId = *it;

        /* Get the download process of the request */
        const DownloadProcess& dp = HttpRequestManager::getDownloadProcess(reqId);

        //printf("Request %u has %u vectors in the dp.\n", req->reqId, dp->size());
        //printf("tsSent=%.6f, tsFirstByte=%.6f, tsLastByte=%.6f\n", req->tsSent, req->tsFirstByte, req->tsLastByte);

        /* Iterate over the elements of the download process, beginning with the most recent */
        DownloadProcess::const_iterator dpIt = dp.end(); --dpIt;
        while(!done)
        {
            /* Get the element of the download process */
            const std::vector<DownloadProcessElement>& dpeVec = *dpIt;

            //printf("Processing vector with %u stored values in [%.6f,%.6f].\n", dpeVec.size(), dpeVec.at(0).first, dpeVec.at(dpeVec.size() - 1).first);

            /* Iterate over the vector in reverse order */
            for(int i = dpeVec.size() - 1; i >= 0 && !done; --i)
            {
                /* Get the most recent unprocessed value */
                const DownloadProcessElement& dpe = dpeVec.at(i);
                const double t1 = dpe.ts_us / 1e6;  // [s]
                const int    b1 = dpe.byte;         // [byte]

                if(t2 == -1) {
                    //printf("Processing: (%.6f, %d). First one, continue with next.\n", t1, b1);
                    t2 = t1;
                    b2 = b1;
                    continue;
                } else if(t1 == t2) {
                    //printf("Processing: (%.6f, %d). Same time as previous. Aggregating to (%.6f, %d).\n", t1, b1, t2, b2);
                    b2 += b1;
                    continue;
                }

                const double avgThrpt = (8.0 * b2) / (t2 - t1); // [bit/s]
                //printf("Processing: ([%.6f, %.6f], %d). Average throughput: %f bit/s.\n", t1, t2, b1, avgThrpt);

                /* Find the intervals to which it contributes */
                for(unsigned j = 0; j < tVec.size() - 1; ++j)
                {
                    const double T1 = tVec.at(j);
                    const double T2 = tVec.at(j + 1);
                    dp2p_assert(T1 < T2);

                    if(t2 <= T1) {                               // t1 <  t2 <= T1 <  T2
                        if(j == 0) {
                            //printf("0. Sind bereits am Vektor vorbei. Aborting.\n");
                            done = true;
                        }
                        break;
                    } else if(t1 <= T1 && T1 < t2 && t2 <= T2) { // t1 <= T1 <  t2 <= T2
                        //printf("1. Contributing %f to [%f,%f].\n", avgThrpt * (t2 - T1), T1, T2);
                        retVal.at(j) += avgThrpt * (t2 - T1);
                    } else if(T1 <= t1 && t1 < t2 && t2 <= T2) { // T1 <= t1 <  t2 <= T2
                        //printf("2. Contributing %f to [%f,%f].\n", avgThrpt * (t2 - t1), T1, T2);
                        retVal.at(j) += avgThrpt * (t2 - t1);
                    } else if(t1 <= T1 && T1 < T2 && T1 <= t2) { // t1 <= T1 <  T2 <= t2
                        //printf("3. Contributing %f to [%f,%f].\n", avgThrpt * (T2 - T1), T1, T2);
                        retVal.at(j) += avgThrpt * (T2 - T1);
                    } else if(T1 <= t1 && t1 < T2 && T2 <= t2) { // T1 <= t1 <  T2 <= t2
                        //printf("4. Contributing %f to [%f,%f].\n", avgThrpt * (T2 - t1), T1, T2);
                        retVal.at(j) += avgThrpt * (T2 - t1);
                    } else if(T1 <  T2 && T2 <= t1 && t1 < t2) { // T1 <  T2 <= t1 <  t2
                        if(j + 1 == tVec.size() - 1)
                            break;
                    } else {
                        ERRMSG("[t1,t2] = [%f,%f], [T1,T2] = [%f,%f], b1 = %f, b2 = %f.", t1, t2, T1, T2, b1, b2);
                        ERRMSG("Bug?");
                        exit(1);
                    }
                }

                t2 = t1;
                b2 = b1;
            }

            if(dpIt == dp.begin() || done) {
                break;
            } else {
                --dpIt;

            }
        }

        /* proceed with next request or break if arrived at the oldest one */
        if(it == rsList.begin() || done) {
            break;
        } else {
            --it;
        }
    }

    return retVal;
}

void Statistics::outputStatistics()
{
    char logPath[2048];

    if(logDir.empty() || httpRequests.empty())
    	return;

    for(map<int, list<int> >::const_iterator it = httpRequests.begin(); it != httpRequests.end(); ++it)
    {
    	const int tcpConnId = it->first;
    	const list<int>& reqList = it->second;

    	if(reqList.empty())
    		continue;

    	/* output seconds between downloads */
#if 0
    	sprintf(logPath, "%s/log_%020"PRId64"_TCP%05"PRId32"_sec_between_downloads.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnId);
    	FILE* fileSecBetweenDownloads = fopen(logPath, "wx");
    	dp2p_assert(fileSecBetweenDownloads);
    	std::list<HttpRequest*>::const_iterator it1 = reqList.begin();
    	std::list<HttpRequest*>::const_iterator it2 = ++reqList.begin();
    	while(it2 != reqList.end()) {
    		fprintf(fileSecBetweenDownloads, "% 17.6f % 17.6f\n", (*it1)->tsLastByte, (*it2)->tsSent - (*it1)->tsLastByte);
    		++it1;
    		++it2;
    	}
    	fclose(fileSecBetweenDownloads);
#endif

    	/* output download progress */
#if 0
    	sprintf(logPath, "%s/log_%020"PRId64"_TCP%05"PRId32"_bytes_downloaded.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnId);
    	FILE* fileDownloadProgress = fopen(logPath, "wx");
    	dp2p_assert(fileDownloadProgress);
    	int64_t byteCounter = 0;
    	for(std::list<HttpRequest*>::const_iterator it = reqList.begin(); it != reqList.end(); ++it) {
    		const HttpRequest& rs = **it;
    		for(DownloadProcess::const_iterator j = rs.downloadProcess->begin(); j != rs.downloadProcess->end(); ++j) {
    			const std::vector<DownloadProcessElement>& vec = *j;
    			for(unsigned k = 0; k < vec.size(); ++k) {
    				const double time = vec.at(k).first;
    				byteCounter += vec.at(k).second;
    				fprintf(fileDownloadProgress, "% 17.6f %17"PRIu64"\n", time, byteCounter);
    			}
    		}
    	}
    	fclose(fileDownloadProgress);
#endif

    	/* output request statistics */
    	if(logRequestStatistics)
    	{
    		sprintf(logPath, "%s/log_%020" PRId64 "_TCP%05" PRId32 "_request_statistics.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnId);
    		FILE* fileRequestStatistics = fopen(logPath, "wx");
    		dp2p_assert(fileRequestStatistics);

    		//fprintf(fileRequestStatistics, "<RequestStatistics>\n");
    		fprintf(fileRequestStatistics, " reqId.u32");
    		fprintf(fileRequestStatistics, " contentId.string");
    		fprintf(fileRequestStatistics, " hostName.string");
    		fprintf(fileRequestStatistics, " path.string");
    		fprintf(fileRequestStatistics, " tsSentOut.u64");
    		fprintf(fileRequestStatistics, " tsFirstData.u64");
    		fprintf(fileRequestStatistics, " tsLastData.u64");
    		fprintf(fileRequestStatistics, " pldSize.double");
    		fprintf(fileRequestStatistics, " maxRemainingReqs.int");
    		fprintf(fileRequestStatistics, " sentPipelined.double");
    		fprintf(fileRequestStatistics, " segmentDuration.double\n");

    		for(std::list<int>::const_iterator it = reqList.begin(); it != reqList.end(); ++it)
    		{
    			const int reqId = *it;

    			fprintf(fileRequestStatistics, " %u", reqId);
    			fprintf(fileRequestStatistics, " %s", HttpRequestManager::getContentId(reqId).toString().c_str());
    			fprintf(fileRequestStatistics, " %s", SourceManager::get(TcpConnectionManager::get(tcpConnId).srcId).hostName.c_str());
    			fprintf(fileRequestStatistics, " %s", HttpRequestManager::getFileName(reqId).c_str());
    			fprintf(fileRequestStatistics, " %" PRIu64, (int64_t)(1e6 * HttpRequestManager::getTsSent(reqId)) + dashp2p::Utilities::getReferenceTime());
    			fprintf(fileRequestStatistics, " %" PRIu64, (int64_t)(1e6 * HttpRequestManager::getTsFirstByte(reqId)) + dashp2p::Utilities::getReferenceTime());
    			fprintf(fileRequestStatistics, " %" PRIu64, (int64_t)(1e6 * HttpRequestManager::getTsLastByte(reqId)) + dashp2p::Utilities::getReferenceTime());
    			fprintf(fileRequestStatistics, " %" PRId64, HttpRequestManager::getContentLength(reqId));
    			fprintf(fileRequestStatistics, " %d", HttpRequestManager::getHdr(reqId).keepAliveMax);
    			fprintf(fileRequestStatistics, " %u", HttpRequestManager::isSentPipelined(reqId) ? 1 : 0);

    			if(mpdWrapper && HttpRequestManager::getContentType(reqId) == ContentType_Segment)
                    fprintf(fileRequestStatistics, " %.6f",  mpdWrapper->getSegmentDuration(dynamic_cast<const ContentIdSegment&>(HttpRequestManager::getContentId(reqId))) / 1e6);
                else
                    fprintf(fileRequestStatistics, " 0");

    			fprintf(fileRequestStatistics, "\n");

    			//fprintf(fileRequestStatistics, "    <Request>\n");
    			//fprintf(fileRequestStatistics, "        <reqId value=\"%u\" type=\"u32\"></reqId>\n",                             rs.reqId);
    			//fprintf(fileRequestStatistics, "        <contentId value=\"%s\" type=\"string\"></contentId>\n",                  rs.getContentId().toString().c_str());
    			//fprintf(fileRequestStatistics, "        <hostName value=\"%s\" type=\"string\"></hostName>\n",                    rs.hostName.c_str());
    			//fprintf(fileRequestStatistics, "        <path value=\"%s\" type=\"string\"></path>\n",                            rs.file.c_str());
    			//fprintf(fileRequestStatistics, "        <tsSentOut value=\"%"PRIu64"\" type=\"u64\"></tsSentOut>\n",              (int64_t)(1e6 * rs.tsSent) + dashp2p::Utilities::getReferenceTime());
    			//fprintf(fileRequestStatistics, "        <tsFirstData value=\"%"PRIu64"\" type=\"u64\"></tsFirstData>\n",          (int64_t)(1e6 * rs.tsFirstByte) + dashp2p::Utilities::getReferenceTime());
    			//fprintf(fileRequestStatistics, "        <tsLastData value=\"%"PRIu64"\" type=\"double\"></tsLastData>\n",         (int64_t)(1e6 * rs.tsLastByte) + dashp2p::Utilities::getReferenceTime());
    			//fprintf(fileRequestStatistics, "        <pldSize value=\"%"PRId64"\" type=\"double\"></pldSize>\n",               rs.hdr.contentLength);
    			//fprintf(fileRequestStatistics, "        <maxRemainingReqs value=\"%d\" type=\"int\"></maxRemainingReqs>\n",       rs.hdr.keepAliveMax);
    			//fprintf(fileRequestStatistics, "        <sentPipelined value=\"%u\" type=\"double\"></sentPipelined>\n",          rs.sentPipelined ? 1 : 0);
    			//if(mpdWrapper && rs.getContentType() == ContentType_Segment)
    			//  fprintf(fileRequestStatistics, "        <segmentDuration value=\"%.6f\" type=\"double\"></segmentDuration>\n",
    			//          mpdWrapper->getSegmentDuration(dynamic_cast<const ContentIdSegment&>(rs.getContentId())) / 1e6);
    			//fprintf(fileRequestStatistics, "    </Request>\n");
    		}
    		//fprintf(fileRequestStatistics, "</RequestStatistics>\n");
    		fclose(fileRequestStatistics);
    	}

    	/* download processes of the requests */
    	if(logRequestDownloadProgress)
    	{
    		sprintf(logPath, "%s/log_%020" PRId64 "_TCP%05" PRId32 "_request_statistics_download_processes", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnId);
    		{
    		    int ret = mkdir(logPath, 0700);
    		    if(ret != 0) {
    		        perror("mkdir()");
    		        ERRMSG("mkdir could not create: %s.", logPath);
    		        dp2p_assert(0);
    		    }
    		}
    		for(std::list<int>::const_iterator it = reqList.begin(); it != reqList.end(); ++it)
    		{
    			const int reqId = *it;
    			sprintf(logPath, "%s/log_%020" PRId64 "_TCP%05" PRId32 "_request_statistics_download_processes/reqId_%06u.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnId, reqId);
    			FILE* dpFile = fopen(logPath, "wx");
    			dp2p_assert(dpFile);
    			for(DownloadProcess::const_iterator j = HttpRequestManager::getDownloadProcess(reqId).begin(); j != HttpRequestManager::getDownloadProcess(reqId).end(); ++j)
    			{
    				const vector<DownloadProcessElement>& vec = *j;
    				for(unsigned k = 0; k < vec.size(); ++k) {
    					fprintf(dpFile, "%" PRId64 " %d\n", vec.at(k).ts_us, vec.at(k).byte);
    				}
    			}
    			dp2p_assert(0 == fclose(dpFile));
    		}
    	}

    	// TODO: the code below does not work if requests are issued in parallel. the function getReceivedBytes must be adapted (see comment above this latter function)
#if 0
    	/* discretized download process */
    	//printf("oldest stored value: %f, now: %f\n", rsList.front()->tsFirstByte, dashp2p::Utilities::now());
    	const double firstByteTimeFloor = std::floor(rsList.front()->tsFirstByte);
    	std::vector<double> tVec((unsigned)(std::ceil(dashp2p::Utilities::now()) - firstByteTimeFloor));
    	for(unsigned i = 0; i < tVec.size(); ++i)
    		tVec.at(i) = firstByteTimeFloor + i;
    	std::vector<double> discDp = getReceivedBytes(tVec);
    	sprintf(logPath, "%s/log_%020"PRId64"_download_process_discr.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
    	FILE* f = fopen(logPath, "wx");
    	dp2p_assert(f);
    	for(unsigned i = 0; i < discDp.size(); ++i)
    		fprintf(f, "% 17.6f  % 7.1f\n", tVec.at(i), discDp.at(i));
    	fprintf(f, "% 17.6f  % 7.1f\n", tVec.at(discDp.size()), -1.0);
    	dp2p_assert(0 == fclose(f));
#endif
    }
}

void Statistics::recordAdaptationDecision(double relTime, int64_t beta, double rho, double rhoLast, int r_last, int r_new, int64_t Bdelay, bool betaMinIncreasing, int reason)
{
    if(logDir.empty() || !logAdaptationDecision)
        return;

    if(!fileAdaptationDecision)
    	prepareFileAdaptationDecision();

    //fprintf(fileAdaptationDecision, "% 17.6f s | % 17.6f s | % 17.6f Mbps | % 17.6f Mbps | %9d bps | %9d bps | % 3.6f s | % 4d\n",
    //        relTime, beta / 1e6, rho / 1e6, rhoLast / 1e6, r_last, r_new, Bdelay / 1e6, reason);

    fprintf(fileAdaptationDecision, "% 11.6f s | %9d bps -> %9d bps | % 11.6f s | %3d | % 11.6f s | % 10.6f Mbps | % 10.6f Mbps | %s\n",
            relTime, r_last, r_new, std::min<double>(9999.0, Bdelay / 1e6), reason, beta / 1e6, rho / 1e6, rhoLast / 1e6, betaMinIncreasing ? "incr" : "decr");
}

void Statistics::recordGiveDataToVlc(int64_t relTime, int64_t usec, int64_t bytes)
{
    if(logDir.empty() || !logGiveDataToVlc)
        return;

    if(!fileGiveDataToVlc)
    	prepareFileGiveDataToVlc();

    fprintf(fileGiveDataToVlc, "% 17.6f % 17.6f % 17.6f\n", relTime / 1e6, usec / 1e6, bytes / 1e6);
}

void Statistics::recordScalarString(const char* name, const char* value)
{
    if(logDir.empty() || !logScalarValues)
        return;

    if(!fileScalarValues)
    	prepareFileScalarValues();

    fprintf(fileScalarValues, "    <%s value=\"%s\" type=\"string\"></%s>\n", name, value, name);
}

void Statistics::recordScalarDouble(const char* name, double value)
{
    if(logDir.empty() || !logScalarValues)
        return;

    if(!fileScalarValues)
        	prepareFileScalarValues();

    fprintf(fileScalarValues, "    <%s value=\"%17.6f\" type=\"double\"></%s>\n", name, value, name);
}

void Statistics::recordScalarU64(const char* name, uint64_t value)
{
    if(logDir.empty() || !logScalarValues)
        return;

    if(!fileScalarValues)
        	prepareFileScalarValues();

    fprintf(fileScalarValues, "    <%s value=\"%" PRIu64 "\" type=\"u64\"></%s>\n", name, value, name);
}

#if 0
void Statistics::recordTcpState(const char* reason, const struct tcp_info& tcpInfo)
{
    if(logDir.empty())
        return;

    if(0 == filesTcpState.count(0))
    	prepareFileTcpState(0);

    static struct tcp_info last_tcpInfo;
    static unsigned cnt = 1;

    /* Only log if something changed. */
    if(tcpInfo != last_tcpInfo)
    {
        /*fprintf(fileTcpState, "% 17.6f | %15s | %17s | %17s | %11u | %6u | %7u | %7u | %10u | %10u | %7u | %7u | %7u | %7u | %7u | %6u | %6u | %7u | %7u | %14u | %13u | %14u | %13u | %6u | %12u | %7u | %6u | %12u | %8u | %6u | %10u | %7u | %9u | %13u |\n",
                dashp2p::Utilities::getTime() / 1e6, reason,
                tcpState2String(tcpInfo.tcpi_state), tcpCAState2String(tcpInfo.tcpi_ca_state),
                tcpInfo.tcpi_retransmits, tcpInfo.tcpi_probes, tcpInfo.tcpi_backoff, tcpInfo.tcpi_options,
                tcpInfo.tcpi_snd_wscale, tcpInfo.tcpi_rcv_wscale,
                tcpInfo.tcpi_rto, tcpInfo.tcpi_ato, tcpInfo.tcpi_snd_mss, tcpInfo.tcpi_rcv_mss,
                tcpInfo.tcpi_unacked, tcpInfo.tcpi_sacked, tcpInfo.tcpi_lost, tcpInfo.tcpi_retrans, tcpInfo.tcpi_fackets,
                tcpInfo.tcpi_last_data_sent, tcpInfo.tcpi_last_ack_sent, tcpInfo.tcpi_last_data_recv, tcpInfo.tcpi_last_ack_recv,
                tcpInfo.tcpi_pmtu, tcpInfo.tcpi_rcv_ssthresh, tcpInfo.tcpi_rtt, tcpInfo.tcpi_rttvar, tcpInfo.tcpi_snd_ssthresh,
                tcpInfo.tcpi_snd_cwnd, tcpInfo.tcpi_advmss, tcpInfo.tcpi_reordering, tcpInfo.tcpi_rcv_rtt,
                tcpInfo.tcpi_rcv_space, tcpInfo.tcpi_total_retrans);*/
        fprintf(filesTcpState.at(0), "tcpState.time(%5u) = % 17.6f;     tcpState.reason{%5u} = '%15s';     tcpState.TCP_STATE{%5u} = '%17s';     tcpState.TCP_CA_STATE{%5u} = '%17s';     tcpState.retransmits(%5u) = %10u;     tcpState.probes(%5u) = %6u;     tcpState.backoff(%5u) = %7u;     tcpState.options(%5u) = %7u;     tcpState.snd_wscale(%5u) = %10u;     tcpState.rcv_wscale(%5u) = %10u;     tcpState.rto(%5u) = % 9.6f;     tcpState.ato(%5u) = %7u;     tcpState.snd_mss(%5u) = %7u;     tcpState.rcv_mss(%5u) = %7u;     tcpState.unacked(%5u) = %7u;     tcpState.sacked(%5u) = %6u;     tcpState.lost(%5u) = %6u;     tcpState.retrans(%5u) = %7u;     tcpState.fackets(%5u) = %7u;     tcpState.last_data_sent(%5u) = %14u;     tcpState.last_ack_sent(%5u) = %13u;     tcpState.last_data_recv(%5u) = %14u;     tcpState.last_ack_recv(%5u) = %13u;     tcpState.pmtu(%5u) = %6u;     tcpState.rcv_ssthresh(%5u) = %12u;     tcpState.rtt(%5u) = % 9.6f;     tcpState.rttvar(%5u) = % 9.6f;     tcpState.snd_ssthresh(%5u) = %12u;     tcpState.snd_cwnd(%5u) = %8u;     tcpState.advmss(%5u) = %6u;     tcpState.reordering(%5u) = %10u;     tcpState.rcv_rtt(%5u) = %7u;     tcpState.rcv_space(%5u) = %9u;     tcpState.total_retrans(%5u) = %13u;\n",
                cnt, dashp2p::Utilities::getTime() / 1e6,
                cnt, reason,
                cnt, tcpState2String(tcpInfo.tcpi_state).c_str(),
                cnt, tcpCAState2String(tcpInfo.tcpi_ca_state).c_str(),
                cnt, tcpInfo.tcpi_retransmits,
                cnt, tcpInfo.tcpi_probes,
                cnt, tcpInfo.tcpi_backoff,
                cnt, tcpInfo.tcpi_options,
                cnt, tcpInfo.tcpi_snd_wscale,
                cnt, tcpInfo.tcpi_rcv_wscale,
                cnt, tcpInfo.tcpi_rto / 1e6,
                cnt, tcpInfo.tcpi_ato,
                cnt, tcpInfo.tcpi_snd_mss,
                cnt, tcpInfo.tcpi_rcv_mss,
                cnt, tcpInfo.tcpi_unacked,
                cnt, tcpInfo.tcpi_sacked,
                cnt, tcpInfo.tcpi_lost,
                cnt, tcpInfo.tcpi_retrans,
                cnt, tcpInfo.tcpi_fackets,
                cnt, tcpInfo.tcpi_last_data_sent,
                cnt, tcpInfo.tcpi_last_ack_sent,
                cnt, tcpInfo.tcpi_last_data_recv,
                cnt, tcpInfo.tcpi_last_ack_recv,
                cnt, tcpInfo.tcpi_pmtu,
                cnt, tcpInfo.tcpi_rcv_ssthresh,
                cnt, tcpInfo.tcpi_rtt / 1e6,
                cnt, tcpInfo.tcpi_rttvar / 1e6,
                cnt, tcpInfo.tcpi_snd_ssthresh,
                cnt, tcpInfo.tcpi_snd_cwnd,
                cnt, tcpInfo.tcpi_advmss,
                cnt, tcpInfo.tcpi_reordering,
                cnt, tcpInfo.tcpi_rcv_rtt,
                cnt, tcpInfo.tcpi_rcv_space,
                cnt, tcpInfo.tcpi_total_retrans);
        last_tcpInfo = tcpInfo;
        ++cnt;
    }
}
#endif

void Statistics::recordTcpState(const TcpConnectionId& tcpConnectionId, const char* reason, const struct tcp_info& tcpInfo)
{
	static map<int, struct tcp_info> lastTcpInfo;

	if(logDir.empty() || !logTcpState)
		return;

	if(0 == filesTcpState.count(tcpConnectionId.numeric()))
		prepareFileTcpState(tcpConnectionId);

	if(tcpInfo != lastTcpInfo[tcpConnectionId.numeric()])
	{
		fprintf(filesTcpState.at(tcpConnectionId.numeric()),
				"%17" PRId64 " %17s %19s %19s %13u %8u %9u %9u %12u %12u % 11.6f %9u %9u %9u %9u %8u %8u %9u %9u %16u %15u %16u %15u %8u %14u % 11.6f % 10.6f %14u %10u %8u %12u %9u %11u %15u\n",
				dashp2p::Utilities::getAbsTime(),
				reason,
				TcpConnectionManager::tcpState2String(tcpInfo.tcpi_state).c_str(),
				TcpConnectionManager::tcpCAState2String(tcpInfo.tcpi_ca_state).c_str(),
				tcpInfo.tcpi_retransmits,
				tcpInfo.tcpi_probes,
				tcpInfo.tcpi_backoff,
				tcpInfo.tcpi_options,
				tcpInfo.tcpi_snd_wscale,
				tcpInfo.tcpi_rcv_wscale,
				tcpInfo.tcpi_rto / 1e6,
				tcpInfo.tcpi_ato,
				tcpInfo.tcpi_snd_mss,
				tcpInfo.tcpi_rcv_mss,
				tcpInfo.tcpi_unacked,
				tcpInfo.tcpi_sacked,
				tcpInfo.tcpi_lost,
				tcpInfo.tcpi_retrans,
				tcpInfo.tcpi_fackets,
				tcpInfo.tcpi_last_data_sent,
				tcpInfo.tcpi_last_ack_sent,
				tcpInfo.tcpi_last_data_recv,
				tcpInfo.tcpi_last_ack_recv,
				tcpInfo.tcpi_pmtu,
				tcpInfo.tcpi_rcv_ssthresh,
				tcpInfo.tcpi_rtt / 1e6,
				tcpInfo.tcpi_rttvar / 1e6,
				tcpInfo.tcpi_snd_ssthresh,
				tcpInfo.tcpi_snd_cwnd,
				tcpInfo.tcpi_advmss,
				tcpInfo.tcpi_reordering,
				tcpInfo.tcpi_rcv_rtt,
				tcpInfo.tcpi_rcv_space,
				tcpInfo.tcpi_total_retrans);
		lastTcpInfo[tcpConnectionId.numeric()] = tcpInfo;
	}
}

void Statistics::recordBytesStored(int64_t time, int64_t bytes)
{
    if(logDir.empty() || !logBytesStored)
        return;

    if(!fileBytesStored)
    	prepareFileBytesStored();

    fprintf(fileBytesStored, "% 18" PRId64 " % 10" PRId64 "\n", time, bytes);
}

void Statistics::recordUsecStored(int64_t time, int64_t usec)
{
    if(logDir.empty() || !logSecStored)
        return;

    if(!fileSecStored)
    	prepareFileSecStored();

    fprintf(fileSecStored, "% 18" PRId64 " % 10" PRId64 "\n", time, usec);
}

void Statistics::recordUnderrun(int64_t begin, int64_t duration)
{
    if(logDir.empty() || !logUnderruns)
        return;

    if(!fileUnderruns)
    	prepareFileUnderruns();

    fprintf(fileUnderruns, "% 17.6f % 17.6f\n", begin / 1e6, duration / 1e6);
}

void Statistics::recordReconnect(int64_t time, enum ReconnectReason reconnectReason)
{
	if(logDir.empty() || !logReconnects)
		return;

	if(!fileReconnects)
		prepareFileReconnects();

	fprintf(fileReconnects, "% 17.6f %d\n", time / 1e6, reconnectReason);
}

void Statistics::recordSegmentSize(ContentIdSegment segId, int64_t bytes)
{
    if(logDir.empty() || !logSegmentSizes)
        return;

    if(!fileSegmentSizes)
    	prepareFileSegmentSizes();

    fprintf(fileSegmentSizes, "%d %d %" PRId64 "\n", segId.bitRate(), segId.segmentIndex(), bytes);
}

void Statistics::recordP2PMeasurementToFile(string filePath, int segNr, int repId,
		int sourceNNumber, double measuredBandwith , int mode, double actualFetchtime)
{
	std::ofstream myfile;
	char fileName[1024];
	sprintf(fileName, "log_%020" PRId64 "_P2PMeasurement.txt", dashp2p::Utilities::getReferenceTime());
	myfile.open ((filePath + fileName).c_str(), std::fstream::in | std::fstream::out | std::fstream::app);
	myfile << dashp2p::Utilities::getReferenceTime() << " " << dashp2p::Utilities::now() << " " << (int) segNr << " " << repId << " " << sourceNNumber << " " <<
			measuredBandwith << " " << mode  << " " <<  actualFetchtime  << "\n";
	myfile.close();
}

void Statistics::recordP2PBufferlevelToFile(string filePath, int64_t availableContigIntervalTime , int64_t availableContigIntervalBytes)
{
	std::ofstream myfile;
	char fileName[1024];
	sprintf(fileName, "log_%020" PRId64 "_P2PMeasurementBufferlevel.txt", dashp2p::Utilities::getReferenceTime());
	myfile.open ((filePath + fileName).c_str(), std::fstream::in | std::fstream::out | std::fstream::app);
	myfile << dashp2p::Utilities::getReferenceTime() << " " << dashp2p::Utilities::now() << " " << availableContigIntervalBytes << " " << availableContigIntervalTime << "\n";
	myfile.close();
}

bool operator==(const struct tcp_info& x, const struct tcp_info& y)
{
    if(        x.tcpi_advmss         == y.tcpi_advmss
            && x.tcpi_ato            == y.tcpi_ato
            && x.tcpi_backoff        == y.tcpi_backoff
            && x.tcpi_ca_state       == y.tcpi_ca_state
            && x.tcpi_fackets        == y.tcpi_fackets
            && x.tcpi_last_ack_recv  == y.tcpi_last_ack_recv
            && x.tcpi_last_ack_sent  == y.tcpi_last_ack_sent
            && x.tcpi_last_data_recv == y.tcpi_last_data_recv
            && x.tcpi_last_data_sent == y.tcpi_last_data_sent
            && x.tcpi_lost           == y.tcpi_lost
            && x.tcpi_options        == y.tcpi_options
            && x.tcpi_pmtu           == y.tcpi_pmtu
            && x.tcpi_probes         == y.tcpi_probes
            && x.tcpi_rcv_mss        == y.tcpi_rcv_mss
            && x.tcpi_rcv_rtt        == y.tcpi_rcv_rtt
            && x.tcpi_rcv_space      == y.tcpi_rcv_space
            && x.tcpi_rcv_ssthresh   == y.tcpi_rcv_ssthresh
            && x.tcpi_rcv_wscale     == y.tcpi_rcv_wscale
            && x.tcpi_reordering     == y.tcpi_reordering
            && x.tcpi_retrans        == y.tcpi_retrans
            && x.tcpi_retransmits    == y.tcpi_retransmits
            && x.tcpi_rto            == y.tcpi_rto
            && x.tcpi_rtt            == y.tcpi_rtt
            && x.tcpi_rttvar         == y.tcpi_rttvar
            && x.tcpi_sacked         == y.tcpi_sacked
            && x.tcpi_snd_cwnd       == y.tcpi_snd_cwnd
            && x.tcpi_snd_mss        == y.tcpi_snd_mss
            && x.tcpi_snd_ssthresh   == y.tcpi_snd_ssthresh
            && x.tcpi_snd_wscale     == y.tcpi_snd_wscale
            && x.tcpi_state          == y.tcpi_state
            && x.tcpi_total_retrans  == y.tcpi_total_retrans
            && x.tcpi_unacked        == y.tcpi_unacked ) {
        return true;
    } else {
        return false;
    }
}

bool operator!=(const struct tcp_info& x, const struct tcp_info& y)
{
    return !(x == y);
}

void Statistics::prepareFileScalarValues()
{
	dp2p_assert(!fileScalarValues);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_scalar_values.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileScalarValues = fopen(logPath, "wx");
	dp2p_assert(fileScalarValues);
	fprintf(fileScalarValues, "<ScalarValues>\n");
}

void Statistics::prepareFileAdaptationDecision()
{
	dp2p_assert(!fileAdaptationDecision);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_adaptation_decision.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileAdaptationDecision = fopen(logPath, "wx");
	dp2p_assert(fileAdaptationDecision);
	//fprintf(fileAdaptationDecision, "| time | beta | rho | rho_last | r_last | r_new | Bdelay | reason |\n");
	fprintf(fileAdaptationDecision, "| time | r_last -> r_new | Bdelay | reason | beta | rho | rhoLast |\n");
}

void Statistics::prepareFileGiveDataToVlc()
{
	dp2p_assert(!fileGiveDataToVlc);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_data_consumed_by_vlc.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileGiveDataToVlc = fopen(logPath, "wx");
	dp2p_assert(fileGiveDataToVlc);
}

void Statistics::prepareFileTcpState(const TcpConnectionId& tcpConnectionId)
{
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_TCP%05" PRId32 "_state.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime(), tcpConnectionId.numeric());
	FILE* f = fopen(logPath, "wx");
	dp2p_assert(f);
	//if(tcpConnId == 0) {
	//	fprintf(f, "tcpState = struct('time', cell(1), 'reason', cell(1), 'TCP_STATE', cell(1), 'TCP_CA_STATE', cell(1), 'retransmits', cell(1), 'probes', cell(1), 'backoff', cell(1), 'options', cell(1), 'snd_wscale', cell(1), 'rcv_wscale', cell(1), 'rto', cell(1), 'ato', cell(1), 'snd_mss', cell(1), 'rcv_mss', cell(1), 'unacked', cell(1), 'sacked', cell(1), 'lost', cell(1), 'retrans', cell(1), 'fackets', cell(1), 'last_data_sent', cell(1), 'last_ack_sent', cell(1), 'last_data_recv', cell(1), 'last_ack_recv', cell(1), 'pmtu', cell(1), 'rcv_ssthresh', cell(1), 'rtt', cell(1), 'rttvar', cell(1), 'snd_ssthresh', cell(1), 'snd_cwnd', cell(1), 'advmss', cell(1), 'reordering', cell(1), 'rcv_rtt', cell(1), 'rcv_space', cell(1), 'total_retrans', cell(1));\n");
	//	fprintf(f, "tcpState.reason = {}; tcpState.TCP_STATE = {}; tcpState.TCP_CA_STATE = {};\n");
	//} else {
		fprintf(f, "      Time        |     Reason      |     TCP_STATE     |    TCP_CA_STATE   | retransmits | probes | backoff | options | snd_wscale | rcv_wscale |    rto    |   ato   | snd_mss | rcv_mss | unacked | sacked |  lost  | retrans | fackets | last_data_sent | last_ack_sent | last_data_recv | last_ack_recv |  pmtu  | rcv_ssthresh |    rtt    |  rttvar  | snd_ssthresh | snd_cwnd | advmss | reordering | rcv_rtt | rcv_space | total_retrans |\n");
		fprintf(f, "------------------|-----------------|-------------------|-------------------|-------------|--------|---------|---------|------------|------------|-----------|---------|---------|---------|---------|--------|--------|---------|---------|----------------|---------------|----------------|---------------|--------|--------------|-----------|----------|--------------|----------|--------|------------|---------|-----------|---------------|\n");
	//}
	dp2p_assert(filesTcpState.insert(pair<int, FILE*>(tcpConnectionId.numeric(), f)).second);
}

void Statistics::prepareFileBytesStored()
{
	dp2p_assert(!fileBytesStored);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_bytes_stored.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileBytesStored = fopen(logPath, "wx");
	dp2p_assert(fileBytesStored);
}

void Statistics::prepareFileSecStored()
{
	dp2p_assert(!fileSecStored);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_sec_stored.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileSecStored = fopen(logPath, "wx");
	dp2p_assert(fileSecStored);
}

#if 0
void Statistics::prepareFileSecDownloaded()
{
	dp2p_assert(!fileSecDownloaded);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_sec_downloaded.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileSecDownloaded = fopen(logPath, "wx");
	dp2p_assert(fileSecDownloaded);
}
#endif

void Statistics::prepareFileUnderruns()
{
	dp2p_assert(!fileUnderruns);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_underruns.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileUnderruns = fopen(logPath, "wx");
	dp2p_assert(fileUnderruns);
}

void Statistics::prepareFileReconnects()
{
	dp2p_assert(!fileReconnects);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_reconnects.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileReconnects = fopen(logPath, "wx");
	dp2p_assert(fileReconnects);
}

void Statistics::prepareFileSegmentSizes()
{
	dp2p_assert(fileSegmentSizes);
	char logPath[1024];
	sprintf(logPath, "%s/log_%020" PRId64 "_segment_sizes.txt", logDir.c_str(), dashp2p::Utilities::getReferenceTime());
	fileSegmentSizes = fopen(logPath, "wx");
	dp2p_assert(fileSegmentSizes);
}

}

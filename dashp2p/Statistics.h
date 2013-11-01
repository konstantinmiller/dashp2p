/****************************************************************************
 * Statistics.h                                                             *
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

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "dashp2p.h"
#include "Utilities.h"
#include "ContentId.h"
#include "MpdWrapper.h"
#include "TcpConnectionManager.h"
#include <list>
#include <vector>
#include <map>
#include <netinet/tcp.h>

using std::vector;
using std::map;
using std::list;

namespace dashp2p {

bool operator==(const struct tcp_info& x, const struct tcp_info& y);
bool operator!=(const struct tcp_info& x, const struct tcp_info& y);

class Statistics
{
private:
    Statistics(){}
    virtual ~Statistics(){}

/* Public methods */
public:
    static void init(const std::string& logDir, const bool logTcpState, const bool logScalarValues, const bool logAdaptationDecision,
    		const bool logGiveDataToVlc, const bool logBytesStored, const bool logSecStored, const bool logUnderruns,
    		const bool logReconnects, const bool logSegmentSizes, const bool logRequestStatistics,
    		const bool logRequestDownloadProgress);
    static void cleanUp();

    static string getLogDir() {return logDir;}

    static void setMpdWrapper(const MpdWrapper* mpdWrapper) {Statistics::mpdWrapper = mpdWrapper;}

    /* TCP connections */
    static int32_t registerTcpConnection();
    static void unregisterTcpConnection(const int32_t tcpConnId);

    static void recordRequestStatistics(const TcpConnectionId& tcpConnectionId, int reqId);
    static int numCompletedRequests(const TcpConnectionId& tcpConnectionId);
    static int getLastRequest(const TcpConnectionId& tcpConnectionId);
    /* Note: Does not consider the last segment. So, the result is only valid if called directly after a request is completed
     * and the request data were passed to the Statistics module. */
    static double getThroughput(const TcpConnectionId& tcpConnectionId, double delta, string devName = "");
    static double getThroughputLastRequest(const TcpConnectionId& tcpConnectionId); // [bit/s]
    static std::vector<double> getReceivedBytes(const TcpConnectionId& tcpConnectionId, std::vector<double> tVec);
    static void outputStatistics();

    static void recordAdaptationDecision(double relTime, int64_t beta, double rho, double rhoLast, int r_last, int r_new, int64_t Bdelay, bool betaMinIncreasing, int reason);

    static void recordGiveDataToVlc(int64_t relTime, int64_t usec, int64_t bytes);

    /**
     * Record values to the scalar output file.
     */
    static void recordScalarString(const char* name, const char* value);
    static void recordScalarDouble(const char* name, double value);
    static void recordScalarU64(const char* name, uint64_t value);

    //static void recordTcpState(const char* reason, const struct tcp_info& tcpInfo);
    static void recordTcpState(const TcpConnectionId& tcpConnectionId, const char* reason, const struct tcp_info& tcpInfo);
    static void recordBytesStored(int64_t time, int64_t bytes);
    static void recordUsecStored(int64_t time, int64_t usec);
    static void recordUnderrun(int64_t begin, int64_t duration);
    static void recordReconnect(int64_t time, enum ReconnectReason reconnectReason);

    static void recordSegmentSize(ContentIdSegment segId, int64_t bytes);

    static void recordP2PMeasurementToFile(string filePath, int segNr, int repId, int sourceNNumber,
    			double measuredBandwith , int mode, double actualFetchtime);
    static void recordP2PBufferlevelToFile(string filePath,
    			int64_t availableContigIntervalTime , int64_t availableContigIntervalBytes);

/* Private methods */
private:
    static void prepareFileScalarValues();
    static void prepareFileAdaptationDecision();
    static void prepareFileGiveDataToVlc();
    static void prepareFileTcpState(const TcpConnectionId& tcpConnectionId);
    static void prepareFileBytesStored();
    static void prepareFileSecStored();
    //static void prepareFileSecDownloaded();
    static void prepareFileUnderruns();
    static void prepareFileReconnects();
    static void prepareFileSegmentSizes();

/* Private fields */
private:
    static const MpdWrapper* mpdWrapper;
    static std::string logDir;
    //static std::list<HttpRequest*> rsList;
    static map<int, list<int> > httpRequests;
    //static TcpConnectionId lastTcpConnectionId;
    static bool  logTcpState;
    static map<int, FILE*> filesTcpState;
    static bool  logScalarValues;
    static FILE* fileScalarValues;
    static bool  logAdaptationDecision;
    static FILE* fileAdaptationDecision;
    static bool  logGiveDataToVlc;
    static FILE* fileGiveDataToVlc;
    static bool  logBytesStored;
    static FILE* fileBytesStored;
    static bool  logSecStored;
    static FILE* fileSecStored;
    //static FILE* fileSecDownloaded;
    static bool  logUnderruns;
    static FILE* fileUnderruns;
    static bool  logReconnects;
    static FILE* fileReconnects;
    static bool  logSegmentSizes;
    static FILE* fileSegmentSizes;
    static bool  logRequestStatistics;
    static bool  logRequestDownloadProgress;
};

}

#endif /* STATISTICS_H_ */

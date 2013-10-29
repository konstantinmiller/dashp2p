/****************************************************************************
 * Dashp2p.cpp                                                              *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jan 31, 2013                                                 *
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

#if !defined DP2P_VLC || DP2P_VLC == 0

#include "Dashp2p.h"
#include "DebugAdapter.h"

#include <libxml/xmlreader.h>

using std::string;


namespace dp2p {

void Dashp2p::initDebugging(DebuggingLevel dl, const char* logFileName)
{
	DebugAdapter::init(dl, logFileName);
}

void Dashp2p::initXml()
{
	xmlInitParser();
}

void Dashp2p::initStatistics(const std::string& logDir, const bool logTcpState, const bool logScalarValues, const bool logAdaptationDecision,
		const bool logGiveDataToVlc, const bool logBytesStored, const bool logSecStored, const bool logUnderruns,
		const bool logReconnects, const bool logSegmentSizes, const bool logRequestStatistics,
		const bool logRequestDownloadProgress)
{
	Statistics::init(logDir, logTcpState, logScalarValues, logAdaptationDecision, logGiveDataToVlc,
			logBytesStored, logSecStored, logUnderruns, logReconnects, logSegmentSizes, logRequestStatistics,
			logRequestDownloadProgress);
}

void Dashp2p::setReferenceTime()
{
	dashp2p::Utilities::setReferenceTime();
}

void Dashp2p::cleanup()
{
	Statistics::cleanUp();
	xmlCleanupParser();
	DebugAdapter::cleanUp();
}

void Dashp2p::cleanupDebugging()
{
	DebugAdapter::cleanUp();
}

void Dashp2p::setDebugFile(const char* dbgFileName)
{
	DebugAdapter::setDebugFile(dbgFileName);
}

} /* namespace dp2p */

#endif // #if !defined DP2P_VLC || DP2P_VLC == 0

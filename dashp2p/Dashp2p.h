/****************************************************************************
 * Dashp2p.h                                                                *
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

#ifndef DASHP2P_H_
#define DASHP2P_H_

#if !defined DP2P_VLC || DP2P_VLC == 0

#include <string>

#include "Utilities.h"
#include "Statistics.h"
#include "ContentIdGeneric.h"

namespace dp2p {

class Dashp2p
{
public:
	Dashp2p();
	virtual ~Dashp2p();

	static void initDebugging(DebuggingLevel dl, const char* logFileName);
	static void initXml();
	static void initStatistics(const std::string& logDir, const bool logTcpState, const bool logScalarValues, const bool logAdaptationDecision,
			const bool logGiveDataToVlc, const bool logBytesStored, const bool logSecStored, const bool logUnderruns,
			const bool logReconnects, const bool logSegmentSizes, const bool logRequestStatistics,
			const bool logRequestDownloadProgress);
	static void setReferenceTime();

	static void cleanup();
	static void cleanupDebugging();

	static void setDebugFile(const char* dbgFileName);
};

} /* namespace dp2p */

#endif /* if !defined DP2P_VLC || DP2P_VLC == 0 */

#endif /* DASHP2P_H_ */

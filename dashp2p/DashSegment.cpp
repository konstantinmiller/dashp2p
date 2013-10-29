/****************************************************************************
 * DashSegment.cpp                                                          *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 20, 2012                                                 *
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


//#include "Dashp2pTypes.h"
#include "DashSegment.h"
#include "DebugAdapter.h"
//#include <inttypes.h>

namespace dashp2p {

DashSegment::DashSegment(const ContentIdSegment& segId, int64_t numBytes, int64_t duration)
  : segId(segId),
    dataField(numBytes),
    duration(duration)
{
}

void DashSegment::setData(int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite)
{
    DBGMSG("Adding data to (RepId %d, SegNr %d) at [%" PRId64 ", %" PRId64 "] with%s overwriting.", segId.bitRate(), segId.bitRate(), byteFrom, byteTo, overwrite?" potential":"out");
    dataField.setData(byteFrom, byteTo, srcBuffer, overwrite);
    //DBGMSG("Return.");
}

pair<int64_t, int64_t> DashSegment::getData(int64_t offset, char* buffer, int bufferSize) const
{
    const int64_t numCopiedBytes = dataField.getData(offset, buffer, bufferSize);
    const int64_t numCopiedUsecs = (numCopiedBytes * getTotalDuration()) / getTotalSize();
    return pair<int64_t, int64_t>(numCopiedUsecs, numCopiedBytes);
}

int64_t DashSegment::getTotalSize() const
{
    return dataField.getReservedSize();
}

pair<int64_t, int64_t> DashSegment::getContigInterval(int64_t offset) const
{
    pair<int64_t, int64_t> ret;
    ret.second = dataField.getContigInterval(offset);
    ret.first = (ret.second * getTotalDuration()) / getTotalSize();
    return ret;
}

void DashSegment::toFile(string& fileName) const
{
	dataField.toFile(fileName);
}

}

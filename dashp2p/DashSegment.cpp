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

DashObject::DashObject(const ContentId& contentId, int64_t numBytes)
  : contentId(*contentId.copy()),
    dataField(nullptr)
{
    if(numBytes > 0)
        dataField = new DataField(numBytes);
}

DashObject::~DashObject()
{
    delete &contentId;
    delete dataField;
}

void DashObject::setSize(int64_t s)
{
    if(dataField == nullptr)
        dataField = new DataField(s);
    else
        dp2p_assert(dataField->getReservedSize() == s);
}

void DashObject::setData(int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite)
{
    DBGMSG("Adding data to %s at [%" PRId64 ", %" PRId64 "] with%s overwriting.",
            contentId.toString().c_str(), byteFrom, byteTo, overwrite?" potential":"out");
    dataField->setData(byteFrom, byteTo, srcBuffer, overwrite);
    //DBGMSG("Done.");
}

int64_t DashObject::getData(int64_t offset, char* buffer, int bufferSize)
{
    //const int64_t numCopiedBytes = dataField->getData(offset, buffer, bufferSize);
    //const int64_t numCopiedUsecs = (numCopiedBytes * duration) / getTotalSize();
    //return pair<int64_t, int64_t>(numCopiedUsecs, numCopiedBytes);
    return dataField->getData(offset, buffer, bufferSize);
}

int64_t DashObject::getTotalSize() const
{
    return dataField->getReservedSize();
}

void DashObject::toFile(string& fileName)
{
	dataField->toFile(fileName);
}

pair<int64_t, int64_t> DashSegment::getContigInterval(int64_t offset)
{
    pair<int64_t, int64_t> ret;
    ret.second = dataField->getContigInterval(offset);
    ret.first = (ret.second * duration) / getTotalSize();
    return ret;
}

}

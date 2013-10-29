/****************************************************************************
 * DataField.cpp                                                            *
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


#include "Dashp2pTypes.h"
#include "DataField.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <list>
using std::list;
using std::pair;


DataField::DataField(int64_t numBytes)
  : p(new char[numBytes]),
    reservedSize(numBytes),
    occupiedSize(0),
    dataMap()
{
    dp2p_assert(p);
}

DataField::~DataField()
{
    delete [] p;
    //int64_t reservedSize;
    //int64_t occupiedSize;
    //map<int64_t, int64_t> dataMap;
}

bool DataField::isOccupied(int64_t byteNr) const
{
    dp2p_assert(p && reservedSize && 0 <= byteNr && byteNr < reservedSize);
    for(map<int64_t, int64_t>::const_iterator it = dataMap.begin(); it != dataMap.end(); ++it)
    {
        const int64_t _from = it->first;
        const int64_t _to = it->second;
        if(_from <= byteNr && byteNr <= _to)
            return true;
        else if(byteNr < _from)
            return false;
    }
    return false;
}

void DataField::setData(int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite)
{
    dp2p_assert(p && byteFrom <= byteTo && byteTo < reservedSize && srcBuffer);

    /* Copy the data */
    memcpy(p + byteFrom, srcBuffer, byteTo - byteFrom + 1);

    /* Find overlapping entries */
    list<pair<int64_t, int64_t> > overlappingEntries;
    for(map<int64_t, int64_t>::iterator it = dataMap.begin(); it != dataMap.end(); ++it)
    {
        const int64_t from = it->first;
        const int64_t to = it->second;
        if((byteFrom <= from && from <= byteTo) || (byteFrom <= to && to <= byteTo) || (from < byteFrom && byteTo < to)) {
            overlappingEntries.push_back(*it);
        }
    }
    dp2p_assert(overlappingEntries.empty() || overwrite);

    /* Erase overlapping entries and adapt the occupied size */
    for(list<pair<int64_t, int64_t> >::iterator it = overlappingEntries.begin(); it != overlappingEntries.end(); ++it) {
        dp2p_assert(1 == dataMap.erase(it->first));
        occupiedSize -= it->second - it->first + 1;
    }

    /* Calculate the boundaries of the new data region */
    int64_t leftBoundaryMerged = overlappingEntries.empty() ? byteFrom : std::min(byteFrom, overlappingEntries.front().first);
    int64_t rightBoundaryMerged =  overlappingEntries.empty() ? byteTo : std::max(byteTo, overlappingEntries.back().first);

    /* Adapt the occupied size */
    occupiedSize += rightBoundaryMerged - leftBoundaryMerged + 1;

    if(!dataMap.empty())
    {
        /* Find the left neighbor and merge if possible */
        DataMap::iterator neighborRight = dataMap.lower_bound(leftBoundaryMerged);
        if(neighborRight != dataMap.end()) {
            dp2p_assert(neighborRight->first > rightBoundaryMerged);
            if(neighborRight->first == rightBoundaryMerged + 1) {
                rightBoundaryMerged = neighborRight->second;
            }
        }
        if(neighborRight != dataMap.begin()) {
            --neighborRight;
            DataMap::iterator neighborLeft = neighborRight;
            ++neighborRight;
            dp2p_assert(neighborLeft->second < leftBoundaryMerged);
            if(neighborLeft->second == leftBoundaryMerged - 1) {
                leftBoundaryMerged = neighborLeft->first;
                dataMap.erase(neighborLeft);
            }
        }
        if(neighborRight != dataMap.end() && neighborRight->second == rightBoundaryMerged) {
            dataMap.erase(neighborRight);
        }
    }

    /* Create an entry in the data map */
    dp2p_assert(dataMap.insert(pair<int64_t, int64_t>(leftBoundaryMerged, rightBoundaryMerged)).second);
}

int64_t DataField::getData(int64_t offset, char* buffer, int bufferSize) const
{
    DataMap::const_iterator it = dataMap.begin();
    for( ; it != dataMap.end(); ++it)
    {
        if(it->first <= offset && offset <= it->second)
            break;
    }
    dp2p_assert(it != dataMap.end());

    const int64_t numCopiedBytes = std::min<int64_t>(it->second - offset + 1, bufferSize);

    memcpy(buffer, p + offset, numCopiedBytes);

    return numCopiedBytes;
}

bool DataField::full() const
{
    dp2p_assert(reservedSize > 0);
    return (occupiedSize == reservedSize);
}

char* DataField::getCopy(char* pCopy, int64_t size) const
{
    dp2p_assert(full());
    if(size > 0) {
    	dp2p_assert(size == reservedSize);
    } else {
    	pCopy = new char[reservedSize];
    }
    dp2p_assert(pCopy);
    memcpy(pCopy, p, reservedSize);
    return pCopy;
}

int64_t DataField::getContigInterval(int64_t offset) const
{
    DataMap::const_iterator it = dataMap.begin();
    for( ; it != dataMap.end(); ++it)
    {
        if(it->first <= offset && offset <= it->second)
            break;
    }
    dp2p_assert(it != dataMap.end());

    return it->second - offset + 1;
}

string DataField::printDownloadedData(int64_t offset) const
{
    string ret;
    char tmp[1024];
    for(DataMap::const_iterator it = dataMap.begin(); it != dataMap.end(); ++it)
    {
        if(it->second >= offset) {
            if(ret.empty())
                sprintf(tmp, "[%"PRId64",%"PRId64"]", std::max<int64_t>(it->first, offset), it->second);
            else
                sprintf(tmp, ",[%"PRId64",%"PRId64"]", std::max<int64_t>(it->first, offset), it->second);
            ret.append(tmp);
        }
    }
    return ret;
}

void DataField::toFile(string& fileName) const
{
	DBGMSG("occupied : %lld reserved : %lld",occupiedSize, reservedSize);
	assert(full());
	FILE* f = fopen(fileName.c_str(),"w");
	assert(f);
	assert(occupiedSize == (int64_t)fwrite(p, 1, occupiedSize, f));
	assert(0 == fclose(f));
}


/*
 * Private methods
 */
#if 0
void DataField::reserve(int64_t numBytes)
{
    dp2p_assert(p == NULL && reservedSize == 0 && occupiedSize == 0 && dataMap.empty());
    reservedSize = numBytes;
    p = new char[reservedSize];
    dp2p_assert(p);
}
#endif

void DataField::mergeMap()
{

}

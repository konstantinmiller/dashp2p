/****************************************************************************
 * SegmentStorage.h                                                         *
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

#ifndef SEGMENTSTORAGE_H_
#define SEGMENTSTORAGE_H_


#include "Dashp2pTypes.h"
#include "DashSegment.h"
#include "Contour.h"
#include "ContentIdSegment.h"
#include <map>
using std::map;


class StreamPosition {
public:
    StreamPosition(): segId(-1,-1,-1,-1), byte(-1) {}
    StreamPosition(ContentIdSegment segId, int64_t byte): segId(segId), byte(byte) {}
    StreamPosition(const StreamPosition& other): segId(other.segId), byte(other.byte) {}
    bool valid() const {return (segId.valid() && byte >= 0);}
    string toString() const;
    ContentIdSegment segId;
    int64_t byte;
};

class SegmentStorage
{
/* Public methods */
public:
    SegmentStorage();
    virtual ~SegmentStorage();
    void clear();
    bool initialized(ContentIdSegment segId) const;
    void initSegment(ContentIdSegment segId, int64_t numBytes, dash::Usec duration);
    void addData(ContentIdSegment segId, int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite);
    StreamPosition getData(StreamPosition startPos, Contour contour, char** buffer, int* bufferSize, int* bytesReturned, dash::Usec* usecReturned);
    StreamPosition getSegmentData(StreamPosition startPos, char** buffer, int* bufferSize, int* bytesReturned, dash::Usec* usecReturned);
    int64_t getTotalSize(ContentIdSegment segId) const;
    int64_t getTotalDuration(ContentIdSegment segId) const;
    const DashSegment& getSegment(ContentIdSegment segId) const;
    /* contiguous interval starting exactly at strPos */
    pair<dash::Usec, int64_t> getContigInterval(StreamPosition strPos, Contour contour) const;
    /* If data is available at the exactly position strPos */
    bool dataAvailable(StreamPosition strPos) const;
    string printDownloadedData(int startSegNr, int64_t offset) const;
    void toFile (ContentIdSegment segId, string& fileName) const;

/* Private types */
private:
    typedef map<ContentIdSegment, DashSegment*> SegMap;

/* Private methods */
private:
    DashSegment& getSegment(ContentIdSegment segId);

/* Private members */
private:
     SegMap segMap;
};

#endif /* SEGMENTSTORAGE_H_ */

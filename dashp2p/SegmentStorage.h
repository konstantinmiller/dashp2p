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

//#include "Dashp2pTypes.h"
#include "DashSegment.h"
#include "Contour.h"
#include "ContentId.h"
#include <map>
#include <mutex>
using std::map;
using std::mutex;

namespace dashp2p {

class StreamPosition {
public:
    StreamPosition(): segId(-1,-1,-1,-1), byte(-1) {}
    StreamPosition(ContentIdSegment segId, int64_t byte): segId(segId), byte(byte) {}
    StreamPosition(const StreamPosition& other): segId(other.segId), byte(other.byte) {}
    bool valid() const {return (segId.valid() && byte >= 0);}
    string toString() const;
public:
    ContentIdSegment segId;
    int64_t byte;
};

class SegmentStorage
{
/* Public methods */
public:
    static void init();
    static void cleanup();
    static bool initialized(const ContentId& contentId);
    static void initSegment(const ContentIdMpd& segId, int64_t numBytes = -1);
    static void initSegment(const ContentIdSegment& segId, int64_t numBytes, int64_t duration);
    static void setSize(const ContentId& contentId, int64_t numBytes);
    static void addData(const ContentId& contentId, int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite);
    static StreamPosition getData(StreamPosition startPos, Contour contour, char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned);
    static StreamPosition getSegmentData(StreamPosition startPos, char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned);
    static int64_t getTotalSize(const ContentId& contentId);
    //static int64_t getTotalDuration(ContentIdSegment segId);
    static DashObject& get(const ContentId& contentId);
    static DashSegment& get(const ContentIdSegment& segId);
    /* contiguous interval starting exactly at strPos */
    static pair<int64_t, int64_t> getContigInterval(StreamPosition strPos, Contour contour);
    /* If data is available at the exactly position strPos */
    static bool dataAvailable(StreamPosition strPos);
    //static string printDownloadedData(int startSegNr, int64_t offset);
    static void toFile (const ContentId& contentId, string& fileName);

/* Private methods */
private:
    SegmentStorage(){}
    virtual ~SegmentStorage(){}
    static DashObject& _get(const ContentId& contentId);
    static DashSegment& _get(const ContentIdSegment& segId);
    static pair<int64_t, int64_t> _getContigInterval(StreamPosition strPos, Contour contour); // not thread safe.
    static bool _dataAvailable(StreamPosition strPos);

/* Private types */
private:
    typedef map<const ContentIdMpd, DashObject*> MpdMap;
    typedef map<const ContentIdSegment, DashSegment*> SegMap;

/* Private members */
private:
    static MpdMap mpdMap;
    static SegMap segMap;
    static mutex _mutex;
};

}

#endif /* SEGMENTSTORAGE_H_ */

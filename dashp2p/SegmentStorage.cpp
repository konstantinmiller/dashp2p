/****************************************************************************
 * SegmentStorage.cpp                                                       *
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

#include "SegmentStorage.h"
#include "DebugAdapter.h"
#include <cassert>
//#include <cinttypes>

/* Disable all debug output in this file */
#ifdef DBGMSG
//# undef DBGMSG
//# define DBGMSG(...)
#endif

namespace dashp2p {

string StreamPosition::toString() const
{
    char tmp[1024];
    sprintf(tmp, "(RepId %d, SegNr %d, offset %" PRId64 ")", segId.bitRate(), segId.segmentIndex(), byte);
    return string(tmp);
}


SegmentStorage::SegmentStorage()
  : segMap()
{
}

SegmentStorage::~SegmentStorage()
{
    /* Delete the segment map and free storage */
    while(!segMap.empty()) {
        delete segMap.begin()->second;
        segMap.erase(segMap.begin());
    }
}

void SegmentStorage::clear()
{
    /* Delete the segment map and free storage */
    while(!segMap.empty()) {
        delete segMap.begin()->second;
        segMap.erase(segMap.begin());
    }
}

bool SegmentStorage::initialized(ContentIdSegment segId) const
{
    return (segMap.count(segId) > 0);
}

void SegmentStorage::initSegment(ContentIdSegment segId, int64_t numBytes, int64_t duration)
{
    DashSegment* dashSegment = new DashSegment(segId, numBytes, duration);
    dp2p_assert(segMap.insert(pair<ContentIdSegment, DashSegment*>(segId, dashSegment)).second);
}

void SegmentStorage::addData(ContentIdSegment segId, int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite)
{
    getSegment(segId).setData(byteFrom, byteTo, srcBuffer, overwrite);
}

StreamPosition SegmentStorage::getData(StreamPosition startPos, Contour contour, char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned)
{
    if(bufferSize[0]) {
        DBGMSG("Enter. Asked for up to %d bytes at position (RepId: %d, SegNr: %d, offset: %" PRId64 ").", bufferSize[0], startPos.segId.bitRate(), startPos.segId.segmentIndex(), startPos.byte);
    } else {
        DBGMSG("Enter. Asked for all available bytes at position (RepId: %d, SegNr: %d, offset: %" PRId64 ").", startPos.segId.bitRate(), startPos.segId.segmentIndex(), startPos.byte);
    }

    const pair<int64_t, int64_t> availableData = getContigInterval(startPos, contour);
    if(availableData.second == 0) {
        DBGMSG("No data available. Will return an invalid stream position.");
        return StreamPosition();
    }

    /* Reserve memory if none is given. */
    if(buffer[0] == NULL) {
        dp2p_assert(bufferSize[0] == 0);
        bufferSize[0] = availableData.second;
        buffer[0] = new char[bufferSize[0]];
    }

    DBGMSG("Available: %" PRId64 " bytes, %" PRId64 " us. Will return %" PRId64 " bytes.", availableData.second, availableData.first, std::min<int64_t>(bufferSize[0], availableData.second));

    bytesReturned[0] = 0;
    usecReturned[0] = 0;
    StreamPosition lastCopiedByte;
    StreamPosition nextByte2Copy = startPos;
    while(bytesReturned[0] < std::min<int64_t>(bufferSize[0], availableData.second))
    {
        const DashSegment& seg = getSegment(nextByte2Copy.segId);
        const pair<int64_t, int64_t> copiedData = seg.getData(nextByte2Copy.byte, buffer[0] + bytesReturned[0], bufferSize[0] - bytesReturned[0]);
        dp2p_assert(copiedData.second > 0);
        bytesReturned[0] += copiedData.second;
        usecReturned[0] += (copiedData.second * seg.getTotalDuration()) / seg.getTotalSize();
        DBGMSG("Copied %" PRId64 " bytes, %" PRId64 " us from position (RegId: %d, SegNr: %d, offset: %" PRId64 ").", copiedData.second, copiedData.first, nextByte2Copy.segId.bitRate(), nextByte2Copy.segId.segmentIndex(), nextByte2Copy.byte);
        lastCopiedByte.segId = nextByte2Copy.segId;
        lastCopiedByte.byte = nextByte2Copy.byte + copiedData.second - 1;
        if(lastCopiedByte.byte == seg.getTotalSize() - 1)
        {
            if(contour.hasNext(nextByte2Copy.segId)) {
                nextByte2Copy.segId = contour.getNext(nextByte2Copy.segId);
                nextByte2Copy.byte = 0;
            } else {
                dp2p_assert(bytesReturned[0] == std::min<int64_t>(bufferSize[0], availableData.second));
            }
        }
    }

    DBGMSG("Available: %" PRId64 " bytes, %" PRId64 " us. Returning: %" PRId32 " bytes, %" PRId64 " us. Last byte at position: (RepId: %" PRId32 ", SegNr: %" PRId32 ", offset: %" PRId64 ").",
            availableData.second, availableData.first, bytesReturned[0], usecReturned[0], lastCopiedByte.segId.bitRate(), lastCopiedByte.segId.segmentIndex(), lastCopiedByte.byte);
    return lastCopiedByte;

#if 0
    /* Get current time. */
    const double now = dashp2p::Utilities::now();

    /* Copy data from internal storage. */
    int bitRateLastByte = -1;
    usecReturned[0] = 0;
    bytesReturned[0] = 0;
    for(int i = initSegmentHasData ? 0 : start; i <= end && i <= stopSegment && bytesReturned[0] < bufferSize[0]; (i == 0) ? i = start : ++i)
    {
        segStored.first = i;
        Segment& seg = segStorage.at(i);
        while(!seg.data.empty() && bytesReturned[0] < bufferSize[0])
        {
            DataBlock& db = *seg.data.begin();
            dp2p_assert(db.from && db.to && db.to >= db.from);
            /* number of bytes to be copied */
            const int tbc = std::min<int>(bufferSize[0] - bytesReturned[0], (db.to - db.from) + 1);
            //DBGMSG("===> Next data block has %d bytes. VLC buffer has %d (%d) bytes free. Copying %d bytes.",
            //        (db.to - db.from) + 1, bufferSize[0] - bytesReturned[0], bufferSize[0], tbc);
            /* copy */
            memcpy(buffer[0] + bytesReturned[0], db.from, tbc);
            /* update variables */
            bitRateLastByte = seg.bitrate;
            bytesReturned[0] += tbc;
            usecReturned[0] += (double)tbc / (double)seg.bytes * (double)seg.usecs();
            bytesStored.first += tbc;
            /* Release internal memory after copying, if required. */
            if(tbc == (db.to - db.from) + 1) {
                delete [] db.p;
                seg.data.pop_front();
                if(seg.data.empty() && seg.completed) {
                    usecStored.first = i * mpdWrapper->getSegmentDuration();
                } else {
                    usecStored.first += (double)tbc / (double)seg.bytes * (double)seg.usecs();
                }
            } else {
                db.from += tbc;
                usecStored.first += (double)tbc / (double)seg.bytes * (double)seg.usecs();
            }

            /* Calculate if beta_min is increasing. */
            if(ifBetaMinIncreasing) {
                if(now >= timeUpperBoundBetaMin + 1) {
                    if(curBetaMin < lastBetaMin) {
                        ifBetaMinIncreasing = false;
                    } else {
                        if(now >= timeUpperBoundBetaMin + 2) {
                            lastBetaMin = (usecStored.second - usecStored.first) / 1e6;
                        } else {
                            lastBetaMin = curBetaMin;
                        }
                        curBetaMin = (usecStored.second - usecStored.first) / 1e6;
                        timeUpperBoundBetaMin = floor(now);
                    }
                } else {
                    curBetaMin = std::min(curBetaMin, (usecStored.second - usecStored.first) / 1e6);
                }
            }
        }
    }
#endif
}

StreamPosition SegmentStorage::getSegmentData(StreamPosition startPos, char** buffer, int* bufferSize, int* bytesReturned, int64_t* usecReturned)
{
	if(bufferSize[0]) {
		DBGMSG("Enter. Asked for up to %d bytes within a single segment at position (RepId: %d, SegNr: %d, offset: %" PRId64 ").", bufferSize[0], startPos.segId.bitRate(), startPos.segId.segmentIndex(), startPos.byte);
	} else {
		DBGMSG("Enter. Asked for all available bytes within a single segment at position (RepId: %d, SegNr: %d, offset: %" PRId64 ").", startPos.segId.bitRate(), startPos.segId.segmentIndex(), startPos.byte);
	}

	Contour contour;
	contour.setNext(startPos.segId);

	return getData(startPos, contour, buffer, bufferSize, bytesReturned, usecReturned);
}

int64_t SegmentStorage::getTotalSize(ContentIdSegment segId) const
{
    return getSegment(segId).getTotalSize();
}

int64_t SegmentStorage::getTotalDuration(ContentIdSegment segId) const
{
    return getSegment(segId).getTotalDuration();
}

const DashSegment& SegmentStorage::getSegment(ContentIdSegment segId) const
{
    SegMap::const_iterator it = segMap.find(segId);
    if(it == segMap.end()) {
        DBGMSG("Can't find segment (RepId: %d, SegNr: %d). Probably a bug.", segId.bitRate(), segId.segmentIndex());
        dp2p_assert(0);
    }
    const DashSegment* seg = it->second;
    return *seg;
}

pair<int64_t, int64_t> SegmentStorage::getContigInterval(StreamPosition strPos, Contour contour) const
{
    pair<int64_t, int64_t> availableData(0, 0);

    DBGMSG("Asking for contiguous interval at position (RepId: %d, SegNr: %d, offset: %" PRId64 ").", strPos.segId.bitRate(), strPos.segId.segmentIndex(), strPos.byte);

    /* If the given position is invalid, return 0 */
    if(!strPos.valid())
        return availableData;

    while(dataAvailable(strPos))
    {
        //DBGMSG("Checking (RepId: %d, SegNr: %d, offset: %"PRId64").", strPos.segId.repId(), strPos.segId.segNr(), strPos.byte);
        const DashSegment& seg = getSegment(strPos.segId);
        pair<int64_t, int64_t> _availableData = seg.getContigInterval(strPos.byte);
        //DBGMSG("Available: %"PRId64" us, %"PRId64" byte. Total segment size: %"PRId64".", _availableData.first, _availableData.second, seg.getTotalSize());
        availableData.first += _availableData.first;
        availableData.second += _availableData.second;
        if(strPos.byte + _availableData.second < seg.getTotalSize()) {
            break;
        } else if(contour.hasNext(strPos.segId)) {
            dp2p_assert(strPos.byte + _availableData.second == seg.getTotalSize());
            strPos.segId = contour.getNext(strPos.segId);
            strPos.byte = 0;
        } else {
            break;
        }
    }

    DBGMSG("Returning %" PRId64 " us, %" PRId64 " byte.", availableData.first, availableData.second);

    return availableData;
}

bool SegmentStorage::dataAvailable(StreamPosition strPos) const
{
    SegMap::const_iterator it = segMap.find(strPos.segId);
    if(it == segMap.end())
        return false;
    return it->second->hasData(strPos.byte);
}

string SegmentStorage::printDownloadedData(int startSegNr, int64_t offset) const
{
    string ret;
    ret.reserve(2048);
    for(SegMap::const_iterator it = segMap.begin(); it != segMap.end(); ++it)
    {
        const ContentIdSegment& segId = it->first;
        const DashSegment& seg = *it->second;
        if(segId.segmentIndex() < startSegNr)
            continue;
        const string segData = (segId.segmentIndex() == startSegNr) ? seg.printDownloadedData(offset) : seg.printDownloadedData(0);
        if(!segData.empty()) {
            char tmp[1024];
            sprintf(tmp, "(%d,%d,%" PRId64 "): ", segId.bitRate(), segId.segmentIndex(), offset);
            ret.append(tmp);
            ret.append(segData);
            ret.append("\n");
        }
    }
    return ret;
}

void SegmentStorage::toFile (ContentIdSegment segId, string& fileName) const
{
	const DashSegment& ds = getSegment(segId);
	ds.toFile(fileName);
}

/*
 * Private methods
 */

DashSegment& SegmentStorage::getSegment(ContentIdSegment segId)
{
    DBGMSG("Enter. Asking for segment (RepId: %d, SegNr: %d).", segId.bitRate(), segId.segmentIndex());
    SegMap::iterator it = segMap.find(segId);
    dp2p_assert(it != segMap.end());
    DashSegment* seg = it->second;
    return *seg;
}

}

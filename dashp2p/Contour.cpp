/****************************************************************************
 * Contour.cpp                                                              *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 21, 2012                                                 *
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

#include "Contour.h"
#include "DebugAdapter.h"
#include <assert.h>

#ifdef DBGMSG
# undef DBGMSG
# define DBGMSG(...)
#endif

using std::pair;

namespace dashp2p {

ContentIdSegment Contour::getStart() const
{
    dp2p_assert(!c.empty());
    ContourMap::const_iterator it = c.begin();
    dp2p_assert(c.count(it->first) == 1);
    ContentIdSegment segId(0, 0, it->second, it->first);
    return segId;
}

bool Contour::hasNext(const ContentIdSegment& segId) const
{
    /* Find given segment in the map */
    pair<ContourMap::const_iterator, ContourMap::const_iterator> range = c.equal_range(segId.segmentIndex());
    dp2p_assert(range.first != c.end() && distance(range.first, range.second) == 1);

    return (range.second != c.end());
}

ContentIdSegment Contour::getNext(const ContentIdSegment& segId) const
{
    DBGMSG("Asking for next segment to (%d, %d).", segId.repId(), segId.segNr());

    /* Find given segment in the map */
    pair<ContourMap::const_iterator, ContourMap::const_iterator> range = c.equal_range(segId.segmentIndex());
    if(range.first == c.end()) {
        DBGMSG("Not found. Probably a bug.");
        dp2p_assert(0);
    } else if(distance(range.first, range.second) != 1) {
        DBGMSG("More than one element for given segment Nr. Bug?");
        dp2p_assert(0);
    }
    //dp2p_assert(range.first != c.end() && range.second != c.end() && distance(range.first, range.second) == 1);

    /* If given segment is the last one, return an invalid SegId */
    if(range.second == c.end()) {
        return ContentIdSegment(-1, -1, -1, -1);
    }

    const ContentIdSegment segNext(0, 0, range.second->second, range.second->first);
    dp2p_assert(segNext.segmentIndex() == segId.segmentIndex() + 1 || segId.segmentIndex() == 0);

    DBGMSG("Found segment (%d, %d).", segNext.repId(), segNext.segNr());

    range = c.equal_range(segNext.segmentIndex());
    if(range.first == c.end() || distance(range.first, range.second) != 1) {
        DBGMSG("Asked for next segment in the contour for (RepId: %d, SegNr: %d). Map contains (RepId: %d, SegNr: %d). Probably a bug.", segId.bitRate(), segId.segmentIndex(), segNext.bitRate(), segNext.segmentIndex());
        dp2p_assert(0);
    }
    return segNext;
}

void Contour::setNext(const ContentIdSegment& nextSeg)
{
    pair<ContourMap::const_iterator, ContourMap::const_iterator> rangeNext = c.equal_range(nextSeg.segmentIndex());
    if(rangeNext.first != c.end()) {
        ERRMSG("Trying to set next segment to (RepId: %d, SegNr: %d) but this SegNr is already in the Contour.", nextSeg.bitRate(), nextSeg.segmentIndex());
        ERRMSG("Contour: %s.", toString().c_str());
    }
    dp2p_assert(rangeNext.second == c.end());

    if(!c.empty()) {
        ContourMap::const_iterator it = c.end();
        --it;
        const int lastSegNr = it->first;
        dp2p_assert_v(lastSegNr == 0 || nextSeg.segmentIndex() == lastSegNr + 1, "Last segment in the contour: %d, trying to add as next: %d. Probably a bug.", lastSegNr, nextSeg.segmentIndex());
    }

    c.insert(pair<int,int>(nextSeg.segmentIndex(), nextSeg.bitRate()));
}

string Contour::toString() const
{
    string ret;
    ret.reserve(1024);
    char tmp[1024];
    for(ContourMap::const_iterator it = c.begin(); it != c.end(); ++it) {
        sprintf(tmp, "(%d,%d)", it->first, it->second);
        ret.append(tmp);
    }
    return ret;
}

}

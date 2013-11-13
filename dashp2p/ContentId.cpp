/****************************************************************************
 * ContentId.cpp                                                            *
 ****************************************************************************
 * Copyright (C) 2013 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 12, 2013                                                 *
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

#include "ContentId.h"


/*bool ContentId::operator<(const ContentId& other) const
{
    dp2p_assert(getType() != other.getType());
    return getType() < other.getType();
}*/

bool ContentIdMpd::operator==(const ContentId& other) const
{
    if(other.getType() == ContentType_Mpd)
        return operator==(dynamic_cast<const ContentIdMpd&>(other));
    else
        return false;
}

string ContentIdSegment::toString() const
{
    ostringstream oss;
    oss << "SegId(" << _periodIndex << "," << _adaptationSetIndex << "," << _bitRate << "," << _segmentIndex << ")";
    return oss.str();
}

bool ContentIdSegment::operator==(const ContentId& other) const
{
    if(other.getType() == ContentType_Segment)
        return operator==(dynamic_cast<const ContentIdSegment&>(other));
    else
        return false;
}

bool ContentIdSegment::operator==(const ContentIdSegment& other) const
{
    return (   _periodIndex == other._periodIndex
            && _adaptationSetIndex == other._adaptationSetIndex
            && _bitRate == other._bitRate
            && _segmentIndex == other._segmentIndex);
}

bool ContentIdSegment::operator<(const ContentId& other) const
{
    if(other.getType() == ContentType_Segment)
        return operator<(dynamic_cast<const ContentIdSegment&>(other));
    else
        return ContentType_Segment < other.getType();
}
bool ContentIdSegment::operator<(const ContentIdSegment& other) const
{
    if(_periodIndex < other._periodIndex) return true;
    if(_periodIndex > other._periodIndex) return false;
    if(_adaptationSetIndex < other._adaptationSetIndex) return true;
    if(_adaptationSetIndex > other._adaptationSetIndex) return false;
    if(_bitRate < other._bitRate) return true;
    if(_bitRate > other._bitRate) return false;
    if(_segmentIndex < other._segmentIndex) return true;
    return false;
}

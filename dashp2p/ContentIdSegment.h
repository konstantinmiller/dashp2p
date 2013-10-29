/****************************************************************************
 * ContentIdSegment.h                                                       *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 09, 2012                                                 *
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

#ifndef CONTENTIDSEGMENT_H_
#define CONTENTIDSEGMENT_H_


#include "ContentId.h"
#include <sstream>
using std::ostringstream;


class ContentIdSegment: public ContentId
{
public:
	ContentIdSegment(int periodIndex, int adaptationSetIndex, int bitRate, int segmentIndex): _periodIndex(periodIndex), _adaptationSetIndex(adaptationSetIndex), _bitRate(bitRate), _segmentIndex(segmentIndex) {}
	virtual ~ContentIdSegment() {}
	virtual ContentIdSegment* copy() const {return new ContentIdSegment(*this);}
	virtual ContentType getType() const {return ContentType_Segment;}
	virtual string toString() const {ostringstream oss; oss << "SegId(" << _periodIndex << "," << _adaptationSetIndex << "," << _bitRate << "," << _segmentIndex << ")"; return oss.str();}

	int periodIndex() const {return _periodIndex;}
	int adaptationSetIndex() const {return _adaptationSetIndex;}
	int bitRate() const {return _bitRate;}
	int segmentIndex() const {return _segmentIndex;}
	bool valid() const {return (_periodIndex >= 0 && _adaptationSetIndex >= 0 && _bitRate >= 0 && _segmentIndex >= 0);}

	bool operator<(const ContentIdSegment& other) const {
		if(_periodIndex < other._periodIndex) return true;
		if(_periodIndex > other._periodIndex) return false;
		if(_adaptationSetIndex < other._adaptationSetIndex) return true;
		if(_adaptationSetIndex > other._adaptationSetIndex) return false;
		if(_bitRate < other._bitRate) return true;
		if(_bitRate > other._bitRate) return false;
		if(_segmentIndex < other._segmentIndex) return true;
		return false;
	}
	bool operator==(const ContentId& other) const {if(other.getType() == ContentType_Segment) return operator==(dynamic_cast<const ContentIdSegment&>(other)); else return false;}
	//bool operator!=(const ContentId& other) const {return !operator==(other);}
	bool operator==(const ContentIdSegment& other) const {return (_periodIndex == other._periodIndex && _adaptationSetIndex == other._adaptationSetIndex && _bitRate == other._bitRate && _segmentIndex == other._segmentIndex);}

private:
	int _periodIndex;
	int _adaptationSetIndex;
	int _bitRate;
	int _segmentIndex;
};


#endif /* CONTENTIDSEGMENT_H_ */

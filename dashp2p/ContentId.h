/****************************************************************************
 * ContentId.h                                                              *
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

#ifndef CONTENTID_H_
#define CONTENTID_H_

#include "DebugAdapter.h"
//#include "PeerId.h"
#include <string>
#include <sstream>
using std::ostringstream;
using std::string;

enum ContentType {
	ContentType_Mpd     = 0,
	//ContentType_MpdPeer = 1,
	ContentType_Segment = 2,
	//ContentType_Tracker = 3,
	//ContentType_Generic = 4
};

class ContentId
{
protected:
	ContentId(){};

public:
	virtual ~ContentId(){};
	virtual ContentId* copy() const = 0;
	virtual ContentType getType() const = 0;
	virtual string toString() const = 0;
	virtual bool operator==(const ContentId& other) const = 0;
	virtual bool operator!=(const ContentId& other) const {return !operator==(other);}
};


class ContentIdMpd: public ContentId
{
public:
	ContentIdMpd() {}
	virtual ~ContentIdMpd() {}
	virtual ContentIdMpd* copy() const {return new ContentIdMpd;}
	virtual ContentType getType() const {return ContentType_Mpd;}
	virtual string toString() const {return "MPD";}
	bool operator==(const ContentId& other) const {if(other.getType() == ContentType_Mpd) return operator==(dynamic_cast<const ContentIdMpd&>(other)); else return false;}
	virtual bool operator==(const ContentIdMpd& /*other*/) const {return true;}
	//virtual bool operator!=(const ContentIdMpd& other) const {return !operator==(other);}
};


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

#if 0
class ContentIdGeneric: public ContentId
{
public:
	ContentIdGeneric(const string& fileName): fileName(fileName) {}
	virtual ~ContentIdGeneric() {}
	virtual ContentIdGeneric* copy() const {return new ContentIdGeneric(fileName);}
	virtual ContentType getType() const {return ContentType_Generic;}
	virtual string toString() const {return fileName;}
	bool operator==(const ContentId& other) const {if(other.getType() == ContentType_Generic) return operator==(dynamic_cast<const ContentIdGeneric&>(other)); else return false;}
	virtual bool operator==(const ContentIdGeneric& other) const {return (0 == fileName.compare(other.fileName));}
	//virtual bool operator!=(const ContentIdMpd& other) const {return !operator==(other);}
private:
	string fileName;
};


class ContentIdMpdPeer: public ContentId
{
public:
	ContentIdMpdPeer(const PeerId& peerId): peerId(peerId){}
	virtual ~ContentIdMpdPeer() {}
	virtual ContentIdMpdPeer* copy() const {return new ContentIdMpdPeer(peerId);}
	virtual ContentType getType() const {return ContentType_MpdPeer;}
	virtual string toString() const {ostringstream ret("MPDPeer("); ret << peerId.toString() << ")"; return ret.str();}
	bool operator==(const ContentId& other) const {if(other.getType() == ContentType_MpdPeer) return operator==(dynamic_cast<const ContentIdMpdPeer&>(other)); else return false;}
	virtual bool operator==(const ContentIdMpdPeer& other) const {return peerId == other.peerId;}
private:
	PeerId peerId;
};


class ContentIdTracker: public ContentId
{
public:
	ContentIdTracker() {}
	virtual ~ContentIdTracker() {}
	virtual ContentIdTracker* copy() const {return new ContentIdTracker;}
	virtual ContentType getType() const {return ContentType_Tracker;}
	virtual string toString() const {return "Tracker";}
	virtual bool operator==(const ContentId& other) const {if(other.getType() == ContentType_Tracker) return operator==(dynamic_cast<const ContentIdTracker&>(other)); else return false;}
	virtual bool operator==(const ContentIdTracker& /*other*/) const {return true;}
};
#endif

#endif /* CONTENTID_H_ */

/****************************************************************************
 * ContentIdMpdPeer.h                                                       *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 28, 2012                                                 *
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

#ifndef CONTENTIDMPDPEER_H_
#define CONTENTIDMPDPEER_H_


#include "ContentId.h"
#include "PeerId.h"
#include <ostream>
using std::ostringstream;


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


#endif /* CONTENTIDMPDPEER_H_ */

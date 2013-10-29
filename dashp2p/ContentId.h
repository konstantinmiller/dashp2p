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
#include <string>
using std::string;


enum ContentType {
	ContentType_Mpd     = 0,
	ContentType_MpdPeer = 1,
	ContentType_Segment = 2,
	ContentType_Tracker = 3,
	ContentType_Generic = 4
};

class ContentIdMpd;
class ContentIdMpdPeer;
class ContentIdSegment;
class ContentIdTracker;

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


#endif /* CONTENTID_H_ */

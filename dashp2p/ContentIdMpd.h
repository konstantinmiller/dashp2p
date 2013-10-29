/****************************************************************************
 * ContentIdMpd.h                                                           *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 12, 2012                                                 *
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

#ifndef CONTENTIDMPD_H_
#define CONTENTIDMPD_H_


#include "ContentId.h"


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


#endif /* CONTENTIDMPD_H_ */

/****************************************************************************
 * ContentIdGeneric.h                                                       *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Feb 20, 2013                                                 *
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

#ifndef CONTENTIDGENERIC_H_
#define CONTENTIDGENERIC_H_


#include "ContentId.h"


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


#endif /* CONTENTIDGENERIC_H_ */

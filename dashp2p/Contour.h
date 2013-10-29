/****************************************************************************
 * Contour.h                                                                *
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

#ifndef CONTOUR_H_
#define CONTOUR_H_


#include "Dashp2pTypes.h"
#include "ContentIdSegment.h"
#include <list>
#include <map>
using std::list;
using std::multimap;


class Contour
{
/* Public methods */
public:
    Contour(): c() {}
    virtual ~Contour() {}
    bool empty() const {return c.empty();}
    int size() const {return c.size();}
    void clear() {c.clear();}
    ContentIdSegment getStart() const;
    bool hasNext(const ContentIdSegment& segId) const;
    ContentIdSegment getNext(const ContentIdSegment& segId) const;
    void setNext(const ContentIdSegment& nextSeg);
    string toString() const;

/* Private types */
private:
    typedef multimap<int, int> ContourMap;

/* Private members */
private:
    ContourMap c;
};

#endif /* CONTOUR_H_ */

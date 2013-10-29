/****************************************************************************
 * PeerId.h                                                                 *
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

#ifndef PEERID_H_
#define PEERID_H_


#include <stdint.h>
#include <string>
using std::string;


class PeerId
{
public:
	//PeerId();
	PeerId(string id):id(id){}
	virtual ~PeerId(){}
	const string toString() const{return id;}
	//string toStringHex() const;
	bool operator==(const PeerId& other) const{return !id.compare(other.id);}
private:
	//static const int len;
	//static uint64_t cnt;
	string id;
};

#endif /* PEERID_H_ */

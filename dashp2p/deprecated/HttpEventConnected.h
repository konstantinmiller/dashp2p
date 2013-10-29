/****************************************************************************
 * HttpEventConnected.h                                                     *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 20, 2012                                                 *
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

#ifndef HTTPEVENTCONNECTED_H_
#define HTTPEVENTCONNECTED_H_


#include "HttpEvent.h"
#include <list>
using std::list;


class HttpEventConnected: public HttpEvent
{
public:
	HttpEventConnected(int connId): HttpEvent(connId) {}
	virtual ~HttpEventConnected(){}
	virtual HttpEventType getType() const {return HttpEvent_Connected;}
};


#endif /* HTTPEVENTCONNECTED_H_ */

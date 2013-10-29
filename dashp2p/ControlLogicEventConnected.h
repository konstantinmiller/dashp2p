/****************************************************************************
 * ControlLogicEventConnected.h                                             *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 19, 2012                                                 *
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

#ifndef CONTROLLOGICEVENTCONNECTED_H_
#define CONTROLLOGICEVENTCONNECTED_H_


#include "ControlLogicEvent.h"


class ControlLogicEventConnected: public ControlLogicEvent
{
public:
	ControlLogicEventConnected(int connId): connId(connId) {}
    virtual ~ControlLogicEventConnected() {}
    virtual ControlLogicEventType getType() const {return Event_Connected;}

public:
    const ConnectionId connId;
};


#endif /* CONTROLLOGICEVENTCONNECTED_H_ */

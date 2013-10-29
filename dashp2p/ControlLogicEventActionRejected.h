/****************************************************************************
 * ControlLogicEventActionRejected.h                                        *
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

#ifndef CONTROLLOGICEVENTACTIONREJECTED_H_
#define CONTROLLOGICEVENTACTIONREJECTED_H_


#include "ControlLogicEvent.h"
#include "HttpRequest.h"
#include "HttpEventDisconnect.h"


class ControlLogicEventActionRejected: public ControlLogicEvent
{
public:
	ControlLogicEventActionRejected(ControlLogicAction* a): a(a) {}
    virtual ~ControlLogicEventDisconnect() {delete a;}
    virtual ControlLogicEventType getType() const {return Event_ActionRejected;}

public:
    ControlLogicAction* a;
};


#endif /* CONTROLLOGICEVENTACTIONREJECTED_H_ */

/****************************************************************************
 * ControlLogicEvent.h                                                      *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 24, 2012                                                 *
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

#ifndef CONTROLLOGICEVENT_H_
#define CONTROLLOGICEVENT_H_


//#include "Dashp2pTypes.h"


class ControlLogicEvent
{
public:
    ControlLogicEvent() {}
    virtual ~ControlLogicEvent(){}
    virtual ControlLogicEventType getType() const = 0;
    virtual string toString() const {
    	switch(getType()) {
    	case Event_ActionRejected: return "ActionRejected";
    	case Event_Connected:      return "Connected";
    	case Event_DataPlayed:     return "DataPlayed";
    	case Event_DataReceived:   return "DataReceived";
    	case Event_Disconnect:     return "Disconnect";
    	case Event_Pause:          return "Pause";
    	case Event_ResumePlayback: return "ResumePlayback";
    	case Event_StartPlayback:  return "StartPlayback";
    	default: ERRMSG("Unknown ControlLogicEvent type."); abort(); break;
    	}
    	return "";
    }
};

#endif /* CONTROLLOGICEVENT_H_ */

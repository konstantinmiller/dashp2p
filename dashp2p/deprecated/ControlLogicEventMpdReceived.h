/****************************************************************************
 * ControlLogicEventMpdReceived.h                                           *
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

#ifndef CONTROLLOGICEVENTMPDRECEIVED_H_
#define CONTROLLOGICEVENTMPDRECEIVED_H_


#include "Dashp2pTypes.h"
#include "ControlLogicEvent.h"
#include <vector>
using std::vector;


class ControlLogicEventMpdReceived: public ControlLogicEvent
{
public:
    ControlLogicEventMpdReceived(MpdWrapper* mpdWrapper, const vector<Representation>& representations, SegNr startSegment, SegNr stopSegment)
      : ControlLogicEvent(),
        mpdWrapper(mpdWrapper),
        representations(representations),
        startSegment(startSegment),
        stopSegment(stopSegment) {}
    virtual ~ControlLogicEventMpdReceived(){}
    virtual ControlLogicEventType getType() const {return Event_MpdReceived;}

public:
    MpdWrapper* mpdWrapper;
    const vector<Representation> representations;
    const SegNr startSegment;
    const SegNr stopSegment;
};

#endif /* CONTROLLOGICEVENTMPDRECEIVED_H_ */

/****************************************************************************
 * ControlLogicActionUpdateContour.h                                        *
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

#ifndef CONTROLLOGICACTIONUPDATECONTOUR_H_
#define CONTROLLOGICACTIONUPDATECONTOUR_H_


#include "Dashp2pTypes.h"
#include "ControlLogicAction.h"
#include <list>
using std::list;


class ControlLogicActionUpdateContour: public ControlLogicAction
{
public:
    ControlLogicActionUpdateContour(): ControlLogicAction(), updateAction(UNDEF), segId(-1,-1) {}
    virtual ~ControlLogicActionUpdateContour(){}
    virtual ControlLogicActionType getType() const {return Action_UpdateContour;}

public:
    enum UpdateAction {UNDEF, SET_NEXT};

public:
    UpdateAction updateAction;
    SegId segId;
};


#endif /* CONTROLLOGICACTIONUPDATECONTOUR_H_ */

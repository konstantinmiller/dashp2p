/****************************************************************************
 * ControlLogicP2P.h                                                        *
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

#ifndef CONTROLLOGICP2P_H_
#define CONTROLLOGICP2P_H_


#include "ControlLogic.h"
#include <string>
using std::string;


class ControlLogicP2P: public ControlLogic
{
/* Public methods */
public:
    ControlLogicP2P(const string& config);
    virtual ~ControlLogicP2P();
    virtual ControlType getType() const {return ControlType_P2P;}
    virtual list<ControlLogicAction*> processEvent(const ControlLogicEvent* e);
};


#endif /* CONTROLLOGICP2P_H_ */

/****************************************************************************
 * ControlLogicActionCloseTcpConnection.h                                   *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 12, 2012                                                 *
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

#ifndef CONTROLLOGICACTIONCLOSETCPCONNECTION_H_
#define CONTROLLOGICACTIONCLOSETCPCONNECTION_H_


#include "Dashp2pTypes.h"
#include "ControlLogicAction.h"
#include <sstream>
using std::ostringstream;
#include <list>
using std::list;


class ControlLogicActionCloseTcpConnection: public ControlLogicAction
{
public:
	ControlLogicActionCloseTcpConnection(int id): ControlLogicAction(), id(id) {}
    virtual ~ControlLogicActionCloseTcpConnection(){}
    virtual ControlLogicAction* copy() const {return new ControlLogicActionCloseTcpConnection(id);}
    virtual ControlLogicActionType getType() const {return Action_CloseTcpConnection;}
    virtual string toString() const {ostringstream ret; ret << "CloseTcpConnection " << id; return ret.str();}

public:
    const int id;
};


#endif /* CONTROLLOGICACTIONCLOSETCPCONNECTION_H_ */

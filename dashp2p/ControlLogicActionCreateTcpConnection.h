/****************************************************************************
 * ControlLogicActionCreateTcpConnection.h                                  *
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

#ifndef CONTROLLOGICACTIONCREATETCPCONNECTION_H_
#define CONTROLLOGICACTIONCREATETCPCONNECTION_H_


#include "Dashp2pTypes.h"
#include "ControlLogicAction.h"
#include <list>
using std::list;


class ControlLogicActionCreateTcpConnection: public ControlLogicAction
{
public:
    ControlLogicActionCreateTcpConnection(int id, const IfData& ifData, const string& hostName, int maxPendingRequests)
      : ControlLogicAction(), id(id), ifData(ifData), hostName(hostName), maxPendingRequests(maxPendingRequests), port(80) {}
    ControlLogicActionCreateTcpConnection(int id, const IfData& ifData, const string& hostName, int maxPendingRequests, int port)
          : ControlLogicAction(), id(id), ifData(ifData), hostName(hostName), maxPendingRequests(maxPendingRequests), port(port) {}
    virtual ~ControlLogicActionCreateTcpConnection(){}
    virtual ControlLogicAction* copy() const {return new ControlLogicActionCreateTcpConnection(id, ifData, hostName, maxPendingRequests);}
    virtual ControlLogicActionType getType() const {return Action_CreateTcpConnection;}
    virtual string toString() const {ostringstream ret; ret << "CreateTcpConnection " << id << "," << hostName << "," <<"," << port << ifData.toString() << "," << maxPendingRequests; return ret.str();}

public:
    const int id;
    const IfData ifData;
    const string hostName;
    const int maxPendingRequests;
    const int port;
};


#endif /* CONTROLLOGICACTIONCREATETCPCONNECTION_H_ */

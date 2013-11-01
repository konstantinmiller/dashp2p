/****************************************************************************
 * ControlLogicAction.h                                                     *
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

#ifndef CONTROLLOGICACTION_H_
#define CONTROLLOGICACTION_H_

#include "dashp2p.h"
//#include "Dashp2pTypes.h"
#include "ContentId.h"
#include "Utilities.h"
//using dashp2p::Utilities;
#include "HttpClientManager.h"

#include <string>
#include <sstream>
#include <list>
using std::ostringstream;
using std::list;
using std::string;

namespace dashp2p {

class ControlLogicAction
{
public:
    virtual ~ControlLogicAction(){}
    virtual ControlLogicAction* copy() const = 0;
    virtual ControlLogicActionType getType() const = 0;
    virtual string toString() const = 0;
};


class ControlLogicActionCloseTcpConnection: public ControlLogicAction
{
public:
	ControlLogicActionCloseTcpConnection(const TcpConnectionId& tcpConnectionId): ControlLogicAction(), tcpConnectionId(tcpConnectionId) {}
    virtual ~ControlLogicActionCloseTcpConnection(){}
    virtual ControlLogicAction* copy() const {return new ControlLogicActionCloseTcpConnection(tcpConnectionId);}
    virtual ControlLogicActionType getType() const {return Action_CloseTcpConnection;}
    virtual string toString() const {ostringstream ret; ret << "CloseTcpConnection " << (string)tcpConnectionId; return ret.str();}
public:
    const TcpConnectionId tcpConnectionId;
};


/*
class ControlLogicActionOpenTcpConnection: public ControlLogicAction
{
public:
	ControlLogicActionOpenTcpConnection(const int& tcpConnectionId): ControlLogicAction(), tcpConnectionId(tcpConnectionId) {}
    virtual ~ControlLogicActionOpenTcpConnection(){}
    virtual ControlLogicAction* copy() const {return new ControlLogicActionOpenTcpConnection(tcpConnectionId);}
    virtual ControlLogicActionType getType() const {return Action_CreateTcpConnection;}
    virtual string toString() const {ostringstream ret; ret << "CreateTcpConnection " << tcpConnectionId; return ret.str();}
public:
    const int tcpConnectionId;
    //const IfData ifData;
    //const string hostName;
    //const int maxPendingRequests;
    //const int port = 80;
};*/


class ControlLogicActionStartDownload: public ControlLogicAction
{
public:
    ControlLogicActionStartDownload(const TcpConnectionId& tcpConnectionId, list<const ContentId*> contentIds, list<dashp2p::URL> urls, list<HttpMethod> httpMethods)
      : ControlLogicAction(), tcpConnectionId(tcpConnectionId), contentIds(contentIds), urls(urls), httpMethods(httpMethods) {}
    virtual ~ControlLogicActionStartDownload() {while(!contentIds.empty()){delete contentIds.front(); contentIds.pop_front();}}
    virtual ControlLogicAction* copy() const;
    virtual ControlLogicActionType getType() const {return Action_StartDownload;}
    virtual string toString() const;
    virtual bool operator==(const ControlLogicActionStartDownload& other) const;
public:
    const TcpConnectionId tcpConnectionId;
    list<const ContentId*> contentIds;
    list<dashp2p::URL> urls;
    list<HttpMethod> httpMethods;
};

}

#endif /* CONTROLLOGICACTION_H_ */

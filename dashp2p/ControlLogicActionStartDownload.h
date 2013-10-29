/****************************************************************************
 * ControlLogicActionStartDownload.h                                        *
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

#ifndef CONTROLLOGICACTIONSTARTDOWNLOAD_H_
#define CONTROLLOGICACTIONSTARTDOWNLOAD_H_

#include "Dashp2pTypes.h"
#include "ContentId.h"
#include "ControlLogicAction.h"
#include "Utilities.h"
using dash::Utilities;
#include <list>
using std::list;

class ControlLogicActionStartDownload: public ControlLogicAction
{
public:
    ControlLogicActionStartDownload(int connId, list<const ContentId*> contentIds, list<dash::URL> urls, list<HttpMethod> httpMethods)
      : ControlLogicAction(), connId(connId), contentIds(contentIds), urls(urls), httpMethods(httpMethods) {}
    virtual ~ControlLogicActionStartDownload() {while(!contentIds.empty()){delete contentIds.front(); contentIds.pop_front();}}
    virtual ControlLogicAction* copy() const;
    virtual ControlLogicActionType getType() const {return Action_StartDownload;}
    virtual string toString() const;
    virtual bool operator==(const ControlLogicActionStartDownload& other) const;

public:
    int connId;
    list<const ContentId*> contentIds;
    list<dash::URL> urls;
    list<HttpMethod> httpMethods;
};

#endif /* CONTROLLOGICACTIONSTARTDOWNLOAD_H_ */

/****************************************************************************
 * ControlLogic.h                                                           *
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

#ifndef CONTROLLOGIC_H_
#define CONTROLLOGIC_H_


#include "Dashp2pTypes.h"
#include "Contour.h"
#include "ControlLogicActionStartDownload.h"
#include "ControlLogicEventConnected.h"
#include "ControlLogicEventDataPlayed.h"
#include "ControlLogicEventDataReceived.h"
#include "ControlLogicEventDisconnect.h"
#include "ControlLogicEventPause.h"
#include "ControlLogicEventResumePlayback.h"
#include "ControlLogicEventStartPlayback.h"
#include "ThreadAdapter.h"
#include "MpdWrapper.h"
#include "DataField.h"
#include <list>
#include <vector>
#include <map>
using std::vector;
using std::list;
using std::map;

class ControlLogic
{

/* Constructor, destructor */
protected:

	ControlLogic(int width, int height, Usec startPosition, Usec stopPosition);

/* Public methods */
public:

	virtual ~ControlLogic();

    virtual ControlType getType() const = 0;

    virtual list<ControlLogicAction*> processEvent(ControlLogicEvent* e);
    virtual list<ControlLogicAction*> actionRejected(ControlLogicAction* a);

    virtual int getStartSegment() const;
    virtual int getStopSegment() const;

    virtual const MpdWrapper* getMpdWrapper() const {return mpdWrapper;}
    virtual const Contour& getContour() const {return contour;}

    virtual void setStartPosition(Usec startPosition) {this->startPosition = startPosition;}

/* Protected methods */
protected:

    virtual list<ControlLogicAction*> processEventConnected           (const ControlLogicEventConnected& e)      = 0;
    virtual list<ControlLogicAction*> processEventDataPlayed          (const ControlLogicEventDataPlayed& e)     = 0;
    virtual list<ControlLogicAction*> processEventDataReceived        (ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDataReceivedMpd     (ControlLogicEventDataReceived& e)         = 0;
    virtual list<ControlLogicAction*> processEventDataReceivedMpdPeer (ControlLogicEventDataReceived& e)         = 0;
    virtual list<ControlLogicAction*> processEventDataReceivedSegment (ControlLogicEventDataReceived& e)         = 0;
    virtual list<ControlLogicAction*> processEventDataReceivedTracker (ControlLogicEventDataReceived& e)         = 0;
    virtual list<ControlLogicAction*> processEventDisconnect          (const ControlLogicEventDisconnect& e)     = 0;
    virtual list<ControlLogicAction*> processEventPause               (const ControlLogicEventPause& e)          = 0;
    virtual list<ControlLogicAction*> processEventResumePlayback      (const ControlLogicEventResumePlayback& e) = 0;
    virtual list<ControlLogicAction*> processEventStartPlayback       (const ControlLogicEventStartPlayback& e)  = 0;

    virtual bool ackActionConnected        (const ConnectionId& connId);
    virtual bool ackActionRequestCompleted (const ContentId& contentId);
    virtual bool ackActionDisconnect       (const ConnectionId& connId);

    virtual list<ControlLogicAction*> actionRejectedStartDownload(ControlLogicActionStartDownload* a) = 0;

    virtual IfData getInitialIf() const {return ifData.at(0);}

    virtual unsigned getIndex(int bitrate);
    virtual void processEventDataReceivedMpd_Completed();
    virtual ControlLogicAction* createActionDownloadSegments(list<const ContentId*> segIds, int connId, HttpMethod httpMethod) const;

/* Protected types */
protected:
    enum ControlLogicState {NO_MPD, HAVE_MPD, DONE};
    typedef list<ControlLogicAction*> ActionList;

/* Protected members */
protected:
    ControlLogicState state;
    Mutex mutex;

    int width;
    int height;

    Usec startPosition;      // [us]
    Usec stopPosition;       // [us]

    /* Stores the vector of available representations. Assumes it to be sorted in ascending order. */
    vector<int> bitRates;

    std::vector<IfData> ifData;

    Contour contour;

    MpdWrapper* mpdWrapper;
    DataField* mpdDataField;

    ActionList pendingActions;
};

#endif /* CONTROLLOGIC_H_ */

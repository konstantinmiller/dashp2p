/****************************************************************************
 * ControlLogicMH.h                                                         *
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

#ifndef CONTROLLOGICMH_H_
#define CONTROLLOGICMH_H_


#include "ControlLogic.h"
#include "TimeSeries.h"
#include <vector>
using std::vector;


class ControlLogicMH: public ControlLogic
{

/* Constructor, destructor */
private:
    ControlLogicMH(const std::string& config);
    virtual ~ControlLogicMH();

/* Public methods */
public:
    virtual ControlType getType() const {return ControlType_MH;}

    virtual list<ControlLogicAction*> processEventDataPlayed    (const ControlLogicEventDataPlayed& e);
    virtual list<ControlLogicAction*> processEventDataReceived  (const ControlLogicEventDataReceived& e);
    virtual list<ControlLogicAction*> processEventDisconnect    (const ControlLogicEventDisconnect& e);
    virtual list<ControlLogicAction*> processEventPause         (const ControlLogicEventPause& e);
    virtual list<ControlLogicAction*> processEventResumePlayback(const ControlLogicEventResumePlayback& e);
    virtual list<ControlLogicAction*> processEventStartPlayback (const ControlLogicEventStartPlayback& e);


    IfData getIfData(int i) const {return ifData.at(i);};
    int getIfInd(string devName) const;
    int getPrimaryIfInd() const {return getIfInd(getPrimaryIf().name);}
    int getSecondaryIfInd() const {return getIfInd(getSecondaryIf().name);}
    IfData getPrimaryIf() const;
    IfData getSecondaryIf() const;
    bool hasSecondary() const {return getSecondaryIf().hasAddr();}

/* Private types */
private:
    /* Data tupe to hold the decision. */
    class Decision {
    public:
        Decision(const Representation& rep, double Bdelay, int reason) : rep(rep), Bdelay(Bdelay), reason(reason) {}
        Representation rep;
        dash::Usec     Bdelay;
        int            reason; // for debugging/logging purposes
    };

/* Private members */
private:
    /* Parameters */
    double Bmin;
    double Blow;
    double Bhigh;
    double alfa1;
    double alfa2;
    double alfa3;
    double alfa4;
    double alfa5;
    double Delta_t;

    TimeSeries<dash::Usec>* betaTimeSeries;

    /* Indicates if we are in the initial increase phase. */
    bool initialIncrease;

    /* Time when the initial increase phase was terminated. */
    double initialIncreaseTerminationTime;

    vector<dash::Usec> Bdelay;
    vector<list<const ContentIdSegment*> > delayedRequests;

/* Private methods */
private:
    /* Selects the representation for the next segment and the buffer level when the download should be started (Inf, if immediately). */
    Decision selectRepresentation(bool ifBetaMinIncreasing, double beta,
            double rho, double rhoLast, unsigned completedRequests, int r_last);
};


#endif /* CONTROLLOGICMH_H_ */

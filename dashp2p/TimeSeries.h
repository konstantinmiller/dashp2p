/****************************************************************************
 * TimeSeries.h                                                             *
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

#ifndef TIMESERIES_H_
#define TIMESERIES_H_

#define __STDC_FORMAT_MACROS

#include "DebugAdapter.h"
#include "Utilities.h"
#include <limits>
#include <list>
#include <vector>
#include <string>
#include <inttypes.h>
using std::numeric_limits;
using std::pair;
using std::list;
using std::vector;
using std::string;


template <class T>
class TimeSeries
{
/* Private methods */
public:
    TimeSeries(dash::Usec discrStep, bool recordingAll, bool recordingDiscrMin);
    virtual ~TimeSeries(){}
    void pushBack(dash::Usec time, const T& x);
    bool minIncreasing() const {return isMinIncreasing;}
    string printDiscretizedMin() const;

/* Private types */
private:
    typedef pair<dash::Usec, T> Element;

/* Private members */
private:
    static const unsigned vecSize;
    const dash::Usec discrStep;
    const bool recordingAll;
    const bool recordingDiscrMin;
    list<vector<Element> > data;
    list<Element> dataDiscrMin;
    bool isMinIncreasing;
    T lastMinValue;
    T curMinValue;
    dash::Usec curStepNr; // minimum number of the step we are expecting next element to be in
    dash::Usec lastTime;
};



template <class T>
const unsigned TimeSeries<T>::vecSize = 1024;


template <class T>
TimeSeries<T>::TimeSeries(dash::Usec discrStep, bool recordingAll, bool recordingDiscrMin)
: discrStep(discrStep),
  recordingAll(recordingAll),
  recordingDiscrMin(recordingDiscrMin),
  data(),
  dataDiscrMin(),
  isMinIncreasing(true),
  lastMinValue(numeric_limits<T>::min()),
  curMinValue(numeric_limits<T>::max()),
  curStepNr(-1),
  lastTime(0)
  {
    // blank
  }

template <class T>
void TimeSeries<T>::pushBack(dash::Usec time, const T& x)
{
    DBGMSG("Enter. Time: %"PRId64" us, level: %"PRId64" us.", time, x);

    dp2p_assert_v(time >= lastTime, "time: %"PRId64", lastTime: %"PRId64, time, lastTime);
    lastTime = time;

    /* Are we recording everything? */
    if(recordingAll)
    {
        /* Add new vector if necessary */
        if(data.empty() || data.back().size() == vecSize) {
            data.push_back(vector<Element>());
            data.back().reserve(vecSize);
        }

        vector<Element>& vec = data.back();

        /* Assert chornological soring */
        if(!vec.empty()) {
            const Element& lastEl = vec.back();
            dp2p_assert(lastEl.first <= time);
        }

        vec.push_back(Element(time, x));
    }

    /* The rest is only if the discretized min value is still increasing */
    if(isMinIncreasing == false) {
        DBGMSG("Already non-increasing. Return.");
        return;
    }

    dp2p_assert(curStepNr * discrStep <= time);

    /* Is this the first value? */
    if(curStepNr == -1)
    {
        DBGMSG("Received first value: (%"PRId64", %"PRId64").", time, x);
        curStepNr = time / discrStep;
        curMinValue = x;
    }

    /* Are we still in the same discrete step? If yes, update the curMinValue. */
    else if(time < (curStepNr + 1) * discrStep)
    {
        DBGMSG("Same step: %"PRId64". Updating the minimum from %"PRId64" to %"PRId64".", curStepNr, curMinValue, std::min<T>(x, curMinValue));
        curMinValue = std::min<T>(x, curMinValue);
    }

    /* Did we cross the discrete step boundary? */
    else
    {
        DBGMSG("Last step: %"PRId64", new step: %"PRId64". Last min value: %"PRId64", current min value: %"PRId64".", curStepNr, time / discrStep, lastMinValue, curMinValue);

        for(unsigned i = curStepNr; i < time / discrStep; ++i)
            dataDiscrMin.push_back(Element(curStepNr * discrStep, curMinValue));
        curStepNr = time / discrStep;
        if(curMinValue < lastMinValue) {
            isMinIncreasing = false;
            DBGMSG("Increasing stopped!");
        }
        lastMinValue = curMinValue;
        curMinValue = x;
    }
}

template <class T>
string TimeSeries<T>::printDiscretizedMin() const
{
    string ret;
    if(dataDiscrMin.empty()) {
        return ret;
    }

    ret.reserve(1024);
    char tmp[1024];
    for(typename list<Element>::const_iterator it = dataDiscrMin.begin(); it != dataDiscrMin.end(); ++it)
    {
        sprintf(tmp, "(%.3f,%.3f) | ", it->first / 1e6, it->second / 1e6);
        ret.append(tmp);
    }
    ret.erase(ret.size() - 3);

    return ret;
}

#endif /* TIMESERIES_H_ */

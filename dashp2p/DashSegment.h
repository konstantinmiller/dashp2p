/****************************************************************************
 * DashSegment.h                                                            *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 20, 2012                                                 *
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

#ifndef DASHSEGMENT_H_
#define DASHSEGMENT_H_

#include "ContentIdSegment.h"
#include "DataField.h"
#include "Utilities.h"
#include <string>
using std::string;
using std::pair;


/* Type for storing information about a segment. */
class DashSegment
{
/* Public methods */
public:
    DashSegment(const ContentIdSegment& segId, int64_t numBytes, dash::Usec duration);
    ~DashSegment(){}
    void setData(int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite);
    pair<dash::Usec, int64_t> getData(int64_t offset, char* buffer, int bufferSize) const;
    int64_t getTotalSize() const;
    dash::Usec getTotalDuration() const {return duration;}
    bool completed() const {return dataField.full();}
    bool hasData(int64_t byteNr) const {return dataField.isOccupied(byteNr);}
    pair<dash::Usec, int64_t> getContigInterval(int64_t offset) const;
    string printDownloadedData(int64_t offset) const {return dataField.printDownloadedData(offset);}
    void toFile(string& fileName) const;

public:
    const ContentIdSegment segId;

/* Private members */
private:
    DataField dataField;
    const dash::Usec duration;
};
#endif /* DASHSEGMENT_H_ */

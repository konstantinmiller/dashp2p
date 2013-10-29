/****************************************************************************
 * DataField.h                                                              *
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

#ifndef DATAFIELD_H_
#define DATAFIELD_H_


#include <stdint.h>
#include <map>
#include <string>
using std::map;
using std::string;

namespace dashp2p {

class DataField
{
/* Public methods */
public:
    DataField(int64_t numBytes);
    virtual ~DataField();
    int64_t getReservedSize() const {return reservedSize;}
    int64_t getOccupiedSize() const {return occupiedSize;}
    bool isOccupied(int64_t byteNr) const;
    void setData(int64_t byteFrom, int64_t byteTo, const char* srcBuffer, bool overwrite);
    int64_t getData(int64_t offset, char* buffer, int bufferSize) const;
    bool full() const;
    char* getCopy(char* pCopy = NULL, int64_t size = 0) const;
    int64_t getContigInterval(int64_t offset) const;
    string printDownloadedData(int64_t offset) const;
    void toFile(string& fileName) const ;

/* Private methods */
private:
    //void reserve(int64_t numBytes);
    void mergeMap();

/* Private types */
private:
    /* <byteFrom, byteTo> */
    typedef map<int64_t, int64_t> DataMap;

/* Private members */
private:
    char* p;
    int64_t reservedSize;
    int64_t occupiedSize;
    DataMap dataMap;
};

}

#endif /* DATAFIELD_H_ */

/****************************************************************************
 * AdaptationMultihomed.h                                                   *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 19, 2012                                                 *
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

#ifndef ADAPTATIONMULTIHOMED_H_
#define ADAPTATIONMULTIHOMED_H_

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class AdaptationMultihomed
{
/* Public methods */
public:
    AdaptationMultihomed(const std::string& config);
    virtual ~AdaptationMultihomed(){}

    struct sockaddr_in getPrimarySockAddr() const;
    struct sockaddr_in getSecondarySockAddr() const;

/* Private types */
private:
    class IfData {
    public:
        IfData(): initialized(false), name(), sockaddr_in(), primary(false) {}
        ~IfData(){}
        bool initialized;
        std::string name;
        struct sockaddr_in sockaddr_in;
        bool primary;
    };

/* Public members */
private:
    std::vector<IfData> ifData;

/* Private methods */
private:
    const IfData& getPrimaryIf() const;
    const IfData& getSecondaryIf() const;
};

#endif /* ADAPTATIONMULTIHOMED_H_ */

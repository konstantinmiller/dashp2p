/****************************************************************************
 * HttpClientManager.h                                                      *
 ****************************************************************************
 * Copyright (C) 2013 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Oct 32, 2013                                                 *
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

#ifndef HTTPCLIENTMANAGER_H_
#define HTTPCLIENTMANAGER_H_

//#include "DashHttp.h"
#include "TcpConnectionManager.h"

#include <vector>
using std::vector;

namespace dashp2p {

// forward declarations
class DashHttp;

#if 0
// class HttpClientId
class HttpClientId {
public:
    HttpClientId(const int& id = -1): id(id) {}
    bool operator==(const HttpClientId& other) const {return other.id == id;}
    bool operator!=(const HttpClientId& other) const {return !(other == *this);}
    int numeric() const {return id;} // TODO: replace by casting operator
    operator string() const {return std::to_string(id);}
private:
    int id;
};
#endif

// class HttpClientManager
class HttpClientManager
{
public: // public methods
    static void cleanup();
    static void create(const TcpConnectionId& tcpConnectionId, const HttpCb& cb);
    static void destroy(const TcpConnectionId& tcpConnectionId);
    static DashHttp& get(const TcpConnectionId& tcpConnectionId) {return *httpVec.at(tcpConnectionId.numeric());}

private: // private methods
    HttpClientManager(){}
    virtual ~HttpClientManager(){}

private: // private types
    typedef vector<DashHttp*> HttpVec;

private: // private fields
    static HttpVec httpVec;
};

} /* namespace dashp2p */
#endif /* HTTPCLIENTMANAGER_H_ */

/*
 * HttpClientManager.cpp
 *
 *  Created on: Oct 31, 2013
 *      Author: miller
 */

#include "HttpClientManager.h"
#include "dashp2p.h"
#include "DashHttp.h"

namespace dashp2p {

// static variables in HttpClientManager
HttpClientManager::HttpVec HttpClientManager::httpVec(1024, NULL);

void HttpClientManager::cleanup()
{
    for(std::size_t i = 0; i < httpVec.size(); ++i)
        delete httpVec.at(i);
    httpVec.clear();
}

void HttpClientManager::create(const TcpConnectionId& id, const HttpCb& cb)
{
    while(id.numeric() >= (int)httpVec.size())
        httpVec.resize(2 * httpVec.size(), NULL);

    dp2p_assert(!httpVec.at(id.numeric()));

    httpVec.at(id.numeric()) = new DashHttp(id, cb);
}

void HttpClientManager::destroy(const TcpConnectionId& tcpConnectionId)
{
    delete httpVec.at(tcpConnectionId.numeric());
    httpVec.at(tcpConnectionId.numeric()) = NULL;
}

} /* namespace dashp2p */

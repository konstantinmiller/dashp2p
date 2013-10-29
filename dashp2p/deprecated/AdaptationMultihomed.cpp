/****************************************************************************
 * AdaptationMultihomed.cpp                                                 *
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

#include "AdaptationMultihomed.h"
#include "DebugAdapter.h"
#include "Utilities.h"
#include <assert.h>
#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef __ANDROID
# include <ifaddrs.h>
#endif


AdaptationMultihomed::AdaptationMultihomed(const std::string& config)
{
#ifdef __ANDROID__
    return;
#else
    DBGMSG("Initializing AdaptationMultihomed with configuration: %s.", config.c_str());

    /* Get the names of the devices */
    dp2p_assert(config.at(0) == '(');
    size_t end = config.find(')', 1);
    dp2p_assert(end != config.npos);
    std::string devsStr = config.substr(1, end - 1);
    std::vector<std::string> ifNames = dash::Utilities::tokenize(devsStr, ':');
    dp2p_assert(ifNames.size() == 2); // only support 2 devs currently
    ifData.resize(ifNames.size());
    for(unsigned i = 0; i < ifNames.size(); ++i)
        ifData.at(i).name = ifNames.at(i);

    /* Get the IP addresses */
    struct ifaddrs* ifaddrs = NULL;
    dp2p_assert(0 == getifaddrs(&ifaddrs));
    dp2p_assert(ifaddrs != NULL);
    struct ifaddrs* ifaddrNext = ifaddrs;
    while(ifaddrNext != NULL)
    {
        if(ifaddrNext->ifa_addr->sa_family == AF_INET) {
            for(unsigned i = 0; i < ifData.size(); ++i) {
                if(0 == strcmp(ifaddrNext->ifa_name, ifData.at(i).name.c_str())) {
                    dp2p_assert(ifData.at(i).initialized == false);
                    memcpy(&ifData.at(i).sockaddr_in, ifaddrNext->ifa_addr, sizeof(ifData.at(i).sockaddr_in));
                    ifData.at(i).initialized = true;
                    break;
                }
            }
        }
        ifaddrNext = ifaddrNext->ifa_next;
    }
    freeifaddrs(ifaddrs);
    for(unsigned i = 0; i < ifData.size(); ++i)
        dp2p_assert(ifData.at(i).initialized);
    dp2p_assert(ifData.at(0).sockaddr_in.sin_port == 0 && ifData.at(1).sockaddr_in.sin_port == 0);

    DBGMSG("Will use %s (%s).", ifData.at(0).name.c_str(), inet_ntoa(ifData.at(0).sockaddr_in.sin_addr)); // cannot combine to single call due to static buffer in inet_ntoa()
    DBGMSG("Will use %s (%s).", ifData.at(1).name.c_str(), inet_ntoa(ifData.at(1).sockaddr_in.sin_addr));

    /* Check what is the primary interface */
    char buf[2048] = {0}; // should be enough for any usage below
    bool found = false;
    FILE* f = popen("ip route list", "r");
    dp2p_assert(f > 0);
    found = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, "default")) {
            dp2p_assert(!found);
            found = true;
            char* p = strstr(buf, "dev");
            dp2p_assert(p);
            p += 4;
            while(*p == ' ')
                ++p;
            if(0 == strncmp(p, ifData.at(0).name.c_str(), ifData.at(0).name.size())) {
                ifData.at(0).primary = true;
                DBGMSG("%s is primary interface.", ifData.at(0).name.c_str());
            } else if(0 == strncmp(p, ifData.at(1).name.c_str(), ifData.at(1).name.size())) {
                found = true;
                DBGMSG("%s is primary interface.", ifData.at(1).name.c_str());
            } else {
                dp2p_assert(0);
            }
        }
    }
    dp2p_assert(found);
    dp2p_assert(0 == fclose(f));

    /* Check if we have the entry 'mhstreaming' in /etc/iproute2/rt_tables */
    f = fopen("/etc/iproute2/rt_tables", "r");
    if(!f) {
        ERRMSG("The file /etc/iproute2/rt_tables does not exist. Routing based on source address (required for multihoming) is not possible.");
        dp2p_assert(0);
    }
    found = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, "mhstreaming")) {
            dp2p_assert(!found);
            found = true;
        }
    }
    dp2p_assert(0 == fclose(f));
    dp2p_assert(found);
    if(!found) {
        ERRMSG("No entry for the mhstreaming routing table found in /etc/iproute2/rt_tables. Please create one.");
        dp2p_assert(0);
    }
    DBGMSG("/etc/iproute2/rt_tables exists and has an entry 'mhstreaming'. Good!");

    /* Check if we have a correct default route in the mhstreaming table */
    f = popen("ip route list table mhstreaming", "r");
    dp2p_assert(f > 0);
    found = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, "default via")) {
            dp2p_assert(!found);
            char tmp[1024];
            sprintf(tmp, "dev %s", getSecondaryIf().name.c_str());
            dp2p_assert(strstr(buf, tmp));
            found = true;
        }
    }
    dp2p_assert(0 == fclose(f));
    dp2p_assert(found);
    DBGMSG("Default route for the mhstreaming routing table is set correctly. Good!");

    /* check if the routing rules are correct for the secondary interface */
    char tmp1[2048];
    sprintf(tmp1, "from all to %s lookup mhstreaming", inet_ntoa(getSecondaryIf().sockaddr_in.sin_addr));
    char tmp2[2048];
    sprintf(tmp2, "from %s lookup mhstreaming", inet_ntoa(getSecondaryIf().sockaddr_in.sin_addr));
    f = popen("ip rule show", "r");
    dp2p_assert(f > 0);
    bool found1 = false;
    bool found2 = false;
    while(fgets(buf, sizeof(buf), f)) {
        if(strstr(buf, tmp1)) {
            dp2p_assert(!found1);
            found1 = true;
        } else if(strstr(buf, tmp2)) {
            dp2p_assert(!found2);
            found2 = true;
        }
    }
    dp2p_assert(0 == fclose(f));
    dp2p_assert(found1 && found2);
    DBGMSG("Routes from/to secondary IP are set correctly. Good!");

    DBGMSG("AdaptationMultihomed successfully initialized.");
#endif
}

struct sockaddr_in AdaptationMultihomed::getPrimarySockAddr() const
{
    return getPrimaryIf().sockaddr_in;
}

struct sockaddr_in AdaptationMultihomed::getSecondarySockAddr() const
{
    return getSecondaryIf().sockaddr_in;
}

const AdaptationMultihomed::IfData& AdaptationMultihomed::getPrimaryIf() const
{
    int ind = -1;
    for(unsigned i = 0; i < ifData.size(); ++i) {
        if(ifData.at(i).primary) {
            dp2p_assert(ind == -1);
            ind = i;
        }
    }
    dp2p_assert(ind != -1);
    return ifData.at(ind);
}

const AdaptationMultihomed::IfData& AdaptationMultihomed::getSecondaryIf() const
{
    int ind = -1;
    for(unsigned i = 0; i < ifData.size(); ++i) {
        if(!ifData.at(i).primary) {
            dp2p_assert(ind == -1);
            ind = i;
        }
    }
    dp2p_assert(ind != -1);
    return ifData.at(ind);
}

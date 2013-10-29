/****************************************************************************
 * Utilities.cpp                                                            *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Aug 10, 2012                                                 *
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

#include "dashp2p.h"
#include "Utilities.h"
#include "DebugAdapter.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
//#include <cinttypes>

namespace dashp2p {

bool operator>=(const struct timespec& a, const struct timespec& b)
{
    if(a.tv_sec > b.tv_sec) {
        return true;
    } else if(a.tv_sec == b.tv_sec && a.tv_nsec >= b.tv_nsec) {
        return true;
    } else {
        return false;
    }
}

bool operator>(const struct timespec& a, const int64_t& b)
{
    if(a.tv_sec > b / 1000000ll) {
        return true;
    } else if(a.tv_sec == b / 1000000ll && a.tv_nsec > (b % 1000000ll) * 1000ll) {
        return true;
    } else {
        return false;
    }
}

struct timespec operator+(const struct timespec& a, const struct timespec& b)
{
    struct timespec ret;
    ret.tv_nsec = (a.tv_nsec + b.tv_nsec) % 1000000000ll;
    int64_t carry = (a.tv_nsec + b.tv_nsec) / 1000000000ll;
    dp2p_assert(0 <= carry && carry <= 1);
    ret.tv_sec = a.tv_sec + b.tv_sec + carry;
    return ret;
}

struct timespec operator-(const struct timespec& a, const struct timespec& b)
{
    dp2p_assert(a >= b);
    struct timespec ret;
    ret.tv_sec = a.tv_sec - b.tv_sec;
    if(a.tv_nsec >= b.tv_nsec) {
        ret.tv_nsec = a.tv_nsec - b.tv_nsec;
    } else {
        --ret.tv_sec;
        ret.tv_nsec = 1000000000ll - (b.tv_nsec - a.tv_nsec);
    }
    return ret;
}


const URL& URL::operator=(const URL& other)
{
    if(&other == this)
        return *this;
    access = other.access;
    hostName = other.hostName;
    path = other.path;
    fileName = other.fileName;
    withoutHostname = other.withoutHostname;
    whole = other.whole;
    return *this;
}


/*************************************************************************
 * Time
 *************************************************************************/
int64_t Utilities::referenceTime = 0;

int64_t Utilities::getTime()
{
    return getAbsTime() - referenceTime;
}

int64_t Utilities::getAbsTime()
{
    struct timespec ts;
    dp2p_assert(0 == clock_gettime(CLOCK_REALTIME, &ts));
    return (int64_t)1000000 * (int64_t)ts.tv_sec + (int64_t)ts.tv_nsec / (int64_t)1000;
}

std::string Utilities::getTimeString(int64_t t, bool showDate)
{
    static char buf[1024];
    time_t _t;
    int64_t usecs = 0;
    if(t == 0) {
        int64_t absTime = getAbsTime();
        _t = absTime / 1000000;
        usecs = absTime % 1000000;
    } else {
        _t = t / 1000000;
        usecs = t % 1000000;
    }
    struct tm T;
    localtime_r(&_t, &T);
    if(showDate)
    	dp2p_assert(strftime(buf, 1024, "%d.%m.%Y %H:%M:%S", &T) + 2 < 1024); // + 2 in order to ensure that nothing was truncated
    else
    	dp2p_assert(strftime(buf, 1024, "%H:%M:%S", &T) + 2 < 1024); // + 2 in order to ensure that nothing was truncated
    sprintf(buf + strlen(buf), ".%06" PRId64, usecs);
    return std::string(buf);
}

std::string Utilities::getTimeString(const struct timespec& t, bool showDate)
{
    static char buf[1024];
    time_t secs = t.tv_sec;
    struct tm T;
    localtime_r(&secs, &T);
    if(showDate)
        dp2p_assert(strftime(buf, 1024, "%d.%m.%Y %H:%M:%S", &T) + 2 < 1024); // + 2 in order to ensure that nothing was truncated
    else
        dp2p_assert(strftime(buf, 1024, "%H:%M:%S", &T) + 2 < 1024); // + 2 in order to ensure that nothing was truncated
    sprintf(buf + strlen(buf), ".%06ld", t.tv_nsec / 1000);
    return std::string(buf);
}

double Utilities::now()
{
    return getTime() / 1e6;
}

int64_t Utilities::convertTime2Epoch(const string& str)
{
    int h = -1;
    int m = -1;
    int s = -1;
    if(3 != sscanf(str.c_str(), "%d:%d:%d", &h, &m, &s) || h < 0 || m < 0 || s < 0) {
        ERRMSG("Could not convert time string [%s] to epoch time.", str.c_str());
        abort();
    }
    time_t t = getAbsTime() / 1000000;
    struct tm T;
    localtime_r(&t, &T);
    T.tm_hour = h;
    T.tm_min = m;
    T.tm_sec = s;
    return (int64_t)1000000 * (int64_t)mktime(&T);
}

int64_t Utilities::convertTime(const string& str)
{
    int h = -1;
    int m = -1;
    int s = -1;
    if(3 != sscanf(str.c_str(), "%d:%d:%d", &h, &m, &s) || h < 0 || m < 0 || s < 0) {
        ERRMSG("Could not convert time string [%s] to relative time.", str.c_str());
        abort();
    }
    return (int64_t)(h * 3600 + m * 60 + s) * (int64_t)1000000;
}

struct timespec Utilities::add(const struct timespec& a, const int64_t& usec)
{
    struct timespec ret;
    ret.tv_nsec = (a.tv_nsec + (usec % 1000000ll) * 1000ll) % 1000000000ll;
    int64_t carry = (a.tv_nsec + (usec % 1000000ll) * 1000ll) / 1000000000ll;
    dp2p_assert(0 <= carry && carry <= 1);
    ret.tv_sec = a.tv_sec + usec / 1000000ll + carry;
    return ret;
}

struct timeval Utilities::usec2tv(const int64_t& usec)
{
	struct timeval ret;
	ret.tv_sec = usec / (int64_t)1000000;
	ret.tv_usec = usec % (int64_t)1000000;
	return ret;
}

/*************************************************************************
 * Sleeping
 *************************************************************************/
void Utilities::sleepSelect(unsigned sec, unsigned usec)
{
    dp2p_assert(usec < 1000000);
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    dp2p_assert(0 == select(0, NULL, NULL, NULL, &timeout));
}


/*************************************************************************
 * Random numbers
 *************************************************************************/
unsigned short Utilities::randState[3] = {0,0,0};

int Utilities::getRandInt (int a, int b)
{
    if( a == b ) {
        return a;
    }
    dp2p_assert( a < b );
    if( randState[0] == 0 && randState[1] == 0 && randState[2] == 0 ) {
        int64_t tmp = getTime();
        randState[0] = tmp >> 32;
        randState[1] = tmp >> 16;
        randState[2] = tmp;
        printf("initializing seed to {%u,%u,%u}.\n", randState[0], randState[1], randState[2]);
    }
    return nrand48( randState ) % (b - a + 1) + a;
}


/*************************************************************************
 * String manipulation
 *************************************************************************/
const std::string Utilities::emptyString;

URL Utilities::splitURL(const std::string& url)
{
    if(url.empty())
        return URL();

    /* get the access type */
    std::string access;
    std::string::size_type endOfAccessType = url.find("://");
    if(endOfAccessType != url.npos) {
        access = url.substr(0, endOfAccessType);
        dp2p_assert(access.compare("http") == 0);
    }

    /* get the host name */
    const int beginOfHostName = access.empty() ? 0 : access.size() + 3;
    std::string::size_type endOfHostName = url.find('/', beginOfHostName);
    dp2p_assert(endOfHostName != url.npos);
    --endOfHostName;
    const std::string hostName = url.substr(beginOfHostName, endOfHostName - beginOfHostName + 1);

    /* get the path without the file name */
    std::string::size_type startOfFileName = url.find_last_of('/');
    dp2p_assert(startOfFileName != url.npos);
    ++startOfFileName;
    const std::string path = (startOfFileName == endOfHostName + 2) ? "" : url.substr(endOfHostName + 2, startOfFileName - endOfHostName - 3);

    /* get the file name */
    const std::string fileName = url.substr(startOfFileName);

    //printf("Splitting %s into (%s, %s, %s, %s)\n", url.c_str(), access.c_str(), hostName.c_str(), path.c_str(), fileName.c_str());

    /* return */
    return URL(access, hostName, path, fileName);
}
Utilities::PeerInformation Utilities::splitIPStringMPDPeer(const std::string& ipString)
{
	struct PeerInformation actualPeerInfo;
	std::string portString;
	size_t i=ipString.find_first_of(":");
	actualPeerInfo.port = atoi(portString.append(ipString, i+1, ipString.size()).c_str());
	actualPeerInfo.ip = ipString.substr(0,i);
	return actualPeerInfo;
}
void Utilities::getRandomString(char* buffer, int len)
{
    for (int i = 0; i < len - 1; ++i) {
        int randomChar = rand()%(26+26+10);
        if (randomChar < 26)
            buffer[i] = 'a' + randomChar;
        else if (randomChar < 26+26)
            buffer[i] = 'A' + randomChar - 26;
        else
            buffer[i] = '0' + randomChar - 26 - 26;
    }
    buffer[len - 1] = 0;
}

bool Utilities::isAbsoluteUrl(const std::string& url)
{
    return url.compare(0, 7, "http://") == 0;
}

std::vector<std::string> Utilities::tokenize(const std::string& str, char sep)
{
    std::vector<std::string> ret;

    std::size_t pos = 0;
    while(pos < str.size())
    {
        std::size_t sepPos = str.find(sep, pos);
        if(sepPos == str.npos) {
            ret.push_back(str.substr(pos, str.size() - pos));
            break;
        }
        ret.push_back(str.substr(pos, sepPos - pos));
        pos = sepPos + 1;
    }

    return ret;
}

string Utilities::toString(const HttpMethod& httpMethod)
{
	switch(httpMethod) {
	case HttpMethod_GET:  return "GET";
	case HttpMethod_HEAD: return "HEAD";
	default: ERRMSG("Unknown HttpMethod %d.", httpMethod); abort(); return "";
	}
}

#if 0
void Utilities::replaceHostName(std::string& url, const std::string& newHostName)
{
    unsigned start = url.find("://");
    dp2p_assert(start != url.npos);
    start += 3;

    unsigned end = url.find("/", start);
    dp2p_assert(end != url.npos);
    end -= 1;

    url.replace(start, end - start + 1, newHostName);
}
#endif

}

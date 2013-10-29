/****************************************************************************
 * Utilities.h                                                              *
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

#ifndef UTILITIES_H_
#define UTILITIES_H_

//#include "Dashp2pTypes.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>
//#include <cinttypes>

using std::string;

class HttpMethod;

namespace dashp2p {

bool operator>=(const struct timespec& a, const struct timespec& b);
bool operator>(const struct timespec& a, const int64_t& b);
struct timespec operator+(const struct timespec& a, const struct timespec& b);
struct timespec operator-(const struct timespec& a, const struct timespec& b);

/*************************************************************************
 * class URL
 *************************************************************************/
class URL
{
public:
    URL(){}

    URL(const std::string& access, const std::string& hostName, const std::string& path, const std::string& fileName):
        access(access),
        hostName(hostName),
        path(path),
        fileName(fileName),
        withoutHostname(path),
        whole(access)
    {
        if(access.empty()) {
            this->access = "http";
            whole = this->access;
        }
        withoutHostname.append("/");
        withoutHostname.append(fileName);
        whole.append("://");
        whole.append(hostName);
        whole.append("/");
        whole.append(path);
        whole.append("/");
        whole.append(fileName);
    }

    URL(const URL& other) {*this = other;}

    const URL& operator=(const URL& other);

    bool operator==(const URL& other) const {
    	return !access.compare(other.access) &&
    			!hostName.compare(other.hostName) &&
    			!path.compare(other.path) &&
    			!fileName.compare(other.fileName) &&
    			!withoutHostname.compare(other.withoutHostname) &&
    			!whole.compare(other.whole);
    }
    bool operator!=(const URL& other) const {return !operator==(other);}

    std::string access;
    std::string hostName;
    std::string path;
    std::string fileName;
    std::string withoutHostname;
    std::string whole;
};

/*************************************************************************
 *************************************************************************
 * class Utilities
 *************************************************************************
 *************************************************************************/
class Utilities
{

/*************************************************************************
 * Time
 *************************************************************************/
public:
    static int64_t getTime();
    static int64_t getAbsTime();
    static std::string getTimeString(int64_t t = 0, bool showDate = true);
    static std::string getTimeString(const struct timespec& t, bool showDate = true);
    static double now();
    static void setReferenceTime() {/* dp2p_assert(referenceTime==0); */ referenceTime = getTime();} // TODO: hanle nicely!
    static int64_t getReferenceTime() {return referenceTime;}
    static int64_t convertTime2Epoch(const string& str);
    static int64_t convertTime(const string& str);
    static struct timespec add(const struct timespec& a, const int64_t& usec);
    static struct timeval usec2tv(const int64_t& usec);
private:
    static int64_t referenceTime;

/*************************************************************************
 * Sleeping
 *************************************************************************/
public:
    static void sleepSelect(unsigned sec, unsigned usec);

/*************************************************************************
 * Random numbers
 *************************************************************************/
public:
    static int getRandInt (int a, int b);
private:
    static unsigned short randState[3];

/*************************************************************************
 * String manipulation
 *************************************************************************/
public:
    static URL splitURL(const std::string& url);
    struct PeerInformation { std::string ip ; int port ; };
    static struct PeerInformation splitIPStringMPDPeer(const std::string& ipString);
    //static void replaceHostName(std::string& url, const std::string& newHostName);
    static const std::string emptyString;
    static void getRandomString(char* buffer, int len);
    static bool isAbsoluteUrl(const std::string& url);
    static std::vector<std::string> tokenize(const std::string& str, char sep);
    static string toString(const HttpMethod& httpMethod);


/*************************************************************************
 * Construction and destruction
 *************************************************************************/
private:
    Utilities() {throw std::logic_error("Utilities class cannot be instantiated.");}
    virtual ~Utilities() {throw std::logic_error("Utilities class cannot be instantiated (and thus destroyed).");}
};

} // namespace dashp2p

#endif

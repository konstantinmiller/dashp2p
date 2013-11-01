/****************************************************************************
 * ControlLogicAction.cpp                                                   *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Nov 19, 2012                                                 *
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

#include <ControlLogicAction.h>

namespace dashp2p {

ControlLogicAction* ControlLogicActionStartDownload::copy() const
{
	list<const ContentId*> contentIds_copy;
	for(list<const ContentId*>::const_iterator it = contentIds.begin(); it != contentIds.end(); ++it) {
		contentIds_copy.push_back((*it)->copy());
	}
	return new ControlLogicActionStartDownload(tcpConnectionId, contentIds_copy, urls, httpMethods);
}

string ControlLogicActionStartDownload::toString() const
{
	ostringstream ret;
	ret << "StartDownload: { ";
	ret << "HttpClientId: " << (string)tcpConnectionId << ", ";
	list<const ContentId*>::const_iterator it = contentIds.begin();
	list<dashp2p::URL>::const_iterator        jt = urls.begin();
	list<HttpMethod>::const_iterator       kt = httpMethods.begin();
	for(; it != contentIds.end(); ++it, ++jt, ++kt) {
		if(it != contentIds.begin())
			ret << ",";
		ret << "(" << (*it)->toString() << "," << jt->whole << "," << Utilities::toString(*kt) << ")";
	}
	ret << "}";
	return ret.str();
}

bool ControlLogicActionStartDownload::operator==(const ControlLogicActionStartDownload& other) const
{
	if(tcpConnectionId != other.tcpConnectionId) {
		return false;
	}

	{
		list<const ContentId*>::const_iterator it = contentIds.begin();
		list<const ContentId*>::const_iterator jt = other.contentIds.begin();
		while(it != contentIds.end() && jt != other.contentIds.end()) {
			if(it == contentIds.end() ||  jt == other.contentIds.end()) {
				return false;
			}
			if(**it != **jt) {
				return false;
			}
			++it;
			++jt;
		}
		if(it != contentIds.end() || jt != other.contentIds.end()) {
			return false;
		}
	}

	{
		list<dashp2p::URL>::const_iterator it = urls.begin();
		list<dashp2p::URL>::const_iterator jt = other.urls.begin();
		while(it != urls.end() && jt != other.urls.end()) {
			if(it == urls.end() ||  jt == other.urls.end()) {
				return false;
			}
			if(*it != *jt) {
				return false;
			}
			++it;
			++jt;
		}
		if(it != urls.end() || jt != other.urls.end()) {
			return false;
		}
	}

	{
		list<HttpMethod>::const_iterator it = httpMethods.begin();
		list<HttpMethod>::const_iterator jt = other.httpMethods.begin();
		while(it != httpMethods.end() && jt != other.httpMethods.end()) {
			if(it == httpMethods.end() ||  jt == other.httpMethods.end()) {
				return false;
			}
			if(*it != *jt) {
				return false;
			}
			++it;
			++jt;
		}
		if(it != httpMethods.end() || jt != other.httpMethods.end()) {
			return false;
		}
	}

	return true;
}

}

/****************************************************************************
 * XmlAdapter.h                                                             *
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

#ifndef XMLADAPTER_H_
#define XMLADAPTER_H_


#include "mpd/model.h"

#if DP2P_VLC == 1
#include <vlc_common.h>
#endif


class XmlAdapter
{
public:
#if DP2P_VLC == 1
    static void init(vlc_object_t* dashp2pPluginObject) {dp2p_assert(dashp2pPluginObject && !XmlAdapter::dashp2pPluginObject); XmlAdapter::dashp2pPluginObject = dashp2pPluginObject;}
    static void cleanup() {dashp2pPluginObject = NULL;}
#endif

    /**
     * Parses the MPD.
     * @param buffer Will be deleted within this function.
     */
    static dash::mpd::MediaPresentationDescription* parseMpd(char* buffer, int bufferSize);

private:
    XmlAdapter(){}
    virtual ~XmlAdapter(){}

private:
#if DP2P_VLC == 1
    static vlc_object_t* dashp2pPluginObject;
#endif
};

#endif /* XMLADAPTER_H_ */

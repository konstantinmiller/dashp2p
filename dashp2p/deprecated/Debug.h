/****************************************************************************
 * Debug.h                                                                  *
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

#ifndef DEBUG_H_
#define DEBUG_H_

#include <vlc_common.h>

#include <string>

class vlc_object_t;

namespace dash {

#define dash_Debug(d, m) ((Debug*)d)->printf("%s(%d): %s", __FILE__, __LINE__, m);

class Debug
{
public:
    Debug(): p_vlc_object(NULL), idString(), ifDisabled(false) {}
    Debug( vlc_object_t* p_vlc_object, const std::string& idString = std::string(MODULE_STRING) );
    Debug( const Debug& dbg ): p_vlc_object(dbg.p_vlc_object), idString(dbg.idString), ifDisabled(false) {}
    Debug( const Debug& dbg, const std::string& idString ): p_vlc_object(dbg.p_vlc_object), idString(idString), ifDisabled(false) {}
    virtual ~Debug();
public:
    void printf(const char *str, ...);
    void printf(const char *str, va_list ap);
    void disable() {ifDisabled = true;}
private:
    vlc_object_t* p_vlc_object;
    const std::string idString;
    bool ifDisabled;
};
} // namespace dash

/* disables dashp2p */
//#define DASHP2P_DISABLE

/* outputs the data passed down the pipeline to the file debug_block_output.mp4 */
//#define DEBUG_BLOCK_OUTPUT

/* output downloaded segments */
//#define DEBUG_DOWNLOADED_SEGMENTS

//#define DEBUG_CURL
#define DEBUG_AdapterCURL
#define DEBUG_DASHP2PControl
#define DEBUG_DownloadManager


#endif

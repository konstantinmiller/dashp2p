/****************************************************************************
 * Debug.cpp                                                                *
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Debug.h"

dash::Debug::Debug(vlc_object_t* p_vlc_object, const std::string& idString)
  : p_vlc_object( p_vlc_object ),
    idString( idString ),
    ifDisabled(false)
{

}

dash::Debug::~Debug()
{

}

void dash::Debug::printf(const char *str, ...)
{
    if(!p_vlc_object || ifDisabled)
        return;

    va_list argList;
    va_start(argList, str);

    //vprintf(str, argList);
    msg_GenericVa( VLC_OBJECT(p_vlc_object), VLC_MSG_DBG, idString.c_str(), str, argList );

    va_end(argList);
}

void dash::Debug::printf(const char *str, va_list ap)
{
    if(!p_vlc_object || ifDisabled)
        return;

    msg_GenericVa( VLC_OBJECT(p_vlc_object), VLC_MSG_DBG, idString.c_str(), str, ap );
}

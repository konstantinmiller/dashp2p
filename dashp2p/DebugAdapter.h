/****************************************************************************
 * DebugAdapter.h                                                           *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Aug 13, 2012                                                 *
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

#ifndef DEBUGADAPTER_H_
#define DEBUGADAPTER_H_

#include <stdio.h>
#include "ThreadAdapter.h"

#if DP2P_VLC != 0
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
# include <vlc_common.h>
#endif

#define ERRMSG(...)  do { DebugAdapter::errPrintf (__VA_ARGS__); } while(false)
#define ERRMSGWT(...)  do { DebugAdapter::errPrintfWt (__VA_ARGS__); } while(false)
#define ERRMSGWL(...)  do { DebugAdapter::errPrintfWl (__FILE__, __func__, __LINE__, __VA_ARGS__); } while(false)

#define WARNMSG(...) do { DebugAdapter::warnPrintf(__VA_ARGS__); } while(false)
#define WARNMSGWT(...) do { DebugAdapter::warnPrintfWt(__VA_ARGS__); } while(false)
#define WARNMSGWL(...) do { DebugAdapter::warnPrintfWl(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(false)

#define INFOMSG(...) do { DebugAdapter::infoPrintf(__VA_ARGS__); } while(false)
#define INFOMSGWT(...) do { DebugAdapter::infoPrintfWt(__VA_ARGS__); } while(false)
#define INFOMSGWL(...) do { DebugAdapter::infoPrintfWl(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(false)

#define DBGMSG(...)  do { DebugAdapter::dbgPrintf (__FILE__, __func__, __LINE__, __VA_ARGS__); } while(false)

enum DebuggingLevel {
	DebuggingLevel_Quiet = 10,
	DebuggingLevel_Err   = 20,
	DebuggingLevel_Warn  = 30,
	DebuggingLevel_Info  = 40,
	DebuggingLevel_Dbg   = 50
};

class DebugAdapter
{
private:
    DebugAdapter(){}
    virtual ~DebugAdapter(){}

/* Public methods */
public:
#if DP2P_VLC != 0
    static void init(DebuggingLevel debuggingLevel, vlc_object_t* dashp2pPluginObject, const char* dbgFileName = NULL);
#else
    static void init(DebuggingLevel debuggingLevel, const char* dbgFileName = NULL);
#endif
    static void cleanUp();
    static void setDebugFile(const char* dbgFileName);
    static void errPrintf(const char* msg, ...);
    static void errPrintfWt(const char* msg, ...);
    static void errPrintfWl(const char* fileName, const char* functionName, int lineNr, ...);
    static void warnPrintf(const char* msg, ...);
    static void warnPrintfWt(const char* msg, ...);
    static void warnPrintfWl(const char* fileName, const char* functionName, int lineNr, ...);
    static void infoPrintf(const char* msg, ...);
    static void infoPrintfWt(const char* msg, ...);
    static void infoPrintfWl(const char* fileName, const char* functionName, int lineNr, ...);
    static void dbgPrintf(const char* fileName, const char* functionName, int lineNr, ...);

/* Private methods */
private:
    static void _errPrintfWl(const char* fileName, const char* functionName, int lineNr, ...);

/* Private members */
private:
    static bool initialized;
    static Mutex mutex;
    static const unsigned bufLen;
    static char* buf;
    static FILE* f;
    static DebuggingLevel debuggingLevel;
#if DP2P_VLC != 0
    static vlc_object_t* dashp2pPluginObject;
#endif
};

#endif /* DEBUGADAPTER_H_ */

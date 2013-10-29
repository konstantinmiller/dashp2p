/****************************************************************************
 * DebugAdapter.cpp                                                         *
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

//#include "Dashp2pTypes.h"
#include <cstdio>
#include <cstdarg>
#include <assert.h>
#include "DebugAdapter.h"
#include "ThreadAdapter.h"
#include "Utilities.h"
#ifdef __ANDROID__
# include <android/log.h>
#endif

namespace dashp2p {

bool DebugAdapter::initialized = false;
Mutex DebugAdapter::mutex;
const unsigned DebugAdapter::bufLen = 10 * 1048576;
char* DebugAdapter::buf = NULL;
FILE* DebugAdapter::f = NULL;
DebuggingLevel DebugAdapter::debuggingLevel = DebuggingLevel_Info;
vlc_object_t* DebugAdapter::dashp2pPluginObject = NULL;

/* interne macros that do not try to lock the mutex */
# define _dp2p_assert(x)               do {\
                                             if(!(x)) {\
                                                 DebugAdapter::_errPrintfWl (__FILE__, __func__, __LINE__, "assert(%s) failed", #x);\
                                                 abort();\
                                             }\
                                      } while(false)

# define _dp2p_assert_v(x, msg, ...)   do {\
	                                         if(!(x)) {\
                                                 DebugAdapter::_errPrintfWl (__FILE__, __func__, __LINE__, "assert(%s) failed", #x);\
                                                 DebugAdapter::_errPrintfWl (__FILE__, __func__, __LINE__, msg, __VA_ARGS__);\
                                                 abort();\
                                             }\
	                                  } while(false)


void DebugAdapter::init(DebuggingLevel debuggingLevel, vlc_object_t* dashp2pPluginObject, const char* dbgFileName)
//void DebugAdapter::init(DebuggingLevel debuggingLevel, const char* dbgFileName)
{
    assert(!initialized);

    DebugAdapter::debuggingLevel = debuggingLevel;

    if(buf != NULL)
        THROW_RUNTIME("Bug");
    ThreadAdapter::mutexInit(&mutex);

    buf = new char[bufLen];
    DebugAdapter::dashp2pPluginObject = dashp2pPluginObject;

    if(dbgFileName != NULL) {
        f = fopen(dbgFileName, "wx");
        if(!f)
            THROW_RUNTIME("Could not open file %s.", dbgFileName);
    }

    DebugAdapter::initialized = true;
}

void DebugAdapter::cleanUp()
{
	assert(initialized);

    ThreadAdapter::mutexLock(&mutex);
    if(f) {
        _dp2p_assert(0 == fclose(f));
        f = NULL;
    }

    _dp2p_assert(buf != NULL);
    delete [] buf;
    buf = NULL;

    initialized = false;

    ThreadAdapter::mutexUnlock(&mutex);
    ThreadAdapter::mutexDestroy(&mutex);
}

void DebugAdapter::setDebugFile(const char* dbgFileName)
{
	assert(initialized);

	ThreadAdapter::mutexLock(&mutex);
	_dp2p_assert(dbgFileName && !f);
	f = fopen(dbgFileName, "wx");
	_dp2p_assert_v(f, "Could not open file %s.", dbgFileName);
	ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::throwRuntime(const char* fileName, const char* functionName, int lineNr, ...)
{
	static char buf[2048];

	va_list argList;
	va_start(argList, lineNr);
	const char* message = va_arg(argList, char*);
	int cnt = std::sprintf(buf, "ERROR in %s::%s() (%d): ", fileName, functionName, lineNr);
	cnt += std::vsprintf(buf + cnt, message, argList);
	cnt += std::sprintf(buf + cnt, "\n");
	va_end(argList);

    throw std::runtime_error(buf);
}

void DebugAdapter::errPrintf(const char* msg, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Err)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, msg);
        fprintf(f, "ERROR: ");
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_ERR, "DASHP2P", msg, argList);
    //printf("ERROR: ");
    //vprintf(msg, argList);
    //printf("\n");
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::errPrintfWt(const char* msg, ...)
{
    assert(initialized);

    if(debuggingLevel < DebuggingLevel_Err)
        return;

    ThreadAdapter::mutexLock(&mutex);

    const string timeString = dashp2p::Utilities::getTimeString(0, false);

    if(f) {
        va_list argList;
        va_start(argList, msg);
        fprintf(f, "[%s] ERROR: ", timeString.c_str());
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_ERR, "DASHP2P", msg, argList);
    //printf("[%s] ERROR: ", timeString.c_str());
    //vprintf(msg, argList);
    //printf("\n");
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::errPrintfWl(const char* fileName, const char* functionName, int lineNr, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Err)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, lineNr);
    	const char* message = va_arg(argList, char*);
    	fprintf(f, "ERROR in %s::%s() (%d): ", fileName, functionName, lineNr);
    	vfprintf(f, message, argList);
    	fprintf(f, "\n");
    	//fflush(f);
    	va_end(argList);
    }

    va_list argList;
    va_start(argList, lineNr);
    const char* message = va_arg(argList, char*);
//#if DP2P_VLC != 0
    assert(strlen(message) < bufLen);
    sprintf(buf, "ERROR in %s::%s() (%d): %s", fileName, functionName, lineNr, message);
//#  ifdef __ANDROID__
//    __android_log_vprint(ANDROID_LOG_ERROR, "VLC/dashp2p", buf, argList);
//#  else
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_ERR, "DASHP2P", buf, argList);
//#  endif
//#else
//    printf("ERROR in %s::%s() (%d): ", fileName, functionName, lineNr);
//    vprintf(message, argList);
//    printf("\n");
//    //fflush(stdout);
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::warnPrintf(const char* msg, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Warn)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, msg);
        fprintf(f, "WARNING: ");
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
//#if DP2P_VLC != 0
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_WARN, "DASHP2P", msg, argList);
//#else
//    printf("WARNING: ");
//    vprintf(msg, argList);
//    printf("\n");
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::warnPrintfWt(const char* msg, ...)
{
    assert(initialized);

    if(debuggingLevel < DebuggingLevel_Warn)
        return;

    ThreadAdapter::mutexLock(&mutex);

    const string timeString = dashp2p::Utilities::getTimeString(0, false);

    if(f) {
        va_list argList;
        va_start(argList, msg);
        fprintf(f, "[%s] WARNING: ", timeString.c_str());
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
//#if DP2P_VLC != 0
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_WARN, "DASHP2P", msg, argList);
//#else
//    printf("[%s] WARNING: ", timeString.c_str());
//    vprintf(msg, argList);
//    printf("\n");
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::warnPrintfWl(const char* fileName, const char* functionName, int lineNr, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Warn)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, lineNr);
    	const char* message = va_arg(argList, char*);
        fprintf(f, "WARNING in %s::%s() (%d): ", fileName, functionName, lineNr);
        vfprintf(f, message, argList);
        fprintf(f, "\n");
        //fflush(f);
        va_end(argList);
    }

    va_list argList;
    va_start(argList, lineNr);
    const char* message = va_arg(argList, char*);
//#if DP2P_VLC != 0
    _dp2p_assert(strlen(message) < bufLen);
   	sprintf(buf, "WARNING in %s::%s() (%d): %s", fileName, functionName, lineNr, message);
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_WARN, "DASHP2P", buf, argList);
//#else
//    printf("WARNING in %s::%s() (%d): ", fileName, functionName, lineNr);
//    vprintf(message, argList);
//    printf("\n");
//    //fflush(stdout);
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::infoPrintf(const char* msg, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Info)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, msg);
        fprintf(f, "INFO: ");
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
//#if DP2P_VLC != 0
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_INFO, "DASHP2P", msg, argList);
//#else
//    printf("INFO: ");
//    vprintf(msg, argList);
//    printf("\n");
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::infoPrintfWt(const char* msg, ...)
{
    assert(initialized);

    if(debuggingLevel < DebuggingLevel_Info)
        return;

    ThreadAdapter::mutexLock(&mutex);

    const string timeString = dashp2p::Utilities::getTimeString(0, false);

    if(f) {
        va_list argList;
        va_start(argList, msg);
        fprintf(f, "[%s] INFO: ", timeString.c_str());
        vfprintf(f, msg, argList);
        fprintf(f, "\n");
        va_end(argList);
    }

    va_list argList;
    va_start(argList, msg);
//#if DP2P_VLC != 0
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_INFO, "DASHP2P", msg, argList);
//#else
//    printf("[%s] INFO: ", timeString.c_str());
//    vprintf(msg, argList);
//    printf("\n");
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::infoPrintfWl(const char* fileName, const char* functionName, int lineNr, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Info)
		return;

    ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, lineNr);
    	const char* message = va_arg(argList, char*);
        fprintf(f, "INFO in %s::%s() (%d): ", fileName, functionName, lineNr);
        vfprintf(f, message, argList);
        fprintf(f, "\n");
        //fflush(f);
        va_end(argList);
    }

    va_list argList;
    va_start(argList, lineNr);
    const char* message = va_arg(argList, char*);
//#if DP2P_VLC != 0
    _dp2p_assert(strlen(message) < bufLen);
    sprintf(buf, "INFO in %s::%s() (%d): %s", fileName, functionName, lineNr, message);
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_INFO, "DASHP2P", buf, argList);
//#else
//    printf("INFO in %s::%s() (%d): ", fileName, functionName, lineNr);
//    vprintf(message, argList);
//    printf("\n");
//    //fflush(stdout);
//#endif
    va_end(argList);

    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::dbgPrintf(const char* fileName, const char* functionName, int lineNr, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Dbg)
		return;

    ThreadAdapter::mutexLock(&mutex);

    const int64_t absNow = dashp2p::Utilities::getTime();

    if(f)
    {
    	va_list argList;
    	va_start(argList, lineNr);
    	const char* message = va_arg(argList, char*);
        fprintf(f, "[%11.6f] %s::%s() (%d): ", absNow / 1e6, fileName, functionName, lineNr);
        vfprintf(f, message, argList);
        fprintf(f, "\n");
        //fflush(f);
        va_end(argList);
    }

    va_list argList;
    va_start(argList, lineNr);
    const char* message = va_arg(argList, char*);
//#if DP2P_VLC != 0
    _dp2p_assert(strlen(message) < bufLen);
    sprintf(buf, "%s::%s() (%d): %s", fileName, functionName, lineNr, message);
    static char tmpBuf[64];
    sprintf(tmpBuf, "[%11.6f] DASHP2P", absNow / 1e6);
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_DBG, tmpBuf, buf, argList);
//#else
//    printf("[%11.4f] %s::%s() (%d): ", absNow / 1e6, fileName, functionName, lineNr);
//    vprintf(message, argList);
//    printf("\n");
//    //fflush(stdout);
//#endif
    va_end(argList);


    ThreadAdapter::mutexUnlock(&mutex);
}

void DebugAdapter::_errPrintfWl(const char* fileName, const char* functionName, int lineNr, ...)
{
	assert(initialized);

	if(debuggingLevel < DebuggingLevel_Err)
		return;

    //ThreadAdapter::mutexLock(&mutex);

    if(f) {
    	va_list argList;
    	va_start(argList, lineNr);
    	const char* message = va_arg(argList, char*);
        fprintf(f, "ERROR in %s::%s() (%d): ", fileName, functionName, lineNr);
        vfprintf(f, message, argList);
        fprintf(f, "\n");
        //fflush(f);
        va_end(argList);
    }

    va_list argList;
    va_start(argList, lineNr);
    const char* message = va_arg(argList, char*);
//#if DP2P_VLC != 0
    assert(strlen(message) < bufLen);
    sprintf(buf, "ERROR in %s::%s() (%d): %s", fileName, functionName, lineNr, message);
//#  ifdef __ANDROID__
//    __android_log_vprint(ANDROID_LOG_ERROR, "VLC/dashp2p", buf, argList);
//#  else
    msg_GenericVa( dashp2pPluginObject, VLC_MSG_ERR, "DASHP2P", buf, argList);
//#  endif
//#else
//    printf("ERROR in %s::%s() (%d): ", fileName, functionName, lineNr);
//    vprintf(message, argList);
//    printf("\n");
//    //fflush(stdout);
//#endif
    va_end(argList);

    //ThreadAdapter::mutexUnlock(&mutex);
}

}

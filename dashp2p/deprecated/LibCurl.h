/****************************************************************************
 * LibCurl.h                                                                *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 6, 2012                                                  *
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

#ifndef LIBCURL_H_
#define LIBCURL_H_


#include "curl/curl.h"
#include <string>
#include <list>

/* If not defined, LibCurl won't produce any debug output. */
// #define LIBCURL_DBG
/* If not defined, libcurl won't produce any debug output. */
// #define LIBCURL_libcurl_DBG
/* Define, if LibCurl debug output should go to dbg_libcurl.txt (instead of stdout). */
// #define LIBCURL_DBG_FILE


class LibCurl
{
private:
    LibCurl(){}
    virtual ~LibCurl(){}

public:
    /**
     * Type for the transfer state.
     */
    enum LibCurlState {LibCurlState_Pre, LibCurlState_Idle, LibCurlState_WaitingForHeader, LibCurlState_ReceivingHeader, LibCurlState_WaitingForData, LibCurlState_ReceivingData, LibCurlState_WaitingForCompletion};

    /**
     * Synchronous download of GET requests.
     * Upon return, @buffer contains the pointer to the data that must be freed by the caller after usage.
     * Returns 0 if request completed, otherwise 1.
     */
    static int syncGET(const std::string& URL, bool ifWaitForCompletion, char** buffer, int* bufferedBytes, int* requestSize);

    /**
     * Initialize LibCurl
     */
    static void init(const std::string& logDir);

    /**
     * Cleanup.
     */
    static void cleanUp();

private:
    /**
     * Handle to the libcurl "multi" interface.
     */
    static CURLM* curlm;

    /**
     * Handle to the libcurl "easy" interface.
     */
    static CURL* curle;

    /**
     * State of the ongoing transfer.
     */
    static LibCurlState state;

    /**
     * Size of the content.
     */
    static unsigned contentSize;

    /**
     * Indicates how much data we still expect from the pending transfer.
     */
    static unsigned expectedBytes;

    /**
     * Callback for libcurl for incoming data.
     */
    static size_t cbData(void *buffer, size_t size, size_t nmemb, void *userp);

    /**
     * Callback for libcurl for incoming header.
     */
    static size_t cbHeader(void *buffer, size_t size, size_t nmemb, void *userp);

    /**
     * Callback for libcurl for debug outputl
     */
    static int cbDebug(CURL* curle, curl_infotype infoType, char* str, size_t size, void* params);

    /**
     * Waits for action.
     */
    static void wait();

    /**
     * List of memory buffers with incoming data.
     */
    static std::list<std::pair<char*,int> > incomingData;

    /**
     * Return buffered bytes in a newly allocated memory block and clear internal storage.
     */
    static void getBufferedBytes(char** buffer, int* bufferedBytes);

    /**
     * File to log download progress.
     */
    static FILE* fileDownloadProgress;

    /**
     * Counter for downloaded bytes.
     */
    static int64_t totalDownloadedBytes;

    /**
     * Debug output.
     */
    static void dbgPrintf(const char* str, ...);
};

#endif /* LIBCURL_H_ */

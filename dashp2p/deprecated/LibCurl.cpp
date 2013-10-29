/****************************************************************************
 * LibCurl.cpp                                                              *
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

#include "dashp2p_simplest.h"
#include "Utilities.h"
#include "LibCurl.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>


CURLM* LibCurl::curlm = NULL;
CURL* LibCurl::curle = NULL;
LibCurl::LibCurlState LibCurl::state = LibCurlState_Pre;
unsigned LibCurl::contentSize = 0;
unsigned LibCurl::expectedBytes = 0;
std::list<std::pair<char*,int> > LibCurl::incomingData;
FILE* LibCurl::fileDownloadProgress = NULL;
int64_t LibCurl::totalDownloadedBytes = 0;


int LibCurl::syncGET(const std::string& URL, bool ifWaitForCompletion, char** buffer, int* bufferedBytes, int* requestSize)
{
    /* Initialize, if not yet done. */
    if(state == LibCurlState_Pre) {
        init(".");
    }

    /* Do we want to start a new request or continue an ongoing one? */
    if(!URL.empty())
    {
        /* debug output */
        dbgPrintf("\n");
        dbgPrintf("-> -> -> GET (%s) %s", ifWaitForCompletion?"sync":"async", URL.c_str());

        /* sanity check */
        dp2p_assert(state == LibCurlState_Idle);

        /* Set the URL */
        CURLcode res = curl_easy_setopt(curle, CURLOPT_URL, URL.c_str());
        dp2p_assert( res == CURLE_OK );

        /* Add the curl easy handle to the multi handle */
        //dbgPrintf("Adding easy to multi.");
        CURLMcode mret = curl_multi_add_handle( curlm, curle );
        dp2p_assert( mret == CURLM_OK );

        /* Update state. */
        state = LibCurlState_WaitingForHeader;

        /* Updating internal variables. */
        contentSize = 0;
    }
    else
    {
        /* sanity check */
        dp2p_assert(state != LibCurlState_Idle);
        //dbgPrintf("\nAsked for more data for ongoing request.");
    }

    /* Loop */
    while(true)
    {
        //dbgPrintf("looping...");

        LibCurl::wait();

        //dbgPrintf("Select returned. Calling curl_multi_perform.");

        /* now call curl_multi_perform */
        int runningHandles = 1;
        CURLMcode mret = curl_multi_perform( curlm, &runningHandles );
        dp2p_assert( mret == CURLM_OK );

        /* Did the transfer complete? */
        if(runningHandles == 0)
        {
            /* Get state messages. */
            int numMsgLeft = 0;
            CURLMsg* msg = curl_multi_info_read( curlm, &numMsgLeft );
            /* Sanity checks */
            dp2p_assert(expectedBytes == 0 && msg && !numMsgLeft && msg->msg == CURLMSG_DONE && msg->easy_handle == curle && msg->data.result == CURLE_OK);
            /* remove the CURL handle from the multi-stack */
            mret = curl_multi_remove_handle( curlm, curle);
            dp2p_assert( mret == CURLM_OK );
            /* check the response status */
            long responseCode = -1;
            CURLcode res = curl_easy_getinfo(curle, CURLINFO_RESPONSE_CODE, &responseCode);
            dp2p_assert(res == CURLE_OK && responseCode == 200);
            /* Allocate memory and copy buffered bytes to the outgoing buffer */
            getBufferedBytes(buffer, bufferedBytes);
            /* Update state. */
            state = LibCurlState_Idle;
            /* Prepare return values. */
            requestSize[0] = contentSize;
            dbgPrintf("Returning %d (%d) bytes to caller.", bufferedBytes[0], requestSize[0]);
            return 0;
        }
        else if(!ifWaitForCompletion && !incomingData.empty())
        {
            /* Allocate memory and copy buffered bytes to the outgoing buffer */
            getBufferedBytes(buffer, bufferedBytes);
            requestSize[0] = contentSize;
            dbgPrintf("Returning %d (%d) bytes to caller.", bufferedBytes[0], requestSize[0]);
            return 1;
        }
        else
        {
            /* blank */
            //dbgPrintf("curl_multi_performed returned. No data received. Continue...");
        }
    }

    /* Error condition but want to make compiler happy by returning something. */
    dp2p_assert(false);
    return 0;
}

void LibCurl::init(const std::string& logDir)
{
    /* Debug output */
    dbgPrintf("Initializing libcurl.");

    /* Sanity checks */
    dp2p_assert(state == LibCurlState_Pre && !curle && !curlm);

    /* logging */
    {
        char path[1024];
        sprintf(path, "%s/log_%"PRId64"_bytes_downloaded.txt", logDir.c_str(), dash::Utilities::getReferenceTime());
        fileDownloadProgress = fopen(path, "w");
        dp2p_assert(fileDownloadProgress);
        fprintf(fileDownloadProgress, "% 17.6f % 17.6f\n", dash::Utilities::getTime() / 1e6, totalDownloadedBytes / 1e6);
    }

    /* Call global init function */
    // TODO: clarify thread safety
    // CURLcode ret = curl_global_init( CURL_GLOBAL_ALL );
    // dp2p_assert( ret == CURLE_OK );

    /* create and initialize a new CURL easy handle */
    curle = curl_easy_init();
    dp2p_assert( curle );

    /* Set the callback for data */
    CURLcode res = curl_easy_setopt(curle, CURLOPT_WRITEFUNCTION, LibCurl::cbData);
    dp2p_assert(res == CURLE_OK);

    /* Set the data to be passed to the callback for data */
    res = curl_easy_setopt(curle, CURLOPT_WRITEDATA, NULL);
    dp2p_assert( res == CURLE_OK );

    /* set various CURL options */
#ifdef LIBCURL_libcurl_DBG
    res = curl_easy_setopt(curle, CURLOPT_VERBOSE, 1);
#else
    res = curl_easy_setopt(curle, CURLOPT_VERBOSE, 0);
#endif
    dp2p_assert(res == CURLE_OK);
    res = curl_easy_setopt(curle, CURLOPT_DEBUGFUNCTION, LibCurl::cbDebug);
    dp2p_assert(res == CURLE_OK);
    //res = curl_easy_setopt(curle, CURLOPT_DEBUGDATA, NULL);
    //dp2p_assert(res == CURLE_OK);
    res = curl_easy_setopt(curle, CURLOPT_NOPROGRESS, 1);
    dp2p_assert(res == CURLE_OK);
    res = curl_easy_setopt(curle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP );
    dp2p_assert(res == CURLE_OK);
    res = curl_easy_setopt(curle, CURLOPT_HEADERFUNCTION, cbHeader);
    dp2p_assert(res == CURLE_OK);
    res = curl_easy_setopt( curle, CURLOPT_HEADERDATA, NULL );
    dp2p_assert( res == CURLE_OK );

    /* create and initialize a new CURL multi handle */
    curlm = curl_multi_init();
    dp2p_assert( curlm );

    /* Update state. */
    state = LibCurlState_Idle;
}


void LibCurl::cleanUp()
{
    // Clean-up the easy handle. */
    if(curle) {
        /* Sanity check. */
        dp2p_assert(curlm);
        /* Try to remove from the multi handle first. */
        curl_multi_remove_handle(curlm, curle);
        /* Clean-up. */
        curl_easy_cleanup(curle);
        curle = NULL;
    }

    /* Clean-up the multi handle. */
    if(curlm) {
        curl_multi_cleanup(curlm);
    }

    /* Do libcurl global cleanup. */
    curl_global_cleanup();

    /* Clean-up internal storage. */
    while(!incomingData.empty())
    {
        delete [] incomingData.begin()->first;
        incomingData.pop_front();
    }

    /* Close log files. */
    if(fileDownloadProgress) {
        fclose(fileDownloadProgress);
    }
}

size_t LibCurl::cbData(void* buffer, size_t size, size_t nmemb, void* /*userp*/)
{
    /* Sanity checks. */
    dp2p_assert(state == LibCurlState_WaitingForData || state == LibCurlState_ReceivingData);
    if(size * nmemb > expectedBytes) {
        printf("size * nmemb = %u * %u > %u = expectedBytes\n", size, nmemb, expectedBytes); fflush(stdout);
        dp2p_assert(false);
    }
    /* Output */
    if(state == LibCurlState_WaitingForData) {
        dbgPrintf("Starting to receive data.");
    }
    /* Update state. */
    state = LibCurlState_ReceivingData;
    /* Get and store the data. */
    char* buf = new char[size * nmemb];
    memcpy(buf, buffer, size * nmemb);
    incomingData.push_back(std::pair<char*,int>(buf, size * nmemb));
    /* Update counter. */
    expectedBytes -= size * nmemb;
    /* Debug output and logging. */
    totalDownloadedBytes += size * nmemb;
    fprintf(fileDownloadProgress, "% 17.6f % 17.6f\n", dash::Utilities::getTime() / 1e6, totalDownloadedBytes / 1e6);
    dbgPrintf("Received %u (%u / %u) bytes of the request.", size * nmemb, contentSize - expectedBytes, contentSize);
    /* return */
    return size * nmemb;
}

size_t LibCurl::cbHeader(void* buffer, size_t size, size_t nmemb, void* /*userp*/)
{
    static int headerBytesReceived = 0;
    /* Sanity checks. */
    dp2p_assert(state == LibCurlState_WaitingForHeader || state == LibCurlState_ReceivingHeader);
    if(state == LibCurlState_WaitingForHeader) {
        dbgPrintf("Starting to receive header.");
        headerBytesReceived = 0;
    }
    /* Update state and internal variables. */
    state = LibCurlState_ReceivingHeader;
    headerBytesReceived += size * nmemb;
    /* Parse the header. */
    if(sscanf((char*)buffer, "Content-Length: %u", &expectedBytes)) {
        contentSize = expectedBytes;
        dbgPrintf("Content-Length: %u", expectedBytes);
    } else if(*(char*)buffer == '\r' && *((char*)buffer + 1) == '\n') {
        state = LibCurlState_WaitingForData;
        dbgPrintf("Header received completely. %d bytes in total.", headerBytesReceived);
    }
    /* logging */
    totalDownloadedBytes += size * nmemb;
    fprintf(fileDownloadProgress, "% 17.6f % 17.6f\n", dash::Utilities::getTime() / 1e6, totalDownloadedBytes / 1e6);
    /* return */
    return size * nmemb;
}

int LibCurl::cbDebug(CURL* /*curle*/, curl_infotype /*infoType*/, char* str, size_t size, void* /*params*/)
{
    if(str[size - 1] == '\n') {
        dbgPrintf("libcurl: %.*s", size - 1, str);
    } else {
        dbgPrintf("libcurl: %.*s", size, str);
    }
    return 0;
}

void LibCurl::wait()
{
    //dbgPrintf("Calling curl_multi_fdset");

    /* Find out, on which file descriptors libcurl wants us to wait */
    fd_set rfds, wfds, efds;
    FD_ZERO( &rfds );
    FD_ZERO( &wfds );
    FD_ZERO( &efds );
    int max_fd = -1;
    CURLMcode mret = curl_multi_fdset( curlm, &rfds, &wfds, &efds, &max_fd);
    dp2p_assert( mret == CURLM_OK );
    if( max_fd == -1 )
    {
        /* wait 100 ms. why? see the libcurl documentation for the curl_multi_fdset function */
        // TODO: tweak the waiting time.
        //const int wait_usec = 100000;
        //dbgPrintf("curl_multi_fdset returned -1. will wait %f sec", wait_usec / 1e6);
        //dp2p_assert( !usleep(wait_usec) );
        // TODO: use VLC function msleep(long long usec) instead of system function usleep
        //dbgPrintf("Returned -1, no select, re-queue and continue.");
        pthread_yield();
    }
    else
    {
        /* how long shall we wait at longest? */
        long to_msec = 0;
        mret = curl_multi_timeout( curlm, &to_msec);
        dp2p_assert( mret == CURLM_OK );
        struct timeval to;
        if( to_msec == -1 ) {
            //dbgPrintf("curl_multi_timeout returned -1" );
            to.tv_sec = 2;
            to.tv_usec = 0;
        } else {
            //dbgPrintf("curl_multi_timeout returned %d msec", to_msec );
            to.tv_sec = 0;
            to.tv_usec = 1000 * to_msec;
        }

        //dbgPrintf("Returned timeout for select: % .3f sec. Calling select.", to_msec / 1e3 );

        /* call select to wait for I/O */
        int ret = select( max_fd + 1, &rfds, &wfds, &efds, &to );
        dp2p_assert( ret >= 0 );
    }
}

void LibCurl::getBufferedBytes(char** buffer, int* bufferedBytes)
{
    // TODO: if we allow acynchronous calls to LibCurl, this function must be protected by mutexes!
    /* sanity checks */
    dp2p_assert(buffer[0] == NULL && bufferedBytes[0] == 0);
    /* If buffer empty, return immediately. */
    if(incomingData.empty()) {
        return;
    }
    /* Calculate the number of buffered bytes. */
    for(std::list<std::pair<char*,int> >::iterator it = incomingData.begin(); it != incomingData.end(); ++it) {
        bufferedBytes[0] += it->second;
    }
    /* sanity checks */
    dp2p_assert(bufferedBytes[0] != 0);
    /* Allocate new memory. */
    buffer[0] = new char[bufferedBytes[0]];
    /* Copy data and clear internal storage. */
    int cnt = 0;
    while(!incomingData.empty())
    {
        memcpy(buffer[0] + cnt, incomingData.begin()->first, incomingData.begin()->second);
        cnt += incomingData.begin()->second;
        delete [] incomingData.begin()->first;
        incomingData.pop_front();
    }
}

void LibCurl::dbgPrintf(const char* str, ...)
{
#ifndef LIBCURL_DBG
    return;
#endif

    va_list argList;
    va_start(argList, str);

    //vprintf(str, argList);
    //::dbgPrintf("LibCurl", str, argList );

#if LIBCURL_DBG_FILE
    static FILE* f = fopen("dbg_libcurl.txt", "w");
#endif
    if(str[0] != '\n') {
#if LIBCURL_DBG_FILE
        fprintf(f, "% 9.3f: ", dash::Utilities::getTime() / 1e6);
#else
        printf("% 9.3f: ", dash::Utilities::getTime() / 1e6);
#endif
    }
#if LIBCURL_DBG_FILE
    vfprintf(f, str, argList);
    fprintf(f, "\n");
#else
    vprintf(str, argList);
    printf("\n");
#endif

    va_end(argList);
}

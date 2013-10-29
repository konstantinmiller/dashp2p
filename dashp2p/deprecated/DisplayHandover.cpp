/****************************************************************************
 * DisplayHandover.cpp                                                      *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Aug 17, 2012                                                 *
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

#define __STDC_FORMAT_MACROS

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "DisplayHandover.h"
#include "Control.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <inttypes.h>
#include <string>


bool DisplayHandover::withHandover = false;
int DisplayHandover::portDisplayHandover = 0;
bool DisplayHandover::ifTerminating = false;
Thread DisplayHandover::thread;
int DisplayHandover::fdSocket = -1;

// TODO: implement start scanning command

void DisplayHandover::init(int portDisplayHandover, bool withHandover)
{
    DisplayHandover::withHandover = withHandover;

    DisplayHandover::portDisplayHandover = portDisplayHandover;

    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    dp2p_assert(fdSocket != -1);

    /* Resolve the address. */
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char portString[1024];
    sprintf(portString, "%d", portDisplayHandover);
    dp2p_assert(0 == getaddrinfo("127.0.0.1", portString, &hints, &result) && result);

    /* Connect. */
    int retVal = connect(fdSocket, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    if(retVal == 0)
    {
        INFOMSG("Operating with DisplayHandover module.");

        /* Start the thread. */
        dp2p_assert(0 == ThreadAdapter::startThread(&thread, threadEntry, NULL));
    }
    else
    {
        fdSocket = -1;
        INFOMSG("Operating without DisplayHandover module.");
    }
}

void DisplayHandover::cleanup()
{
    ifTerminating = true;

    if(fdSocket == -1)
        return;

    ThreadAdapter::joinThread(thread);

    playbackTerminated();

    dp2p_assert(0 == shutdown(fdSocket, SHUT_RDWR));
    dp2p_assert(0 == close(fdSocket));
    fdSocket = -1;
}

void DisplayHandover::playbackStarted()
{
    if(!withHandover || fdSocket == -1)
        return;

    int retVal = send(fdSocket, "START\n", strlen("START\n"), 0);
    dp2p_assert(retVal == (int)strlen("START\n"));
}

void DisplayHandover::playbackTerminated()
{
    if(!withHandover || fdSocket == -1)
        return;

    int retVal = send(fdSocket, "TERMINATING\n", strlen("TERMINATING\n"), 0);
    dp2p_assert(retVal == (int)strlen("TERMINATING\n"));
}

void DisplayHandover::displayOverlay(int line, const char* format, ...)
{
    if(withHandover || fdSocket == -1)
        return;

    static bool init = false;
    static char bufLine[4][256];
    static char buf[2048];
    static dash::Usec lastDisplayTime = 0;

    if(!init) {
        init = true;
        buf[0] = 0;
        for(int i = 0; i < 4; ++i)
            bufLine[i][0] = 0;
    }

    dp2p_assert(line < 4);

    va_list argList;
    va_start(argList, format);
    vsprintf(bufLine[line], format, argList);
    va_end(argList);

    dash::Usec now = dash::Utilities::getTime();
    if(now / 1000000 > lastDisplayTime / 1000000) {
        sprintf(buf, "DISPLAY=%s\r%s\r%s\r%s\n", bufLine[0], bufLine[1], bufLine[2], bufLine[3]);
        int retVal = send(fdSocket, buf, strlen(buf), 0);
        dp2p_assert(retVal == (int)strlen(buf));
        lastDisplayTime = now;
    }
}

void* DisplayHandover::threadEntry(void* /*params*/)
{
    DBGMSG("[%.3fs] DisplayHandover thread gestartet.", dash::Utilities::now());

    const int recvBufSize = 8192;
    char recvBuf[recvBufSize];
    int bytesReceived = 0;
    while(!ifTerminating)
    {
        fd_set fdSetRead;
        FD_ZERO(&fdSetRead);
        FD_SET(fdSocket, &fdSetRead);
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        DBGMSG("[%.3fs] Will wait for data to read.", dash::Utilities::now());
        int selectRetVal = select(fdSocket + 1, &fdSetRead, NULL, NULL, &timeout);
        if(selectRetVal > 0) { /* We can read from the socket. */
            dp2p_assert(withHandover);
            dp2p_assert(selectRetVal == 1 && FD_ISSET(fdSocket, &fdSetRead));
        } else if(selectRetVal < 0) { /* Error. */
            perror("select()");
            dp2p_assert(0);
        } else { /* select() timeout. */
            DBGMSG("[%.3fs] Timeout.", dash::Utilities::now());
            continue;
        }

        /* Read from the socket */
        DBGMSG("[%.3fs] Data available. Will read from the socket.", dash::Utilities::now());
        ssize_t numBytesReceived = recv(fdSocket, recvBuf + bytesReceived, recvBufSize - bytesReceived, 0);
        if(numBytesReceived == -1) {
            perror("recv()");
            dp2p_assert(0);
        } else if(numBytesReceived == 0) {
            printf("connection closed\n");
            dp2p_assert(0);
        }

        recvBuf[bytesReceived + numBytesReceived] = 0;

        DBGMSG("Received %d bytes, %d bytes in total. Received: [%s]", numBytesReceived, numBytesReceived + bytesReceived, recvBuf);

        bytesReceived += numBytesReceived;

        if(recvBuf[bytesReceived - 1] != '\n') {
            DBGMSG("Command partially received. Will wait for the rest.");
            continue;
        }

        if(0 == strcmp(recvBuf, "GET_POSITION\n"))
        {
            bytesReceived = 0;
            recvBuf[0] = 0;

            const dash::URL& splittedMpdUrl = Control::getMpdUrl();
            const uint64_t position = Control::getPosition(); // in [s]
            const std::vector<dash::Usec> switchingPoints = Control::getSwitchingPoints(5);
            char sendBuf[8192];
            int cnt = sprintf(sendBuf, "GET_POSITION=%s,%"PRIu64",", splittedMpdUrl.whole.c_str(), position);
            for(unsigned i = 0; i < switchingPoints.size() - 1; ++i)
                cnt += sprintf(sendBuf + cnt, "%"PRIu64",", switchingPoints.at(i));
            sprintf(sendBuf + cnt, "%"PRIu64"\n", switchingPoints.back());
            DBGMSG("%s", sendBuf);

            /* Send back. */
            int retVal = send(fdSocket, sendBuf, strlen(sendBuf), 0);
            dp2p_assert(retVal >= 0 && retVal == (int)strlen(sendBuf));
            continue;
        }

        uint64_t startPosition = 0;
        uint64_t startTime = 0;
        char newMPDURL[2048];
        if(3 == sscanf(recvBuf, "SET_POSITION=%[^,],%"SCNu64",%"SCNu64"\n", newMPDURL, &startPosition, &startTime))
        {
            bytesReceived = 0;
            recvBuf[0] = 0;

            /* Send back. */
            char sendBuf[8192];
            sprintf(sendBuf, "SET_POSITION=OK\n");
            int retVal = send(fdSocket, sendBuf, strlen(sendBuf), 0);
            dp2p_assert(retVal >= 0 && retVal == (int)strlen(sendBuf));

            Control::receiveDisplayHandover(std::string(newMPDURL), startPosition, startTime);

            continue;
        }

        dash::Usec stopTime = -1;
        if(1 == sscanf(recvBuf, "STOP %"SCNd64"\n", &stopTime))
        {
            DBGMSG("Received STOP from handover controller.");

            bytesReceived = 0;
            recvBuf[0] = 0;

            Control::prepareToPause();
            continue;
        }

        dp2p_assert(0);
    }

    return NULL;
}

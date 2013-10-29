/****************************************************************************
 * OverlayAdapter.cpp                                                       *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Sep 17, 2012                                                 *
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


#include "OverlayAdapter.h"
#include "DebugAdapter.h"
#include "Utilities.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <cstdarg>
#include <algorithm>

#ifndef __ANDROID__
# include <sys/ipc.h>
# include <sys/shm.h>
#endif

namespace dashp2p {

Thread OverlayAdapter::thread;
bool OverlayAdapter::terminating = false;
bool OverlayAdapter::initialized = false;
string OverlayAdapter::input;
string OverlayAdapter::output;
const int OverlayAdapter::numOverlays = 1;
std::vector<char*> OverlayAdapter::shms = std::vector<char*>(numOverlays, (char*)NULL);
std::vector<int> OverlayAdapter::shmIds = std::vector<int>(numOverlays, -1);
std::vector<int> OverlayAdapter::overlayIds = std::vector<int>(numOverlays, -1);
char OverlayAdapter::ackBuf[2048];
FILE* OverlayAdapter::fCmd = NULL;
FILE* OverlayAdapter::fAck = NULL;
Mutex OverlayAdapter::mutex;
CondVar OverlayAdapter::condVar;
const int OverlayAdapter::msgBufSize = 2048;
const int OverlayAdapter::numLines = 5;
std::vector<bool> OverlayAdapter::dirty = std::vector<bool>(numLines, false);
std::vector<char*>* OverlayAdapter::msgBuf1 = NULL;
std::vector<char*>* OverlayAdapter::msgBuf2 = NULL;


void OverlayAdapter::init(const string& input, const string& output)
{
#ifdef __ANDROID__
    return;
#else
    //OverlayAdapter::thread;
    OverlayAdapter::terminating = false;
    OverlayAdapter::initialized = false;
    OverlayAdapter::input = input;
    OverlayAdapter::output = output;
    OverlayAdapter::shms = std::vector<char*>(numOverlays, (char*)NULL);
    OverlayAdapter::shmIds = std::vector<int>(numOverlays, -1);
    OverlayAdapter::overlayIds = std::vector<int>(numOverlays, -1);
    //OverlayAdapter::ackBuf[2048];
    OverlayAdapter::fCmd = NULL;
    OverlayAdapter::fAck = NULL;
    //OverlayAdapter::mutex;
    //OverlayAdapter::condVar;
    //OverlayAdapter::msgBufSize = 2048;
    //const int OverlayAdapter::numLines = 3;
    OverlayAdapter::dirty = std::vector<bool>(numLines, false);

    /* Buffers */
    OverlayAdapter::msgBuf1 = new std::vector<char*>(numLines, (char*)NULL);
    for(unsigned i = 0; i < msgBuf1->size(); ++i) {
        msgBuf1->at(i) = new char[msgBufSize];
        strcpy(msgBuf1->at(i), "|");
    }
    OverlayAdapter::msgBuf2 = new std::vector<char*>(numLines, (char*)NULL);
    for(unsigned i = 0; i < msgBuf2->size(); ++i) {
        msgBuf2->at(i) = new char[msgBufSize];
        strcpy(msgBuf2->at(i), "|");
    }

    /* Create shared memory and attach to it */
    for(unsigned i = 0; i < overlayIds.size(); ++i)
    {
        /* Creating shared memory */
        shmIds.at(i) = shmget(IPC_PRIVATE, msgBufSize * numLines, S_IRWXU);
        dp2p_assert(shmIds.at(i) != -1);

        /* Attaching shared memory */
        shms.at(i) = (char*)shmat(shmIds.at(i), NULL, 0);
        dp2p_assert(shms.at(i) != (void*)-1);
        shms.at(i)[0] = 0;

        /* Queuing shared memory for automatic destruction after nobody is attached to it any more */
        dp2p_assert(-1 != shmctl(shmIds.at(i), IPC_RMID, 0 ));
    }

    /* Create the named pipelines if necessary */
    struct stat fileStat;
    if(0 != stat(input.c_str(), &fileStat)) {
        dp2p_assert(0 == mkfifo(input.c_str(), S_IRWXU));
        DBGMSG("Creating named pipeline: %s.", input.c_str());
    } else {
        DBGMSG("Named pipeline: %s already exists.", input.c_str());
    }
    dp2p_assert(0 == stat(input.c_str(), &fileStat));
    dp2p_assert(S_ISFIFO(fileStat.st_mode));

    if(0 != stat(output.c_str(), &fileStat)) {
        dp2p_assert(0 == mkfifo(output.c_str(), S_IRWXU));
        DBGMSG("Creating named pipeline: %s.", output.c_str());
    } else {
        DBGMSG("Named pipeline: %s already exists.", output.c_str());
    }
    dp2p_assert(0 == stat(output.c_str(), &fileStat));
    dp2p_assert(S_ISFIFO(fileStat.st_mode));

    ThreadAdapter::startThread(&thread, mainThread, NULL);
#endif
}

void OverlayAdapter::cleanup()
{
#ifdef __ANDROID__
    return;
#else
    DBGMSG("Terminating OverlayAdapter.");

    OverlayAdapter::terminating = true;

    ThreadAdapter::joinThread(thread);

    DBGMSG("Terminating OverlayAdapter: thread terminated.");

    OverlayAdapter::initialized = false;
    input.clear();
    output.clear();
    for(unsigned i = 0; i < shms.size(); ++i)
        dp2p_assert(0 == shmdt(shms.at(i)));
    shms.clear();
    OverlayAdapter::shmIds.clear();
    overlayIds.clear();
    //OverlayAdapter::ackBuf[2048];
    if(fCmd) {
        dp2p_assert(0 == fclose(fCmd));
        fCmd = NULL;
    }
    if(fAck) {
        dp2p_assert(0 == fclose(fAck));
        fAck = NULL;
    }
    ThreadAdapter::mutexDestroy(&OverlayAdapter::mutex);
    ThreadAdapter::condVarDestroy(&OverlayAdapter::condVar);
    //OverlayAdapter::msgBufSize = 2048;
    OverlayAdapter::dirty.clear();
    for(unsigned i = 0; i < msgBuf1->size(); ++i)
        delete [] msgBuf1->at(i);
    delete msgBuf1; msgBuf1 = NULL;
    for(unsigned i = 0; i < msgBuf2->size(); ++i)
        delete [] msgBuf2->at(i);
    delete msgBuf2; msgBuf2 = NULL;

    /* Delete the named pipelines */
    struct stat fileStat;
    if(0 == stat(input.c_str(), &fileStat)) {
        dp2p_assert(0 == remove(input.c_str()));
    }
    if(0 == stat(output.c_str(), &fileStat)) {
    	dp2p_assert(0 == remove(output.c_str()));
    }

    DBGMSG("OverlayAdapter terminated.");
#endif
}

void* OverlayAdapter::mainThread(void*)
{
#ifdef __ANDROID__
    return NULL;
#else

    if(terminating)
        return NULL;

    /* Connect to the pipelines */
    DBGMSG("Opening overlay input: %s.", input.c_str());
    fCmd = fopen(input.c_str(), "w");
    if(!fCmd)
        perror("fopen()");
    else
        DBGMSG("Overlay input pipe opened successfully.");
    DBGMSG("Opening overlay output: %s.", output.c_str());
    fAck = fopen(output.c_str(), "r");
    if(!fAck)
        perror("fopen()");
    else
        DBGMSG("Overlay output pipe opened successfully.");

    if(terminating)
        return NULL;

    /* Create the overlays */
    for(unsigned i = 0; i < overlayIds.size(); ++i)
    {
        fprintf(fCmd, "GenImage\n"); fflush(fCmd);
        ackBuf[0] = 0;
        dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
        dp2p_assert_v(strncmp(ackBuf, "SUCCESS:", 8) == 0, "Received [%8s] instead of SUCCESS.", ackBuf);
        int tmpInt = -1;
        dp2p_assert(1 == fscanf(fAck, "%d", &tmpInt));
        overlayIds.at(i) = tmpInt;
    }

    /* Send shared memory */
    for(unsigned i = 0; i < overlayIds.size(); ++i)
    {
        fprintf(fCmd, "DataSharedMem %d %d %d %s %d\n", overlayIds.at(i), msgBufSize * numLines, 1, "TEXT", shmIds.at(i)); fflush(fCmd);
        ackBuf[0] = 0;
        dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
        dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);
    }

    /* Get textsize */
    fprintf(fCmd, "GetTextSize %d\n", overlayIds.at(0)); fflush(fCmd);
    ackBuf[0] = 0;
    dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
    dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);
    int textSize = -1;
    dp2p_assert(1 == fscanf(fAck, "%d", &textSize));

    /* Make overlay visible */
    for(unsigned i = 0; i < overlayIds.size(); ++i)
    {
        fprintf(fCmd, "SetVisibility %d %d\n", overlayIds.at(i), 1); fflush(fCmd);
        ackBuf[0] = 0;
        dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
        dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);
    }

    ThreadAdapter::mutexInit(&mutex);
    ThreadAdapter::condVarInit(&condVar);

    initialized = true;

    while(!terminating)
    {
        ThreadAdapter::mutexLock(&mutex);
        if(std::find(dirty.begin(), dirty.end(), true) == dirty.end()) {
            DBGMSG("Will wait for overlay output.");
            int ret = ThreadAdapter::condVarTimedWait(&condVar, &mutex, dashp2p::Utilities::getAbsTime() + 1000000);
            if(terminating) {
                DBGMSG("Overlay got terminatingn signal.");
                ThreadAdapter::mutexUnlock(&mutex);
                return NULL;
            } else if(ret == ETIMEDOUT) {
                DBGMSG("Overlay waiting timed out.");
                ThreadAdapter::mutexUnlock(&mutex);
                continue;
            } else {
                DBGMSG("Overlay updated.");
                dp2p_assert(std::find(dirty.begin(), dirty.end(), true) != dirty.end());
            }
        }

        for(int i = 0; i < numLines; ++i)
        {
            if(!dirty.at(i))
                continue;
            char* tmp = msgBuf2->at(i);
            msgBuf2->at(i) = msgBuf1->at(i);
            msgBuf1->at(i) = tmp;
            dirty.at(i) = false;
        }

        ThreadAdapter::mutexUnlock(&mutex);

        if(terminating)
            break;

        /* Send shared memory */
        for(unsigned i = 0; i < overlayIds.size(); ++i)
        {
            int cnt = 0;
            for(unsigned j = 0; j < numLines - 1; ++j)
                cnt += sprintf(shms.at(i) + cnt, "%s\n", msgBuf1->at(j));
            cnt += sprintf(shms.at(i) + cnt, "%s", msgBuf1->at(msgBuf1->size() - 1));

            //fprintf(fCmd, "StartAtomic\n"); fflush(fCmd);
            //ackBuf[0] = 0;
            //dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            //dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);

            //fprintf(fCmd, "SetTextSize %d %d\n", overlayIds.at(i), textSize); //fflush(fCmd);
            //ackBuf[0] = 0;
            //dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            //dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);

            fprintf(fCmd, "DataSharedMem %d %d %d %s %d\n", overlayIds.at(i), msgBufSize * numLines, 1, "TEXT", shmIds.at(i)); fflush(fCmd);
            ackBuf[0] = 0;
            dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);

            //fprintf(fCmd, "EndAtomic\n"); fflush(fCmd);
            //ackBuf[0] = 0;
            //dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            //dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);

            //ackBuf[0] = 0;
            //dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            //dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);

            //ackBuf[0] = 0;
            //dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
            //dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);
        }
    }

    /* Delete the overlay */
    for(unsigned i = 0; i < overlayIds.size(); ++i)
    {
        fprintf(fCmd, "DeleteImage %d\n", overlayIds.at(i)); fflush(fCmd);
        ackBuf[0] = 0;
        dp2p_assert(1 == fscanf(fAck, "%8s", ackBuf));
        dp2p_assert(strncmp(ackBuf, "SUCCESS:", 8) == 0);
    }

    return NULL;
#endif
}

void OverlayAdapter::print(int line, const char* format, ...)
{
#ifdef __ANDROID__
    return;
#else
    if(!initialized || terminating)
        return;

    dp2p_assert(line < numLines);

    ThreadAdapter::mutexLock(&mutex);

    va_list argList;
    va_start(argList, format);
    vsnprintf(msgBuf2->at(line), msgBufSize, format, argList);
    va_end(argList);

    //static int cnt = 0;
    if(0 != strcmp(msgBuf1->at(line), msgBuf2->at(line))) {
        dirty.at(line) = true;
        //sprintf(msgBuf2->at(numLines - 1), "%d", ++cnt);
        //dirty.at(numLines - 1) = true;
        ThreadAdapter::condVarSignal(&condVar);
    }

    ThreadAdapter::mutexUnlock(&mutex);
#endif
}

}

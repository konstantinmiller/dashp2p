/****************************************************************************
 * OverlayAdapter.h                                                         *
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

#ifndef OVERLAYADAPTER_H_
#define OVERLAYADAPTER_H_


#include "ThreadAdapter.h"
#include <stdio.h>
#include <vector>
#include <string>
using std::string;

namespace dashp2p {

class OverlayAdapter
{
private:
    OverlayAdapter();
    virtual ~OverlayAdapter();

public:
    static void init(const string& input, const string& output);
    static void cleanup();
    static void* mainThread(void*);
    static void print(int line, const char* format, ...);

private:
    static Thread thread;
    static bool terminating;
    static bool initialized;
    static string input;
    static string output;
    static const int numOverlays;
    static std::vector<char*> shms;
    static std::vector<int> shmIds;
    static std::vector<int> overlayIds;
    static char ackBuf[2048];
    static FILE* fCmd;
    static FILE* fAck;
    static Mutex mutex;
    static CondVar condVar;
    static const int msgBufSize;
    static const int numLines;
    static std::vector<bool> dirty;
    static std::vector<char*>* msgBuf1;
    static std::vector<char*>* msgBuf2;
};

}

#endif /* OVERLAYADAPTER_H_ */


#if 0
DataSharedMem
DeleteImage
EndAtomic
GenImage
GetAlpha
GetPosition
GetTextAlpha
GetTextColor
GetTextSize
GetVisibility
SetAlpha
SetPosition
SetTextAlpha
SetTextColor
SetTextSize
SetVisibility
StartAtomic
#endif

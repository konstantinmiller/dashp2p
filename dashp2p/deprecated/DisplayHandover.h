/****************************************************************************
 * DisplayHandover.h                                                        *
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

#ifndef DISPLAYHANDOVER_H_
#define DISPLAYHANDOVER_H_


#include "ThreadAdapter.h"
#include "DebugAdapter.h"
#include "Utilities.h"


class DisplayHandover
{
private:
    DisplayHandover(){}
    virtual ~DisplayHandover(){}

/* Public methods */
public:
    static void init(int portDisplayHandover, bool withHandover);
    static void cleanup();
    static void playbackStarted();
    static void playbackTerminated();
    static void displayOverlay(int line, const char* format, ...);

/* Private methods */
private:
    static void* threadEntry(void* params);

/* Private fields */
private:
    static bool withHandover;
    static int portDisplayHandover;
    static bool ifTerminating;
    static Thread thread;
    static int fdSocket;
};

#endif /* DISPLAYHANDOVER_H_ */

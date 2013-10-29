/****************************************************************************
 * ThreadAdapter.h                                                          *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Aug 10, 2012                                                 *
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

#ifndef THREADADAPTER_H_
#define THREADADAPTER_H_


#if DP2P_VLC != 0 // Use VLC threads
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
typedef vlc_thread_t Thread;
typedef vlc_cond_t CondVar;
typedef vlc_mutex_t Mutex;
#else // Use pthreads
#include <pthread.h>
typedef pthread_t Thread;
typedef pthread_cond_t CondVar;
typedef pthread_mutex_t Mutex;
#endif

#include <inttypes.h>

class ThreadAdapter
{
private:
    ThreadAdapter(){}
    ~ThreadAdapter(){}

/* Public methods */
public:
    static int startThread(Thread* thread, void*(*entry)(void*), void* data);
    static void joinThread(Thread thread);
    static void mutexInit(Mutex* mutex);
    static void mutexDestroy(Mutex* mutex);
    static void mutexLock(Mutex* mutex);
    static void mutexUnlock(Mutex* mutex);
    static void condVarInit(CondVar* condVar);
    static void condVarDestroy(CondVar* condVar);
    static void condVarSignal(CondVar* condVar);
    static void condVarWait(CondVar* condVar, Mutex* mutex);
    static int condVarTimedWait(CondVar* condVar, Mutex* mutex, int64_t abstime); // abstime in [us]
};

#endif /* THREADADAPTER_H_ */

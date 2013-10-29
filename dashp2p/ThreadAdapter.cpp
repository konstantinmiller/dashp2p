/****************************************************************************
 * ThreadAdapter.cpp                                                        *
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


#include "Dashp2pTypes.h"
#include "ThreadAdapter.h"
#include <assert.h>
#include <time.h>


int ThreadAdapter::startThread(Thread* thread, void*(*entry)(void*), void* data)
{
#if DP2P_VLC != 0
    return vlc_clone(thread, entry, data, VLC_THREAD_PRIORITY_INPUT);
#else
    return pthread_create(thread, NULL, entry, data);
#endif
}

void ThreadAdapter::joinThread(Thread thread)
{
#if DP2P_VLC != 0
    vlc_join(thread, NULL);
#else
    dp2p_assert(0 == pthread_join(thread, NULL));
#endif
}

void ThreadAdapter::mutexInit(Mutex* mutex)
{
#if DP2P_VLC != 0
    vlc_mutex_init(mutex);
#else
    dp2p_assert(0 == pthread_mutex_init(mutex, NULL));
#endif
}

void ThreadAdapter::mutexDestroy(Mutex* mutex)
{
#if DP2P_VLC != 0
    vlc_mutex_destroy(mutex);
#else
    dp2p_assert(0 == pthread_mutex_destroy(mutex));
#endif
}

void ThreadAdapter::mutexLock(Mutex* mutex)
{
#if DP2P_VLC != 0
    vlc_mutex_lock(mutex);
#else
    dp2p_assert(0 == pthread_mutex_lock(mutex));
#endif
}

void ThreadAdapter::mutexUnlock(Mutex* mutex)
{
#if DP2P_VLC != 0
    vlc_mutex_unlock(mutex);
#else
    dp2p_assert(0 == pthread_mutex_unlock(mutex));
#endif
}

void ThreadAdapter::condVarInit(CondVar* condVar)
{
#if DP2P_VLC != 0
    vlc_cond_init_daytime(condVar);
#else
    dp2p_assert(0 == pthread_cond_init (condVar, NULL));
#endif
}

void ThreadAdapter::condVarDestroy(CondVar* condVar)
{
#if DP2P_VLC != 0
    vlc_cond_destroy(condVar);
#else
    dp2p_assert(0 == pthread_cond_destroy(condVar));
#endif
}

void ThreadAdapter::condVarSignal(CondVar* condVar)
{
#if DP2P_VLC != 0
    vlc_cond_signal(condVar);
#else
    dp2p_assert(0 == pthread_cond_signal(condVar));
#endif
}

void ThreadAdapter::condVarWait(CondVar* condVar, Mutex* mutex)
{
#if DP2P_VLC != 0
    vlc_cond_wait(condVar, mutex);
#else
    dp2p_assert(0 == pthread_cond_wait(condVar, mutex));
#endif
}

int ThreadAdapter::condVarTimedWait(CondVar* condVar, Mutex* mutex, int64_t abstime)
{
#if DP2P_VLC != 0
    return vlc_cond_timedwait(condVar, mutex, abstime);
#else
    struct timespec ts;
    ts.tv_nsec = 1000 * (abstime % 1000000);
    ts.tv_sec = abstime / 1000000;
    // TODO: check if that works fine with abstime obtained from gettimeofday()
    return pthread_cond_timedwait(condVar, mutex, &ts);
#endif
}

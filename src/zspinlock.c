/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2014, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zspinlock.c
 * @brief Spinlock functions used for log.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#include "zspinlock.h"

#ifdef _MSC_VER
#include <windows.h>
#elif defined(__GNUC__)
#if __GNUC__<4 || (__GNUC__==4 && __GNUC_MINOR__<1)
#error GCC version must be greater or equal than 4.1.2
#endif
#include <sched.h>
#else
#error Currently only windows and linux os are supported
#endif

void spin_init(spin_lock_t* lock,long* flag);
void spin_lock(spin_lock_t* lock);
int spin_trylock(spin_lock_t* lock);
void spin_unlock(spin_lock_t* lock);
int spin_is_lock(spin_lock_t* lock);

void spin_init(spin_lock_t* lock,long* flag)
{
#ifdef _MSC_VER
    InterlockedExchange((volatile long*)&lock->spin_,0);
    //InterlockedExchange((long*)&lock->spin_,flag?(long)flag:(long)&lock->flag_);
#elif defined(__GNUC__)
    __sync_and_and_fetch((long*)&lock->spin_,0);
    //__sync_lock_test_and_set((long*)&lock->spin_,flag?(long)flag:(long)&lock->flag_);
#endif
}

void spin_lock(spin_lock_t* lock)
{
#ifdef _MSC_VER
    for (;0!=InterlockedExchange((volatile long*)&lock->spin_,1);)
    {
        ;
    }
#elif defined(__GNUC__)
    for (;0!=__sync_fetch_and_or((long*)&lock->spin_,1);)
    {
        ;
    }
#endif
}

int spin_trylock(spin_lock_t* lock)
{
#ifdef _MSC_VER
    return !InterlockedExchange((volatile long*)&lock->spin_,1);
#elif defined(__GNUC__)
    return !__sync_fetch_and_or((long*)&lock->spin_,1);
#endif
}

void spin_unlock(spin_lock_t* lock)
{
#ifdef _MSC_VER
    InterlockedExchange((volatile long*)&lock->spin_,0);
#elif defined(__GNUC__)
    __sync_and_and_fetch((long*)&lock->spin_,0);
#endif
}

int spin_is_lock(spin_lock_t* lock)
{
#ifdef _MSC_VER
    return InterlockedExchangeAdd((volatile long*)&lock->spin_,0);
#elif defined(__GNUC__)
    return __sync_add_and_fetch((long*)&lock->spin_,0);
#endif
}

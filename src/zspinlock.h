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
 * @file zspinlock.h
 * @brief Spinlock functions used for log header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#ifndef ZSPINLOCK_H
#define ZSPINLOCK_H

#include "zcommon.h"

typedef struct {
    volatile long spin_;
    volatile long flag_;
}spin_lock_t;

void spin_init(spin_lock_t* lock, long* flag);
void spin_lock(spin_lock_t* lock);
void spin_unlock(spin_lock_t* lock);
int  spin_trylock(spin_lock_t* lock);
int  spin_is_lock(spin_lock_t* lock);

#endif

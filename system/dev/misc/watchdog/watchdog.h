// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2013 Google Inc.
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zircon/types.h>

#define WATCHDOG_MAGIC 'wdog'
#define PET_SIGNAL ZX_USER_SIGNAL_0
typedef struct watch_dog {
    platform_device_protocol_t pdev;
    zx_device_t* zxdev;
    zx_time_t hw_watchdog_pet_timeout;
    zx_time_t hw_watchdog_bark_timeout;
    port_handler_t th;
    zx_handle_t timer_irq;
    zx_handle_t hw_watchdog_timer;
    io_buffer_t regs_iobuff;
    void* hw_watchdog_base;
} watch_dog_t;

/* HW watchdog support.  This is nothing but a simple helper used to
 * automatically dismiss a platform's HW watchdog using LK timers.  Platforms
 * must supply
 *
 * platform_watchdog_init
 * platform_watchdog_pet
 *
 * in order to use the HW watchdog helper functions.  After initialized, users
 * may enable and disable the HW watchdog whenever appropriate.  The helper will
 * maintain a timer which dismisses the watchdog at the pet interval recommended
 * by the platform.  Any programming error which prevents the scheduler timer
 * mechanism from running properly will eventually result in the watchdog firing
 * and the system rebooting.  Whenever possible, when using SW based watchdogs,
 * it is recommended that systems provide platform support for a HW watchdog and
 * enable the HW watchdog.  SW watchdogs are based on LK timers, and should be
 * reliable as long as the scheduler and timer mechanism is running properly;
 * the HW watchdog functionality provided here should protect the system in case
 * something managed to break timers on LK.
 */

extern zx_status_t platform_watchdog_init(watch_dog_t* wdog);
extern void platform_watchdog_set_enabled(bool enabled);
extern void platform_watchdog_pet(watch_dog_t* wdog);
extern void trigger_wdog_bite(watch_dog_t* wdog);
extern void wdog_bark_handler(void* args);
extern void task_for_other_cpus(void* args);
extern void ping_other_cpus(watch_dog_t* wdog);
zx_status_t watchdog_hw_init(watch_dog_t* wdog);
extern int wdog_work(void* args);
void watchdog_hw_set_enabled(bool enabled);

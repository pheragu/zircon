// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2013 Google Inc.
// Copyright (c) 2015 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <arch/arm64/mp.h>
#include <arch/arm64/periphmap.h>
#include <assert.h>
#include <dev/interrupt.h>
#include <dev/timer/arm_generic.h>
#include <err.h>
#include <inttypes.h>
#include <kernel/cpu.h>
#include <kernel/event.h>
#include <kernel/mp.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <lib/watchdog.h>
#include <pdev/driver.h>
#include <platform.h>
#include <platform/timer.h>
#include <stdlib.h>
#include <trace.h>
#include <zircon/boot/driver-config.h>
#include <zircon/compiler.h>
#include <zircon/types.h>
#include <zircon/types.h>

#define WDT0_ACCSCSSNBARK_INT 0
#define TCSR_WDT_CFG 0x30
#define WDT0_RST 0x04
#define WDT0_EN 0x08
#define WDT0_STS 0x0C
#define WDT0_BARK_TIME 0x10
#define WDT0_BITE_TIME 0x14
#define PET_TIME 9000000000
#define BARK_TIME 11

static long WDT_HZ = 32765;

static void hw_watchdog_timer_callback(timer_t* timer, zx_time_t now, void* arg);

void trigger_wdog_bite(watch_dog_t* wdog) {
    writel(1, wdog->hw_watchdog_base + WDT0_BITE_TIME);
    /* Mke sure bite time is written before we reset */
    mb();
    writel(1, wdog->hw_watchdog_base + WDT0_RST);
    /* Make sure we wait only after reset */
    mb();
    /* Delay to make sure bite occurs */
    while (1)
        ;
}

void wdog_bark_handler(void* args) {
    watch_dog_t* wdog = (watch_dog_t*)args;

    dprintf(INFO, "Bark handler initiated\n");
    trigger_wdog_bite(wdog);
}

void platform_watchdog_pet(watch_dog_t* wdog) {
    zx_time_t deadline = current_time() + wdog->hw_watchdog_pet_timeout;
    writel(1, wdog->hw_watchdog_base + WDT0_RST);
    /* Start a timer periodically*/
    timer_set_oneshot(&wdog->hw_watchdog_timer, deadline, hw_watchdog_timer_callback, wdog);
}

static void hw_watchdog_timer_callback(timer_t* timer, zx_time_t now, void* arg) {
    watch_dog_t* wdog = (watch_dog_t*)arg;
    wdog->pet_wdog = true;
    event_signal(&wdog->wdog_event, true);
}

void task_for_other_cpus(void* args) {
    watch_dog_t* wdog = (watch_dog_t*)args;
    wdog->cpumask |= cpu_num_to_mask(arch_curr_cpu_num());
    smp_mb();
}

void ping_other_cpus(watch_dog_t* wdog) {
    wdog->cpumask = 0 & (wdog->cpumask);
    smp_mb();
    cpu_mask_t online_mask = 0;
    mp_sync_exec(MP_IPI_TARGET_ALL, online_mask, task_for_other_cpus, wdog);
}

int wdog_work(void* args) {
    watch_dog_t* wdog = (watch_dog_t*)args;
    for (;;) {
        while (!wdog->pet_wdog) {
            event_wait(&wdog->wdog_event);
        }
        /* Ensure you mention we are ready to wait on next signal*/
        event_unsignal(&wdog->wdog_event);
        ping_other_cpus(wdog);
        wdog->pet_wdog = false;
        platform_watchdog_pet(wdog);
    }
    return 0;
}

zx_status_t platform_watchdog_init(watch_dog_t* wdog) {
    zx_time_t deadline = current_time() + wdog->hw_watchdog_pet_timeout;
    timer_set_oneshot(&wdog->hw_watchdog_timer, deadline, hw_watchdog_timer_callback, wdog);
    return ZX_OK;
}

zx_status_t watchdog_hw_init(watch_dog_t* wdog) {
    DEBUG_ASSERT(ZX_TIME_INFINITE != wdog->hw_watchdog_pet_timeout);
    event_init(&wdog->wdog_event, false, 0);
    timer_init(&wdog->hw_watchdog_timer);
    return platform_watchdog_init(wdog);
}

static void watchdog_init(const void* driver_data, uint32_t length) {

    ASSERT(length >= sizeof(dcfg_simple_t));
    const dcfg_simple_t* driver = driver_data;
    watch_dog_t* wdog_data = NULL;
    ASSERT(driver->mmio_phys);

    wdog_data = malloc(sizeof(watch_dog_t));
    wdog_data->hw_watchdog_base = periph_paddr_to_vaddr(driver->mmio_phys);
    ASSERT(wdog_data->hw_watchdog_base);

    wdog_data->timer_irq = driver->irq;
    zx_status_t status = register_int_handler(wdog_data->timer_irq, &wdog_bark_handler, wdog_data);
    DEBUG_ASSERT(status == ZX_OK);
    unmask_interrupt(wdog_data->timer_irq);

    wdog_data->pet_wdog = false;
    wdog_data->t = thread_create_etc(&wdog_data->wdog_thread, "wdog_thread", &wdog_work, wdog_data, DEFAULT_PRIORITY, NULL);
    wdog_data->hw_watchdog_pet_timeout = PET_TIME;
    wdog_data->hw_watchdog_bark_timeout = (BARK_TIME * WDT_HZ);

    writel(wdog_data->hw_watchdog_bark_timeout, wdog_data->hw_watchdog_base + WDT0_BARK_TIME);
    writel(wdog_data->hw_watchdog_bark_timeout + 3 * WDT_HZ, wdog_data->hw_watchdog_base + WDT0_BITE_TIME);

    watchdog_hw_init(wdog_data);

    writel(1, wdog_data->hw_watchdog_base + WDT0_EN);
    writel(1, wdog_data->hw_watchdog_base + WDT0_RST);

    /* start the created thread */
    status = thread_detach_and_resume(wdog_data->t);
    if (status != ZX_OK) {
        dprintf(INFO, "Watchdog init failed\n");
        free(wdog_data);
    }
}

LK_PDEV_INIT(watchdog_init, KDRV_ARM_WATCH_DOG, watchdog_init, LK_INIT_LEVEL_PLATFORM);

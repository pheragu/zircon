// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2013 Google Inc.
// Copyright (c) 2015 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT


#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <inttypes.h>
#include <zircon/compiler.h>
#include <zircon/types.h>
#include <zircon/process.h>
#include <zircon/boot/driver-config.h>

#include <ddk/driver.h>
#include <ddk/device.h>
#include <ddk/binding.h>
#include <ddk/io-buffer.h>
#include <zircon/syscalls/debug.h>
#include <hw/arch_ops.h>
#include <ddk/protocol/platform-device.h>
#include <ddk/protocol/platform-defs.h>
#include <hw/reg.h>
#include <port/port.h>

#include <stdio.h>
#include <unistd.h>
#include <threads.h>
#include "watchdog.h"

#define WDT0_ACCSCSSNBARK_INT 0
#define TCSR_WDT_CFG    0x30
#define WDT0_RST    0x04
#define WDT0_EN     0x08
#define WDT0_STS    0x0C
#define WDT0_BARK_TIME  0x10
#define WDT0_BITE_TIME  0x14
#define PET_TIME    9000000000
#define BARK_TIME   11

static zx_protocol_device_t wdog_device_ops = {
    .version = DEVICE_OPS_VERSION,
};

static long WDT_HZ = 32765;
port_t port;

/* BITE forcefully */
void trigger_wdog_bite(watch_dog_t *wdog) {
    writel(1, wdog->hw_watchdog_base + WDT0_BITE_TIME);
    /* Mke sure bite time is written before we reset */
    hw_mb();
    writel(1, wdog->hw_watchdog_base + WDT0_RST);
    /* Make sure we wait only after reset */
    hw_mb();
    /* Delay to make sure bite occurs */
    while(1);
}

/* Thread waiting for bark interrupt */
int wdog_work_irq(void *args)
{
    watch_dog_t *wdog = (watch_dog_t *) args;
    zx_status_t status;

    for(;;) {
                status = zx_interrupt_wait(wdog->timer_irq, NULL);
                if (status != ZX_OK)
                        return status;  
                trigger_wdog_bite(wdog);
    }
    /* Ensure you mention we are ready to wait on next signal*/
    return 0;
}

static zx_status_t wdog_callback(port_handler_t *ph, zx_signals_t signals, uint32_t evt) {
    watch_dog_t *wdog = containerof(ph, watch_dog_t, th);
    // Pet the watchdog
    platform_watchdog_pet(wdog);
    // Ensure the call back doesnt hit again
    return ZX_ERR_NEXT;
}

void platform_watchdog_pet(watch_dog_t *wdog) {
    writel(1, wdog->hw_watchdog_base + WDT0_RST);
    // Restart the timer everytime it is pet
    zx_timer_set(wdog->hw_watchdog_timer, zx_deadline_after(wdog->hw_watchdog_pet_timeout), 0);
}

int wdog_work(void *args)
{
    port_dispatch(&port, ZX_TIME_INFINITE, false);
    return 0;
}

zx_status_t platform_watchdog_init(watch_dog_t *wdog) {
    
    /* start the created thread waiting for pet_time*/ 

    thrd_t t;
    thrd_create_with_name(&t, wdog_work, wdog, "wdog_thread");
    // Start the pet timer
    zx_timer_set(wdog->hw_watchdog_timer, zx_deadline_after(wdog->hw_watchdog_pet_timeout), 0);

    wdog->th.handle = wdog->hw_watchdog_timer;
    // Signal the port needs to wait for
    wdog->th.waitfor = ZX_TIMER_SIGNALED;
    // Callback once signal is received at the port
    wdog->th.func = wdog_callback;
    // Keep listening always
    port_wait_repeating(&port, &wdog->th);
    return ZX_OK;
}

zx_status_t watchdog_hw_init(watch_dog_t *wdog)
{
    zx_status_t status;

    /* IO remmap - pdev has all the mmios */
    status = pdev_map_mmio_buffer(&wdog->pdev, 0, ZX_CACHE_POLICY_UNCACHED_DEVICE, &wdog->regs_iobuff);
    if(status != ZX_OK) {
        printf("Watchdog map mmio failed\n"); 
        goto hw_init_fail;
    }
    /* Get base address */
    wdog->hw_watchdog_base = io_buffer_virt(&wdog->regs_iobuff);

    /* Map interrupt */
    status = pdev_map_interrupt(&wdog->pdev, 0, &wdog->timer_irq);
    if(status != ZX_OK) {
        printf("Watchdog irq failed\n"); 
        goto hw_init_fail;
    }

    /* create a timer handle for pet timer */
    status = zx_timer_create(0, ZX_CLOCK_MONOTONIC, &wdog->hw_watchdog_timer);
    if(status != ZX_OK) {
        printf("Watchdog timer failed\n");
        goto hw_init_fail;
    }

    /* Creating a thread to watch out for bark interrupt */
    thrd_t t_irq;
    thrd_create_with_name(&t_irq, wdog_work_irq, wdog, "wdog_work_irq");
    return ZX_OK;

hw_init_fail:
    if (wdog) {
        io_buffer_release(&wdog->regs_iobuff);
        if (wdog->timer_irq != ZX_HANDLE_INVALID)
                zx_handle_close(wdog->timer_irq);
        if (wdog->hw_watchdog_timer != ZX_HANDLE_INVALID)
                zx_handle_close(wdog->hw_watchdog_timer);
    }
    return status;
}

static zx_status_t watchdog_init(void* ctx, zx_device_t* parent) {
    
    watch_dog_t* wdog_data = calloc(1, sizeof(watch_dog_t));
    if (!wdog_data) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Get device info from board file */
    zx_status_t status = device_get_protocol(parent, ZX_PROTOCOL_PLATFORM_DEV, &wdog_data->pdev);
    if(status != ZX_OK) {
        printf("Watchdog get protocol failed\n"); 
        free(wdog_data);
        return status;
    }
    
    status = watchdog_hw_init(wdog_data); 
    if(status != ZX_OK) {
        goto init_fail;
        return status;
    }
    /* Initilialize a port which will be needed to get the timer signal */
    if(port_init(&port) < 0) {
        printf("Watchdog port init failed\n");
        goto init_fail;
        return -1;
    }

    device_add_args_t args = {
        .version = DEVICE_ADD_ARGS_VERSION,
        .name = "wdog",
        .ctx = wdog_data,
        .ops = &wdog_device_ops,
    };

    status = device_add(parent, &args, &wdog_data->zxdev);
    if(status != ZX_OK) {
        printf("Watchdog device_add failed\n"); 
        goto init_fail;
        return status;
    }

    wdog_data->hw_watchdog_pet_timeout = PET_TIME; 
    wdog_data->hw_watchdog_bark_timeout = (BARK_TIME * WDT_HZ);

    writel(wdog_data->hw_watchdog_bark_timeout, wdog_data->hw_watchdog_base + WDT0_BARK_TIME);
    writel(wdog_data->hw_watchdog_bark_timeout + 3*WDT_HZ, wdog_data->hw_watchdog_base + WDT0_BITE_TIME);
   
    platform_watchdog_init(wdog_data);  
    
    writel(1, wdog_data->hw_watchdog_base + WDT0_EN);
    writel(1, wdog_data->hw_watchdog_base + WDT0_RST);
 
    printf("watchdog_init complete\n");

    return ZX_OK;

init_fail:
    if (wdog_data) {
        io_buffer_release(&wdog_data->regs_iobuff);
        if (wdog_data->timer_irq != ZX_HANDLE_INVALID)
                zx_handle_close(wdog_data->timer_irq);
        if (wdog_data->hw_watchdog_timer != ZX_HANDLE_INVALID)
                zx_handle_close(wdog_data->hw_watchdog_timer);
        free(wdog_data);
    }
    return status;
}
static zx_driver_ops_t wdog_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = watchdog_init,
};

ZIRCON_DRIVER_BEGIN(wdog_driver, wdog_driver_ops, "zircon", "0.1", 3)
    BI_MATCH_IF(EQ, BIND_PROTOCOL, ZX_PROTOCOL_PLATFORM_DEV),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_GENERIC),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_GENERIC),
ZIRCON_DRIVER_END(wdog_driver)


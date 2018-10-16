#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/platform-defs.h>

#include <zircon/assert.h>
#include <zircon/process.h>
#include <zircon/syscalls.h>
#include <zircon/threads.h>

#include "sdm845.h"

static const pbus_mmio_t wdog_mmios[] = {
    {
        .base = 0x17980000,
        .length = 0x1000,
    },
};

static const pbus_irq_t wdog_irqs[] = {
   {
        .irq = 32,
        .mode = ZX_INTERRUPT_MODE_LEVEL_HIGH,
   },
};

static const pbus_dev_t wdog_dev = {
   .name = "watchdog",
   .vid = PDEV_VID_GENERIC,
   .pid = PDEV_PID_GENERIC,
   .mmios = wdog_mmios,
   .mmio_count = countof(wdog_mmios),
   .irqs = wdog_irqs,
   .irq_count = countof(wdog_irqs),
};

static void sdm845_release(void* ctx) {

    sdm845_t* board = ctx;
    free(board);
}

static zx_protocol_device_t sdm845_device_protocol = {
    .version = DEVICE_OPS_VERSION,
    .release = sdm845_release,
};


static zx_status_t sdm845_bind(void* ctx, zx_device_t* parent) {
    printf("Initing sdm845 board\n");
    sdm845_t* board = calloc(1, sizeof(sdm845_t));
    if (!board) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_status_t status = device_get_protocol(parent, ZX_PROTOCOL_PLATFORM_BUS, &board->pbus);
    if (status != ZX_OK) {
        free(board);
        return ZX_ERR_NOT_SUPPORTED;
    }
    
    board->parent = parent;

    device_add_args_t args = {
        .version = DEVICE_ADD_ARGS_VERSION,
        .name = "sdm845",
        .ctx = board,
        .ops = &sdm845_device_protocol,
        // nothing should bind to this device
        // all interaction will be done via the pbus_interface_t
        //.flags = DEVICE_ADD_NON_BINDABLE,
    };

    status = device_add(parent, &args, NULL);
    if (status != ZX_OK) {
        goto fail;
    }
   
    status = pbus_device_add(&board->pbus, &wdog_dev);
    if (status != ZX_OK) {
    zxlogf(ERROR, "sdm845_bind failed %d pbus_device_add\n", status);
        goto fail;
    }

    return ZX_OK;

fail:
    zxlogf(ERROR, "sdm845_bind failed %d\n", status);
    sdm845_release(board);
    return status;
}

static zx_driver_ops_t sdm845_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = sdm845_bind,
};

ZIRCON_DRIVER_BEGIN(sdm845, sdm845_driver_ops, "zircon", "0.1", 3)
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_PLATFORM_BUS),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_QCOM),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_PID, PDEV_PID_SDM845),
ZIRCON_DRIVER_END(sdm845)


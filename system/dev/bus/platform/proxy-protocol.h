// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/protocol/canvas.h>
#include <ddk/protocol/i2c.h>
#include <ddk/protocol/platform-device.h>
#include <ddk/protocol/scpi.h>
#include <ddk/protocol/usb-mode-switch.h>

// maximum transfer size we can proxy.
#define PDEV_I2C_MAX_TRANSFER_SIZE 4096

// RPC ops
enum {
    // ZX_PROTOCOL_PLATFORM_DEV
    PDEV_GET_MMIO = 1,
    PDEV_GET_INTERRUPT,
    PDEV_GET_BTI,
    PDEV_GET_DEVICE_INFO,
    PDEV_GET_BOARD_INFO,

    // ZX_PROTOCOL_USB_MODE_SWITCH
    UMS_SET_MODE,

    // ZX_PROTOCOL_GPIO
    GPIO_CONFIG,
    GPIO_SET_ALT_FUNCTION,
    GPIO_READ,
    GPIO_WRITE,
    GPIO_GET_INTERRUPT,
    GPIO_RELEASE_INTERRUPT,
    GPIO_SET_POLARITY,

    // ZX_PROTOCOL_I2C
    I2C_GET_MAX_TRANSFER,
    I2C_TRANSACT,

    // ZX_PROTOCOL_CLK
    CLK_ENABLE,
    CLK_DISABLE,

    // ZX_PROTOCOL_SCPI
    SCPI_GET_SENSOR,
    SCPI_GET_SENSOR_VALUE,
    SCPI_GET_DVFS_INFO,
    SCPI_GET_DVFS_IDX,
    SCPI_SET_DVFS_IDX,

    // ZX_PROTOCOL_CANVAS
    CANVAS_CONFIG,
    CANVAS_FREE,
};

typedef struct {
    zx_txid_t txid;
    uint32_t protocol;
    uint32_t op;
} rpc_header_t;

typedef struct {
    uint32_t index;
    uint32_t flags;
} rpc_pdev_req_t;

typedef struct {
    zx_paddr_t paddr;
    size_t length;
    uint32_t irq;
    uint32_t mode;
    pdev_device_info_t device_info;
    pdev_board_info_t board_info;
} rpc_pdev_rsp_t;

typedef struct {
        usb_mode_t usb_mode;
} rpc_ums_req_t;

typedef struct {
    uint32_t index;
    uint32_t flags;
    uint32_t polarity;
    uint64_t alt_function;
    uint8_t value;
} rpc_gpio_req_t;

typedef struct {
    uint8_t value;
} rpc_gpio_rsp_t;

typedef struct {
    uint32_t index;
    size_t write_length;
    size_t read_length;
    i2c_complete_cb complete_cb;
    void* cookie;
} rpc_i2c_req_t;

typedef struct {
    uint32_t index;
} rpc_clk_req_t;

typedef struct {
    size_t max_transfer;
    i2c_complete_cb complete_cb;
    void* cookie;
} rpc_i2c_rsp_t;

typedef struct {
    uint8_t power_domain;
    uint16_t idx;
    uint32_t sensor_id;
    char name[20];
} rpc_scpi_req_t;

typedef struct {
    uint32_t sensor_value;
    uint16_t dvfs_idx;
    uint32_t sensor_id;
    scpi_opp_t opps;
} rpc_scpi_rsp_t;

typedef struct {
    canvas_info_t info;
    size_t offset;
    uint8_t idx;
} rpc_canvas_req_t;

typedef struct {
    uint8_t idx;
} rpc_canvas_rsp_t;

typedef struct {
    rpc_header_t header;
    union {
        rpc_pdev_req_t pdev;
        rpc_ums_req_t ums;
        rpc_gpio_req_t gpio;
        rpc_i2c_req_t i2c;
        rpc_clk_req_t clk;
        rpc_scpi_req_t scpi;
        rpc_canvas_req_t canvas;
    };
} pdev_req_t;

typedef struct {
    rpc_header_t header;
    zx_status_t status;
    union {
        rpc_pdev_rsp_t pdev;
        rpc_gpio_rsp_t gpio;
        rpc_i2c_rsp_t i2c;
        rpc_scpi_rsp_t scpi;
        rpc_canvas_rsp_t canvas;
    };
} pdev_resp_t;

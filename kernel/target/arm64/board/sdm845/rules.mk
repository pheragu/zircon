# Copyright 2017 The Fuchsia Authors
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
$(warning "SETTING BOOT_SHIM USE")
LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM_BOARD_NAME := sdm845
PLATFORM_USE_SHIM := true

include make/board.mk
include make/fastboot.mk

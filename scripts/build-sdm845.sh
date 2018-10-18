#!/bin/sh

# Copyright 2016 The Fuchsia Authors
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

BUILD_DIR=build-arm64
#(TODO) - Add mkbootimg in your path
MKBOOTIMG=/local/mnt/workspace/kdev/tools/mkbootimg/mkbootimg

KERNEL=$BUILD_DIR/sdm845-zzircon.bin-dtb

OUT_IMAGE=$BUILD_DIR/boot.img

MEMBASE=0x80000000
DT_ADDR=0x1e00000

# have mkbootimg from a kdev environment in your PATH
# this can be found at /path/to/kdev/tools/mkbootimg
$MKBOOTIMG --kernel $KERNEL \
--base $MEMBASE \
--pagesize 4096 \
--tags_offset $DT_ADDR \
-o $OUT_IMAGE || exit 1

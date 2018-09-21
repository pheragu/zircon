# Copyright 2016 The Fuchsia Authors
# Copyright (c) 2008-2015 Travis Geiselbrecht
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

# Project for sdm845-arm64

TARGET := sdm845
ARM_CPU := cortex-a53
ARCH := arm64

include kernel/project/virtual/test.mk
include kernel/project/virtual/user.mk



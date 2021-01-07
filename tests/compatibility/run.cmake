# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

# Simple script to 'create' with one executable and 'open' with another one

include(${SRC_DIR}/../../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE1} ${DIR}/testfile c)
execute(${TEST_EXECUTABLE2} ${DIR}/testfile o)

finish()

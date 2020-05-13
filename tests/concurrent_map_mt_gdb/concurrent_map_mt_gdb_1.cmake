# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

crash_with_gdb(${SRC_DIR}/concurrent_map_mt_gdb_1.gdb ${TEST_EXECUTABLE} i 1 ${DIR}/testfile)
execute(${TEST_EXECUTABLE} c 1 ${DIR}/testfile)

finish()

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

crash_with_gdb(${SRC_DIR}/concurrent_map_mt_gdb_0.gdb ${TEST_EXECUTABLE} i 0 ${DIR}/testfile)
execute(${TEST_EXECUTABLE} c 0 ${DIR}/testfile)

finish()

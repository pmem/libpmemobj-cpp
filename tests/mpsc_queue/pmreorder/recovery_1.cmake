# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} c 0 ${DIR}/testfile)
pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} x 0 ${DIR}/testfile 0)
pmreorder_execute(true NoReorderNoCheck "PMREORDER_MARKER=ReorderAccumulative" ${TEST_EXECUTABLE} o 0)

finish()

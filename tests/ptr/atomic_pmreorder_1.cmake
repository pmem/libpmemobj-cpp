# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} c ${DIR}/testfile)
pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} i ${DIR}/testfile)
pmreorder_execute(true NoReorderNoCheck "PMREORDER_MARKER=ReorderAccumulative" ${TEST_EXECUTABLE} o)

finish()

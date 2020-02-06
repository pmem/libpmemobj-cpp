# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

set(TESTDIR ${DIR}/testdir)
configure_file(${SRC_DIR}/pool.set.in ${DIR}/pool.set @ONLY)

file(MAKE_DIRECTORY ${DIR}/testdir)

execute(${TEST_EXECUTABLE} ${DIR}/pool.set)

finish()

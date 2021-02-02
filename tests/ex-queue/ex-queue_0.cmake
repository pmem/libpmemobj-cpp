# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} ${DIR}/testfile push 1)
execute(${TEST_EXECUTABLE} ${DIR}/testfile push 2)
execute(${TEST_EXECUTABLE} ${DIR}/testfile push 3)
execute(${TEST_EXECUTABLE} ${DIR}/testfile show)
execute(${TEST_EXECUTABLE} ${DIR}/testfile pop)
execute(${TEST_EXECUTABLE} ${DIR}/testfile show)

check_file_exists(${DIR}/testfile)

finish()

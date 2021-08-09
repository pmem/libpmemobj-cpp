# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# properly create using default size and mode
execute(${TEST_EXECUTABLE} t ${DIR}/testfile "test")
check_file_exists(${DIR}/testfile)

execute(${TEST_EXECUTABLE} o ${DIR}/testfile "test")

finish()

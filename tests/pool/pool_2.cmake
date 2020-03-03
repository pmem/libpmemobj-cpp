# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test non-existing file, poolsize > 0
execute(${TEST_EXECUTABLE} d ${DIR}/testfile "test" 20 0600)

check_file_exists(${DIR}/testfile)

finish()

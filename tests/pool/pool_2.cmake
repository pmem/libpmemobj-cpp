# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test properly create and double close
execute(${TEST_EXECUTABLE} d ${DIR}/testfile "test" 20 0600)

check_file_exists(${DIR}/testfile)

finish()

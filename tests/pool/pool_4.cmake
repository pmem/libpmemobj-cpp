# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# failed to create, non-existent dir
execute(${TEST_EXECUTABLE} c ${DIR}/non-existent-dir/testfile "test" 20 0600)

check_file_doesnt_exist(${DIR}/non-existent-dir/testfile)

finish()

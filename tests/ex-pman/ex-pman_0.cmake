# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute_process(COMMAND tty OUTPUT_FILE /dev/null RESULT_VARIABLE result)

if (result EQUAL 0)
	execute(${SRC_DIR}/prepare_input.sh ${DIR}/input)
	execute(${TEST_EXECUTABLE} ${DIR}/testfile ${SRC_DIR}/../../examples/pman/map INPUT_FILE ${DIR}/input)
else()
	message(WARNING "Skip: stdout is not terminal")
endif()

finish()

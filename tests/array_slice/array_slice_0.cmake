# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

if(${TRACER} STREQUAL pmemcheck)
	set(IS_PMEMCHECK "1")
else()
	set(IS_PMEMCHECK "0")
endif()

execute(${TEST_EXECUTABLE} ${DIR}/testfile "${IS_PMEMCHECK}")

finish()

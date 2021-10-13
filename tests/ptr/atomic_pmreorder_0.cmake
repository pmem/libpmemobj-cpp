# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} c ${DIR}/testfile)
pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} m ${DIR}/testfile)
# negative test, should fail, uses mock of atomic_self_relative_ptr
# pmreorder_execute(true NoReorderNoCheck "PMREORDER_MARKER=NoReorderDoCheck" ${TEST_EXECUTABLE} n)
# pmreorder_execute(true NoReorderNoCheck "PMREORDER_MARKER=ReorderAccumulative" ${TEST_EXECUTABLE} n)
pmreorder_execute(true ReorderReverseAccumulative PMREORDER_MARKER=ReorderFull ${TEST_EXECUTABLE} n)
pmreorder_execute(true ReorderReverseAccumulative /home/luke/repos/luke/libpmemobj-cpp/tests/ptr/pmreorder.conf ${TEST_EXECUTABLE} n)

finish()

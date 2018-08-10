#
# Copyright 2018, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(DIR ${PARENT_DIR}/${TEST_NAME})

function(setup)
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${PARENT_DIR}/${TEST_NAME})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PARENT_DIR}/${TEST_NAME})
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIN_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR})
endfunction()

# Performs cleanup and log matching.
function(finish)
    message(STATUS "Test ${name}:")
    if(EXISTS ${BIN_DIR}/${TRACER}_${TESTCASE}.out)
        file(READ ${BIN_DIR}/${TRACER}_${TESTCASE}.out OUT)
        message(STATUS "Stdout:\n${OUT}")
    endif()
    if(EXISTS ${BIN_DIR}/${TRACER}_${TESTCASE}.err)
        file(READ ${BIN_DIR}/${TRACER}_${TESTCASE}.err ERR)
        message(STATUS "Stderr:\n${ERR}")
    endif()

    if(EXISTS ${SRC_DIR}/${TRACER}_${TESTCASE}.err.match)
        match(${BIN_DIR}/${TRACER}_${TESTCASE}.err ${SRC_DIR}/${TRACER}_${TESTCASE}.err.match)
    endif()
    if(EXISTS ${SRC_DIR}/${TRACER}_${TESTCASE}.out.match)
        match(${BIN_DIR}/${TRACER}_${TESTCASE}.out ${SRC_DIR}/${TRACER}_${TESTCASE}.out.match)
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${PARENT_DIR}/${TEST_NAME})
endfunction()

# Verifies ${log_file} matches ${match_file} using "match".
function(match log_file match_file)
    execute_process(COMMAND
            ${PERL_EXECUTABLE} ${MATCH_SCRIPT} -o ${log_file} ${match_file}
            RESULT_VARIABLE MATCH_ERROR)

    if(MATCH_ERROR)
        message(FATAL_ERROR "Log does not match: ${MATCH_ERROR}")
    endif()
endfunction()

# Verifies file exists
function(check_file_exists file)
    if(NOT EXISTS ${file})
        message(FATAL_ERROR "${file} doesn't exist")
    endif()
endfunction()

function(execute_common output_file name)
    if(NOT EXISTS ${name})
        message(FATAL_ERROR "Tests were not build! Run make first.")
    endif()

    if(TESTS_USE_FORCED_PMEM)
        set(ENV{PMEM_IS_PMEM_FORCE} 1)
    endif()

    if(${TRACER} STREQUAL pmemcheck)
        if(TESTS_USE_FORCED_PMEM)
            # pmemcheck runs really slow with pmem, disable it
            set(ENV{PMEM_IS_PMEM_FORCE} 0)
        endif()
        set(TRACE valgrind --error-exitcode=99 --tool=pmemcheck)
    elseif(${TRACER} STREQUAL memcheck)
        set(TRACE valgrind --error-exitcode=99 --tool=memcheck --leak-check=full
           --suppressions=${SRC_DIR}/../ld.supp --suppressions=${SRC_DIR}/../memcheck-stdcpp.supp)
    elseif(${TRACER} STREQUAL helgrind)
        set(TRACE valgrind --error-exitcode=99 --tool=helgrind)
    elseif(${TRACER} STREQUAL drd)
        set(TRACE valgrind --error-exitcode=99 --tool=drd)
    elseif(${TRACER} STREQUAL kgdb)
        set(TRACE konsole -e cgdb --args)
    elseif(${TRACER} MATCHES "none.*")
        # nothing
    else()
        message(FATAL_ERROR "Unknown tracer '${TRACER}'")
    endif()

    string(REPLACE ";" " " TRACE_STR "${TRACE}")
    message(STATUS "Executing: ${TRACE_STR} ${name} ${ARGN}")

    if(${output_file} STREQUAL none)
        execute_process(COMMAND ${TRACE} ${name} ${ARGN}
            OUTPUT_QUIET
            RESULT_VARIABLE res)
    else()
        execute_process(COMMAND ${TRACE} ${name} ${ARGN}
            RESULT_VARIABLE res
            OUTPUT_FILE ${BIN_DIR}/${output_file}.out
            ERROR_FILE ${BIN_DIR}/${output_file}.err)
    endif()

    if(res)
	    message(FATAL_ERROR "${TRACE} ${name} ${ARGN} failed: ${res}")
    endif()

    if(TESTS_USE_FORCED_PMEM)
        unset(ENV{PMEM_IS_PMEM_FORCE})
    endif()
endfunction()

# Generic command executor which handles failures and prints command output
# to specified file.
function(execute_with_output out name)
    execute_common(${out} ${name} ${ARGN})
endfunction()

# Generic command executor which handles failures but ignores output.
function(execute_ignore_output name)
    execute_common(none ${name} ${ARGN})
endfunction()

# Executes test command ${name} and verifies its status.
# First argument of the command is test directory name.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(execute name)
    execute_common(${TRACER}_${TESTCASE} ${name} ${ARGN})
endfunction()

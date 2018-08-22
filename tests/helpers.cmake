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
    message(STATUS "Test ${TEST_NAME}:")
    if(EXISTS ${BIN_DIR}/${TEST_NAME}.out)
        file(READ ${BIN_DIR}/${TEST_NAME}.out OUT)
        message(STATUS "Stdout:\n${OUT}")
    endif()
    if(EXISTS ${BIN_DIR}/${TEST_NAME}.err)
        file(READ ${BIN_DIR}/${TEST_NAME}.err ERR)
        message(STATUS "Stderr:\n${ERR}")
    endif()

    if(EXISTS ${SRC_DIR}/${TEST_NAME}.err.match)
        match(${BIN_DIR}/${TEST_NAME}.err ${SRC_DIR}/${TEST_NAME}.err.match)
    endif()
    if(EXISTS ${SRC_DIR}/${TEST_NAME}.out.match)
        match(${BIN_DIR}/${TEST_NAME}.out ${SRC_DIR}/${TEST_NAME}.out.match)
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

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=810295
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=780173
# https://bugs.kde.org/show_bug.cgi?id=303877
#
# valgrind issues an unsuppressable warning when exceeding
# the brk segment, causing matching failures. We can safely
# ignore it because malloc() will fallback to mmap() anyway.
#
# list of ingored warnings should match with the list provided by PMDK:
# https://github.com/pmem/pmdk/blob/master/src/test/unittest/unittest.sh
function(valgrind_ignore_warnings valgrind_log)
    execute_process(COMMAND bash "-c" "cat ${valgrind_log} | grep -v \
    -e \"WARNING: Serious error when reading debug info\" \
    -e \"When reading debug info from \" \
    -e \"Ignoring non-Dwarf2/3/4 block in .debug_info\" \
    -e \"Last block truncated in .debug_info; ignoring\" \
    -e \"parse_CU_Header: is neither DWARF2 nor DWARF3 nor DWARF4\" \
    -e \"brk segment overflow\" \
    -e \"see section Limitations in user manual\" \
    -e \"Warning: set address range perms: large range\"\
    -e \"further instances of this message will not be shown\"\
    >  ${valgrind_log}.tmp
mv ${valgrind_log}.tmp ${valgrind_log}")
endfunction()

function(execute_common output_file name)
    if(NOT EXISTS ${name})
        message(FATAL_ERROR "Tests were not found! If not built, run make first.")
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
           --suppressions=${SRC_DIR}/../ld.supp --suppressions=${SRC_DIR}/../memcheck-stdcpp.supp --suppressions=${SRC_DIR}/../memcheck-libunwind.supp)
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

    if (NOT (${TRACER} STREQUAL kgdb))
        if (NOT WIN32)
            set(TRACE timeout -s SIGALRM -k 200s 180s ${TRACE})
        endif()
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
            OUTPUT_FILE ${BIN_DIR}/${TEST_NAME}.out
            ERROR_FILE ${BIN_DIR}/${TEST_NAME}.err)
    endif()

    # memcheck and pmemcheck match files should follow name pattern:
    # testname_testcasenr_memcheck/pmemcheck.err.match
    # If they do exist, ignore test result - it will be verified during
    # log matching in finish() function.
    if(EXISTS ${SRC_DIR}/${TEST_NAME}.err.match)
        valgrind_ignore_warnings(${BIN_DIR}/${TEST_NAME}.err)
    # pmemcheck is a special snowflake and it doesn't set exit code when
    # it detects an error, so we have to look at its output if match file
    # was not found.
    elseif(${TRACER} STREQUAL pmemcheck)
        if(NOT EXISTS ${BIN_DIR}/${TEST_NAME}.err)
            message(FATAL_ERROR "${TEST_NAME}.err not found.")
        endif()

        file(READ ${BIN_DIR}/${TEST_NAME}.err PMEMCHECK_ERR)
        message(STATUS "Stderr:\n${PMEMCHECK_ERR}")
        if(NOT PMEMCHECK_ERR MATCHES "ERROR SUMMARY: 0")
            message(FATAL_ERROR "Test executed with error(s).")
        endif()
    elseif(res)
        if(EXISTS ${BIN_DIR}/${TEST_NAME}.out)
            file(READ ${BIN_DIR}/${TEST_NAME}.out OUT)
            message(STATUS "Stdout:\n${OUT}\nEnd of stdout")
        endif()
        if(EXISTS ${BIN_DIR}/${TEST_NAME}.err)
            file(READ ${BIN_DIR}/${TEST_NAME}.err ERR)
            message(STATUS "Stderr:\n${ERR}\nEnd of stderr")
        endif()

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

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2019, Intel Corporation

include(FindDoxygen)
if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpmemobj++.Doxyfile.in"
		       "${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.Doxyfile" @ONLY)

	add_custom_target(doc ALL ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.Doxyfile"
			  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cpp_html/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
elseif(NOT DOXYGEN_FOUND)
	message(WARNING "Doxygen not found - documentation will not be generated")
else()
	message(WARNING "Dot tool not found - documentation will not be generated")
endif()

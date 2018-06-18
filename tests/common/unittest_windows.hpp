/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UNITTEST_WINDOWS_HPP
#define UNITTEST_WINDOWS_HPP

#include <windows.h>

#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IRGRP S_IRUSR
#define S_IWGRP S_IWUSR

/*
 * ut_toUTF8 -- convert WCS to UTF-8 string
 */
char *
ut_toUTF8(const wchar_t *wstr)
{
	int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, -1,
				       nullptr, 0, nullptr, nullptr);
	if (size == 0) {
		UT_FATAL("!ut_toUTF8");
	}

	char *str = new char[size];

	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr, -1, str,
				size, nullptr, nullptr) == 0) {
		UT_FATAL("!ut_toUTF8");
	}

	return str;
}

/*
 * ut_statw -- statw that never returns -1
 */
int
ut_statw(const char *file, int line, const char *func, const wchar_t *path,
	 os_stat_t *st)
{
	int ret = _wstat64(path, st);

	if (ret < 0)
		UT_FATAL("%s:%d %s - !stat: %S", file, line, func, path);

	/* clear unused bits to avoid confusion */
	st->st_mode &= 0600;

	return ret;
}

#define STATW(path, st) ut_statw(__FILE__, __LINE__, __func__, path, st)

#endif /* UNITTEST_WINDOWS_HPP */

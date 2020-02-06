// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018, Intel Corporation */

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

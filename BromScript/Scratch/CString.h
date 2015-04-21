/*  libscratch - Multipurpose objective C++ library.
	Copyright (C) 2012 - 2013  Angelo Geels

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef SCRATCH_CSTRING_H_INCLUDED
#define SCRATCH_CSTRING_H_INCLUDED 1

#ifdef USE_PRAGMAONCE
#pragma once
#endif

#include "CStackArray.h"

#define CSTRING_FORMAT_BUFFER_SIZE 255

SCRATCH_NAMESPACE_BEGIN;

class CString {
public:
	char* str_szBuffer;
	static char* str_szEmpty;

	CString();
	CString(double number);
	CString(char szValue);
	CString(const char* szValue);
	CString(CString* szValue);
	CString(const CString &strCopy);
	CString(CString&& strMove);
	~CString();

	static CString Format(const char* strFormat, ...);

	void SetF(const char* strFormat, ...);
	void AppendF(const char* strFormat, ...);
	void CopyToBuffer(const char* szSrc);
	void AppendToBuffer(const char* szSrc);
	void AppendToBuffer(const char* szSrc, int iCount);
	void AppendToBuffer(char cSrc);

	void Split(const CString &strNeedle, CStackArray<CString> &astrResult);
	int Count(const CString &strNeedle);
	CString Trim();
	CString TrimLeft();
	CString TrimRight();
	CString Trim(char c);
	CString TrimLeft(char c);
	CString TrimRight(char c);
	CString Replace(const CString &strNeedle, const CString &strReplace);
	CString Reverse();
	CString ToLower();
	CString ToUpper();
	CString Substring(int start);
	CString Substring(int start, int len);

	int Size();
	int IndexOf(char c);
	int IndexOf(char c, int startpos);
	int LastIndexOf(char c);
	int LastIndexOf(char c, int startpos);
	int IndexOf(CString text, int startpos);
	int LastIndexOf(CString text, int startpos);

	bool Contains(const CString &strNeedle);
	bool StartsWith(const CString &strNeedle);
	bool EndsWith(const CString &strNeedle);
	bool StartsWith(const char strNeedle);
	bool EndsWith(const char strNeedle);

	operator const char*();
	operator const char*() const;

	CString& operator=(char* szSrc);
	CString& operator=(const char* szSrc);
	CString& operator=(const CString &strSrc);

	CString& operator+=(const char* szSrc);
	CString& operator+=(const char cSrc);

	bool operator==(const char* szSrc);
	bool operator!=(const char* szSrc);

	char& operator[](int iIndex);
};

CString operator+(CString &strLHS, const char* szRHS);
CString operator+(CString &strLHS, const char cRHS);

SCRATCH_NAMESPACE_END;
#endif // include once check

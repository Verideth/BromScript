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

#ifndef SCRATCH_CFILESTREAM_H_INCLUDED
#define SCRATCH_CFILESTREAM_H_INCLUDED 1

#ifdef USE_PRAGMAONCE
#pragma once
#endif

#include "Common.h"
#include "CStream.h"
#include "CString.h"

SCRATCH_NAMESPACE_BEGIN;
class CFileStream {
public:
	CString fs_strFileName;
	FILE* fs_pfh;

public:
	CFileStream(void);
	~CFileStream(void);

	unsigned long Size();
	unsigned long Location();
	void Seek(unsigned long ulPos, int iOrigin);

	bool Open(const char* szFileName, const char* szMode);
	void Close();
	void Write(const void* p, unsigned long iLen);
	void* Read(unsigned long iLen);
	const void* ReadToEnd(void);
};

SCRATCH_NAMESPACE_END;
#endif

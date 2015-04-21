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

#ifndef SCRATCH_CSTREAM_H_INCLUDED
#define SCRATCH_CSTREAM_H_INCLUDED 1

#ifdef USE_PRAGMAONCE
#pragma once
#endif

#include "Common.h"
#include "CString.h"

SCRATCH_NAMESPACE_BEGIN;
enum ENewLineMode {
	ENLM_CRLF,
	ENLM_LF,
	ENLM_CR,
};

class CStream {
public:
	ENewLineMode strm_nlmNewLineMode;

public:
	CStream(void);
	~CStream(void);

	virtual unsigned long Size() = 0;
	virtual unsigned long Location() = 0;
	virtual void Seek(unsigned long ulPos, int iOrigin) = 0;

	virtual void Close();

	virtual void Write(const void* p, unsigned long iLen) = 0;
	inline void WriteIndex(const int &i) { Write(&i, sizeof(int)); }
	inline void WriteLong(const long &l) { Write(&l, sizeof(long)); }
	inline void WriteFloat(const float &f) { Write(&f, sizeof(float)); }
	inline void WriteDouble(const double &d) { Write(&d, sizeof(double)); }
	void WriteString(const CString &str);
	void WriteStream(CStream &strm);

	virtual void* Read(unsigned long iLen) = 0;
	void* ReadToEnd(void);
	inline int  ReadIndex(void) { return *(int*)Read(sizeof(int)); }
	inline long   ReadLong(void) { return *(long*)Read(sizeof(long)); }
	inline float  ReadFloat(void) { return *(float*)Read(sizeof(float)); }
	inline double ReadDouble(void) { return *(double*)Read(sizeof(double)); }
	CString ReadString(void);

	void WriteLine(const CString &str);
	CString ReadLine(void);

	inline CStream& operator <<(int i) { WriteIndex(i); return *this; }
	inline CStream& operator <<(long l) { WriteLong(l); return *this; }
	inline CStream& operator <<(float f) { WriteFloat(f); return *this; }
	inline CStream& operator <<(double d) { WriteDouble(d); return *this; }
	inline CStream& operator <<(CString str) { WriteString(str); return *this; }
	inline CStream& operator <<(CStream &strm) { WriteStream(strm); return *this; }

	inline CStream& operator >>(int &i) { i = ReadIndex(); return *this; }
	inline CStream& operator >>(long &l) { l = ReadLong(); return *this; }
	inline CStream& operator >>(float &f) { f = ReadFloat(); return *this; }
	inline CStream& operator >>(double &d) { d = ReadDouble(); return *this; }
	inline CStream& operator >>(CString &str) { str = ReadString(); return *this; }
};

SCRATCH_NAMESPACE_END;
#endif

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

#include "CFileStream.h"
SCRATCH_NAMESPACE_BEGIN;
CFileStream::CFileStream(void) :fs_pfh(NULL) {
}

CFileStream::~CFileStream(void) {
	Close();
}

unsigned long CFileStream::Size() {
	unsigned long ulPos = Location();
	Seek(0, SEEK_END);
	unsigned long ulSize = Location();
	Seek(ulPos, SEEK_SET);
	return ulSize;
}

unsigned long CFileStream::Location() {
	return ftell(fs_pfh);
}

void CFileStream::Seek(unsigned long ulPos, int iOrigin) {
	fseek(fs_pfh, ulPos, iOrigin);
}

bool CFileStream::Open(const char* szFileName, const char* szMode) {
	// must not already have a handle open
	if (fs_pfh != NULL) return false;

	// open file
	FILE* pfh = fopen(szFileName, szMode);

	// it might not exist
	if (pfh == NULL) return false;

	// remember info
	fs_strFileName = szFileName;
	fs_pfh = pfh;
	return true;
}

void CFileStream::Close(void) {
	// close the file handle
	if (fs_pfh != NULL) {
		fclose(fs_pfh);
		fs_pfh = NULL;
	}
}

void CFileStream::Write(const void* p, unsigned long iLen) {
	fwrite(p, 1, iLen, fs_pfh);
}

void* CFileStream::Read(unsigned long iLen) {
	void* pBuffer = new char[iLen];
	fread(pBuffer, 1, iLen, fs_pfh);
	return pBuffer;
}

const void* CFileStream::ReadToEnd() {
	if (fs_pfh == NULL) return 0;
	return Read(Size() - Location());
}
SCRATCH_NAMESPACE_END;
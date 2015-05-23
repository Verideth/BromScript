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

#include <cstdio> // for vsprintf
#include <cstdarg> // for va_list
#include <cstring> // for strlen and strcmp

#include "CString.h"

SCRATCH_NAMESPACE_BEGIN;
extern int str_iInstances = 0;

char* CString::str_szEmpty = "";

void CString::CopyToBuffer(const char* szSrc) {
	if (this->str_szBuffer != CString::str_szEmpty)
		delete[] this->str_szBuffer;

	int len = strlen(szSrc);
	if (len == 0) {
		this->str_szBuffer = CString::str_szEmpty;
		return;
	}

	len++; // copy the null along
	this->str_szBuffer = new char[len];
	memcpy(this->str_szBuffer, szSrc, len);
}

void CString::AppendToBuffer(const char* szSrc) {
	size_t iCount = strlen(szSrc);
	if (iCount <= 0)
		return;

	int selfsize = this->Size();

	char* newbuff = new char[selfsize + iCount + 1];
	memcpy(newbuff, this->str_szBuffer, selfsize);
	memcpy(newbuff + selfsize, szSrc, iCount);
	newbuff[selfsize + iCount] = 0;

	if (this->str_szBuffer != CString::str_szEmpty)
		delete[] this->str_szBuffer;

	this->str_szBuffer = newbuff;
}

void CString::AppendToBuffer(const char* szSrc, int iCount) {
	if (iCount <= 0)
		return;

	int selfsize = this->Size();

	char* newbuff = new char[selfsize + iCount + 1];
	memcpy(newbuff, this->str_szBuffer, selfsize);
	memcpy(newbuff + selfsize, szSrc, iCount);
	newbuff[selfsize + iCount] = 0;

	if (this->str_szBuffer != CString::str_szEmpty)
		delete[] this->str_szBuffer;

	this->str_szBuffer = newbuff;
}

void CString::AppendToBuffer(char cSrc) {
	int selfsize = this->Size();

	char* newbuff = new char[selfsize + 2];
	memcpy(newbuff, this->str_szBuffer, selfsize);
	newbuff[selfsize] = cSrc;
	newbuff[++selfsize] = 0;

	if (this->str_szBuffer != CString::str_szEmpty)
		delete[] this->str_szBuffer;

	this->str_szBuffer = newbuff;
}

CString::CString() :str_szBuffer(CString::str_szEmpty) {
}

CString::CString(CString&& strMove) : str_szBuffer(strMove.str_szBuffer) {
	strMove.str_szBuffer = CString::str_szEmpty;
}

CString::CString(double number) : str_szBuffer(CString::str_szEmpty) {
	char buffer[255];
	sprintf(buffer, "%f", number);

	this->CopyToBuffer(buffer);

	this->TrimRight('0');
	this->Trim('.');
}

CString::CString(CString* szStr) :str_szBuffer(CString::str_szEmpty) {
	this->CopyToBuffer(szStr->str_szBuffer);
}

CString::CString(const char* szStr) : str_szBuffer(CString::str_szEmpty) {
	this->CopyToBuffer(szStr);
}

CString::CString(char szStr) : str_szBuffer(CString::str_szEmpty) {
	this->str_szBuffer = new char[2];
	this->str_szBuffer[0] = szStr;
	this->str_szBuffer[1] = 0;
}

CString::CString(const CString &copy) :str_szBuffer(CString::str_szEmpty) {
	this->CopyToBuffer(copy);
}

CString CString::Format(const char* format, ...) {
	int size = 512;
	char* buffer = 0;
	buffer = new char[size];

	va_list vl;
	va_start(vl, format);

	int nsize = vsnprintf(buffer, size, format, vl);
	if (size <= nsize) { //fail delete buffer and try again
		delete[] buffer;

		buffer = new char[nsize + 1]; //+1 for /0
		nsize = vsnprintf(buffer, size, format, vl);
	}

	va_end(vl);

	CString ret(buffer);
	delete[] buffer;

	return ret;
}

CString::~CString() {
	if (this->str_szBuffer != CString::str_szEmpty) {
		delete[] this->str_szBuffer;
	}
}

CString CString::Substring(int start) {
	return this->Substring(start, this->Size() - start);
}

CString CString::Substring(int iStart, int iLen) {
	if (iStart >= strlen(this->str_szBuffer))
		return "";

	if (iStart < 0) {
		iLen += iStart;
		iStart = 0;

		if (iLen <= 0)
			return "";
	}

	// Get the first offset
	CString strRet(this->str_szBuffer + iStart);

	if (iLen < 0)
		iLen = strRet.Size() + iLen;

	// Check for stupid developers
	if (iLen >= strlen(strRet)) {
		strRet.AppendToBuffer('\0');
		return strRet;
	}

	// Then set the null terminator at the length the user wants
	strRet[iLen] = '\0';

	// Return
	return strRet;
}
int CString::Size() {
	return strlen(this->str_szBuffer);
}

void CString::SetF(const char* szFormat, ...) {
	int size = 512;
	char* buffer = 0;
	buffer = new char[size];

	va_list vl;
	va_start(vl, szFormat);

	int nsize = vsnprintf(buffer, size, szFormat, vl);
	if (size <= nsize) { //fail delete buffer and try again
		delete[] buffer;

		buffer = new char[nsize + 1]; //+1 for /0
		nsize = vsnprintf(buffer, size, szFormat, vl);
	}

	va_end(vl);

	// Copy the just-created buffer to the main buffer
	this->CopyToBuffer(buffer);

	// Clean up
	delete[] buffer;
}

void CString::AppendF(const char* szFormat, ...) {
	int size = 512;
	char* buffer = 0;
	buffer = new char[size];

	va_list vl;
	va_start(vl, szFormat);

	int nsize = vsnprintf(buffer, size, szFormat, vl);
	if (size <= nsize) { //fail delete buffer and try again
		delete[] buffer;

		buffer = new char[nsize + 1]; //+1 for /0
		nsize = vsnprintf(buffer, size, szFormat, vl);
	}

	va_end(vl);

	// Copy the just-created buffer to the main buffer
	this->AppendToBuffer(buffer);

	// Clean up
	delete[] buffer;
}

void CString::Split(const CString &strNeedle, CStackArray<CString> &astrResult) {
	// Keep a pointer to the current offset and a "previous offset"
	char* szOffset = str_szBuffer;
	char* szOffsetPrev = szOffset;

	do {
		// Find the needle from the string in the current offset pointer
		szOffset = strstr(szOffset, strNeedle);

		// If the needle is found
		if (szOffset != NULL) {
			// Get the length for the string
			int iLen = szOffset - szOffsetPrev;

			// And get a buffer started
			char* szPart = new char[iLen + 1];

			// Copy over the characters to the part buffer
			int i = 0;
			for (; i < iLen; i++) {
				szPart[i] = *(szOffset - (iLen - i));
			}
			szPart[i] = '\0';

			// Add it to the return array
			astrResult.Push() = szPart;
			delete[] szPart;

			// Increase the offset pointer by the needle length
			szOffset += strlen(strNeedle);

			// Keep track of the pointer
			szOffsetPrev = szOffset;
		} else {
			// Get the length for the string
			int iLen = strlen(szOffsetPrev);

			// And get a buffer started
			char* szPart = new char[iLen + 1];

			// Copy over the characters to the part buffer
			int i = 0;
			for (; i < iLen; i++) {
				szPart[i] = szOffsetPrev[i];
			}
			szPart[i] = '\0';

			// Add it to the return vector
			astrResult.Push() = szPart;
			delete[] szPart;
		}
	} while (szOffset != NULL);
}

CString CString::Trim() {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// While there's a space, keep incrementing the offset
	while (*szOffset == ' ' || *szOffset == '\n' || *szOffset == '\r' || *szOffset == '\t') {
		// This way, we'll trim all the spaces on the left
		szOffset++;
	}

	// Loop from right to left in the string
	for (int i = strlen(szOffset) - 1; i >= 0; i--) {
		// When we find something other than a space
		if (szOffset[i] != ' ' && szOffset[i] != '\n' && szOffset[i] != '\r' && szOffset[i] != '\t') {
			// Put a null terminator to trim the right part
			szOffset[i + 1] = '\0';

			// Stop reading
			break;
		}
	}

	// Return
	return CString(szOffset);
}

CString CString::TrimLeft() {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// While there's a space, keep incrementing the offset
	while (*szOffset == ' ' || *szOffset == '\n' || *szOffset == '\r' || *szOffset == '\t') {
		// This way, we'll trim all the spaces on the left
		szOffset++;
	}

	// Return
	return CString(szOffset);
}

CString CString::TrimRight() {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// Loop from right to left in the string
	for (int i = strlen(szOffset) - 1; i >= 0; i--) {
		// When we find something other than a space
		if (szOffset[i] != ' ' && szOffset[i] != '\n' && szOffset[i] != '\r' && szOffset[i] != '\t') {
			// Put a null terminator to trim the right part
			szOffset[i + 1] = '\0';

			// Stop reading
			break;
		}
	}

	// Return
	return CString(szOffset);
}

CString CString::TrimLeft(char c) {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// While there's a space, keep incrementing the offset
	while (*szOffset == c) {
		// This way, we'll trim all the spaces on the left
		szOffset++;
	}

	// Return
	return CString(szOffset);
}

CString CString::TrimRight(char c) {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// Loop from right to left in the string
	for (int i = strlen(szOffset) - 1; i >= 0; i--) {
		// When we find something other than a space
		if (szOffset[i] != c) {
			// Put a null terminator to trim the right part
			szOffset[i + 1] = '\0';

			// Stop reading
			break;
		}
	}

	// Return
	return CString(szOffset);
}

CString CString::Trim(char c) {
	// Keep a pointer to the current offset
	char* szOffset = this->str_szBuffer;

	// While there's a space, keep incrementing the offset
	while (*szOffset == c) {
		// This way, we'll trim all the spaces on the left
		szOffset++;
	}

	// Loop from right to left in the string
	for (int i = strlen(szOffset) - 1; i >= 0; i--) {
		// When we find something other than a space
		if (szOffset[i] != c) {
			// Put a null terminator to trim the right part
			szOffset[i + 1] = '\0';

			// Stop reading
			break;
		}
	}

	// Return
	return CString(szOffset);
}

CString CString::Replace(const CString &strNeedle, const CString &strReplace) {
	CString strRet("");

	// Keep a pointer to the current offset and a "previous offset"
	char* szOffset = this->str_szBuffer;
	char* szOffsetPrev = szOffset;

	do {
		// Find the offset of the needle
		szOffset = strstr(szOffset, strNeedle);

		// If it's found
		if (szOffset != NULL) {
			// Append everything before the needle of the original characters to the return value
			strRet.AppendToBuffer(szOffsetPrev, szOffset - szOffsetPrev);

			// Append the replace value
			strRet += strReplace;

			// Increase the offset pointer by the needle length
			szOffset += strlen(strNeedle);

			// Keep track of the pointer
			szOffsetPrev = szOffset;
		} else {
			// Append the remaining part of the source string
			strRet.AppendToBuffer(szOffsetPrev, strlen(szOffsetPrev));
		}
	} while (szOffset != NULL);

	return strRet;
}

int CString::Count(const CString &strNeedle) {
	int ret = 0;

	// Keep a pointer to the current offset and a "previous offset"
	char* szOffset = this->str_szBuffer;
	char* szOffsetPrev = szOffset;

	do {
		// Find the offset of the needle
		szOffset = strstr(szOffset, strNeedle);

		// If it's found
		if (szOffset != NULL) {
			ret++;
			szOffset += strlen(strNeedle);
			szOffsetPrev = szOffset;
		}
	} while (szOffset != NULL);

	return ret;
}

CString CString::Reverse() {
	CString strRet(this->str_szBuffer);

#ifdef _MSC_VER
	strrev(strRet.str_szBuffer);
#else // lol linux
	char *p1, *p2;

	for (p1 = strRet.str_szBuffer, p2 = strRet.str_szBuffer + strlen(strRet.str_szBuffer) - 1; p2 > p1; ++p1, --p2){
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
#endif

	return strRet;
}

CString CString::ToLower() {
	CString strRet(this->str_szBuffer);

#ifdef _MSC_VER
	strlwr(strRet.str_szBuffer);
#else // lol linux
	char* string = this->str_szBuffer;
	while(*string){
		if (*string >= 'A' && *string <= 'Z'){
			*string = *string + 32;
		}

		string++;
	}
#endif

	return strRet;
}

CString CString::ToUpper() {
	CString strRet(this->str_szBuffer);

#ifdef _MSC_VER
	strupr(strRet.str_szBuffer);
#else // lol linux
	char* string = this->str_szBuffer;
	while(*string){
		if (*string >= 'a' && *string <= 'z'){
			*string = *string - 32;
		}

		string++;
	}
#endif

	return strRet;
}

int CString::IndexOf(char c) {
	int strlen = this->Size();
	for (int i = 0; i < strlen; i++) {
		if (this->str_szBuffer[i] == c) {
			return i;
		}
	}

	return -1;
}

int CString::IndexOf(char c, int spos) {
	int strlen = this->Size();
	for (int i = spos; i < strlen; i++) {
		if (this->str_szBuffer[i] == c) {
			return i;
		}
	}

	return -1;
}

int CString::LastIndexOf(char c) {
	int strlen = this->Size();
	for (int i = strlen - 1; i > -1; i--) {
		if (this->str_szBuffer[i] == c) {
			return i;
		}
	}

	return -1;
}

int CString::LastIndexOf(char c, int spos) {
	int strlen = this->Size();
	for (int i = spos; i > -1; i--) {
		if (this->str_szBuffer[i] == c) {
			return i;
		}
	}

	return -1;
}

int CString::IndexOf(CString text, int spos) {
	int strlen = this->Size();
	int textlen = text.Size();

	for (int i = spos; i <= strlen - textlen; i++) {
		if (CString(this->str_szBuffer).Substring(i, textlen) == text) {
			return i;
		}
	}

	return -1;
}

int CString::LastIndexOf(CString text, int spos) {
	int strlen = this->Size();
	int textlen = text.Size();

	for (int i = spos; i > -1; i--) {
		if (CString(this->str_szBuffer).Substring(i, textlen) == text) {
			return i;
		}
	}

	return -1;
}

bool CString::Contains(const CString &strNeedle) {
	return strstr(this->str_szBuffer, strNeedle) != NULL;
}

bool CString::StartsWith(const char strNeedle) {
	return strNeedle == this->str_szBuffer[0];
}

bool CString::StartsWith(const CString &strNeedle) {
	return strstr(this->str_szBuffer, strNeedle) == this->str_szBuffer;
}

bool CString::EndsWith(const char strNeedle) {
	return strNeedle == this->str_szBuffer[strlen(this->str_szBuffer) - 1];
}

bool CString::EndsWith(const CString &strNeedle) {
	CString* comp = (CString*)&strNeedle;

	int ss = this->Size();
	int cs = comp->Size();

	if (cs > ss) return false;

	return this->Substring(ss - cs) == strNeedle;
}

CString::operator const char *() {
	return this->str_szBuffer;
}

CString::operator const char *() const {
	return this->str_szBuffer;
}

CString& CString::operator=(char* src) {
	// Copy the right hand side to the buffer.
	this->CopyToBuffer(src);
	return *this;
}

CString& CString::operator=(const char* src) {
	// Copy the right hand side to the buffer.
	this->CopyToBuffer(src);
	return *this;
}

CString& CString::operator=(const CString &strSrc) {
	// If the right hand side is not the left hand side...
	if (this != &strSrc) {
		// Copy the right hand side to the buffer.
		this->CopyToBuffer(strSrc);
	}
	return *this;
}

CString& CString::operator+=(const char* szSrc) {
	// Append the right hand side to the buffer.
	this->AppendToBuffer(szSrc);
	return *this;
}

CString& CString::operator+=(const char cSrc) {
	// Append the right hand side to the buffer.
	this->AppendToBuffer(cSrc);
	return *this;
}

bool CString::operator==(const char* szSrc) {
	return !strcmp(this->str_szBuffer, szSrc);
}

bool CString::operator!=(const char* szSrc) {
	return strcmp(this->str_szBuffer, szSrc) != 0;
}

char& CString::operator[](int iIndex) {
	return this->str_szBuffer[iIndex];
}

CString operator+(const CString &strLHS, const char* szRHS) {
	return CString(strLHS) += szRHS;
}

CString operator+(const CString &strLHS, const char cRHS) {
	return CString(strLHS) += cRHS;
}


SCRATCH_NAMESPACE_END;

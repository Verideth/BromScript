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

#ifndef SCRATCH_CMEMORYSTREAM_H_INCLUDED
#define SCRATCH_CMEMORYSTREAM_H_INCLUDED 1

#ifdef USE_PRAGMAONCE
  #pragma once
#endif

#include "Common.h"
#include "CStream.h"

SCRATCH_NAMESPACE_BEGIN;
class CMemoryStream : public CStream
{
public:
  unsigned char* strm_pubBuffer;
  unsigned long strm_ulPosition;
  unsigned long strm_ulSize;
  unsigned long strm_ulUsed;

public:
  CMemoryStream(void);
  ~CMemoryStream(void);

  unsigned long Size();
  unsigned long Location();
  void Seek(unsigned long ulPos, int iOrigin);

  void Write(const void* p, unsigned long iLen);
  void* Read(unsigned long iLen);
  const void* ReadToEnd(void);

private:
  void AllocateMoreMemory(int ctBytes);
};
SCRATCH_NAMESPACE_END;

#endif

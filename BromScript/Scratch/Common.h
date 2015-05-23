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

#ifndef SCRATCH_COMMON_H_INCLUDED
#define SCRATCH_COMMON_H_INCLUDED

#ifdef USE_PRAGMAONCE
#pragma once
#endif

#include <cstdio>
#include <cmath>
#include <cassert>

#define SCRATCH_NAMESPACE_BEGIN namespace Scratch {
#define SCRATCH_NAMESPACE_END }

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SCRATCH_NO_GLOBFUNC
template<class T> inline T Max(const T &v1, const T &v2) { return v1 >= v2 ? v1 : v2; }
template<class T> inline T Min(const T &v1, const T &v2) { return v1 <= v2 ? v1 : v2; }
template<class T> inline T Swap(const T &v1, const T &v2) { T& t = v1; v1 = v2; v2 = t; }

template<class T> inline T Abs(const T &v) { return v < 0 ? -v : v; }
#endif

#ifdef _MSC_VER
#ifndef ASSERT
#define ASSERT(expr) {            \
  if(!(expr)) {                   \
  static bool bWasHere = false;   \
    if(!bWasHere) {               \
      _wassert(_CRT_WIDE(#expr),  \
        _CRT_WIDE(__FILE__),      \
        __LINE__);                \
      bWasHere = true;            \
	    }                             \
    }                               \
}
#endif
#else
#ifndef ASSERT
#define ASSERT(expr) assert(expr)
#endif
#endif

#endif // include once check

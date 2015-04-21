/*
	BromScript - On Runtime Scripting Language.
	Copyright (C) 2012 - 2015  Alex Brouwer (Bromvlieg: http://brom.4o3.nl/)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#ifndef BROMSCRIPT_USERDATA_INTEROP_INCLUDED
#define BROMSCRIPT_USERDATA_INTEROP_INCLUDED
#define BROMSCRIPT_USERDATA_INTEROP_TYPE 53

#include "../SIF.h"
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <cstdlib>
#include <dlfcn.h>
#endif


namespace BromScript{
	namespace Userdatas{
		namespace Interop{
			class CInterop {
			public:
				Scratch::CString Filename;
#ifdef _MSC_VER
				HMODULE LibHandle;
#else
				void* LibHandle;
#endif

				CInterop(Scratch::CString filename);
				~CInterop();

				void* GetMethod(Scratch::CString filename);
				void Close();
			};

			void RegisterUD(BromScript::Instance* bsi);
		}
	}
}

#endif
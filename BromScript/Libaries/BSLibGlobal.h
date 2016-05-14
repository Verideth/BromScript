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

#ifndef BROMSCRIPT_LIBARY_G_INCLUDED
#define BROMSCRIPT_LIBARY_G_INCLUDED

#include "../SIF.h"

namespace BromScript{
	namespace Libaries{
		namespace Global{
			BS_FUNCTION(Sleep);
			BS_FUNCTION(Environment);
			BS_FUNCTION(ToString);
			BS_FUNCTION(ToNumber);
			BS_FUNCTION(Print);
			BS_FUNCTION(TypeOf);
			BS_FUNCTION(Time);
			BS_FUNCTION(Call);
			BS_FUNCTION(Error);
			BS_FUNCTION(Include);
#ifdef _MSC_VER
			BS_FUNCTION(IncludeDll);
#else
			BS_FUNCTION(IncludeSO);
#endif
			BS_FUNCTION(SetReadOnly);
		}
	}
}

#endif
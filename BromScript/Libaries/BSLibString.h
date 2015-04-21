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

#ifndef BROMSCRIPT_LIBARY_STRING_INCLUDED
#define BROMSCRIPT_LIBARY_STRING_INCLUDED

#include "../SIF.h"

namespace BromScript{
	namespace Libaries{
		namespace String{
			BS_FUNCTION(ToChar);
			BS_FUNCTION(FromChar);
			BS_FUNCTION(Split);
			BS_FUNCTION(Sub);
			BS_FUNCTION(IndexOf);
			BS_FUNCTION(LastIndexOf);
			BS_FUNCTION(Left);
			BS_FUNCTION(Right);
			BS_FUNCTION(Trim);
			BS_FUNCTION(TrimLeft);
			BS_FUNCTION(TrimRight);
			BS_FUNCTION(Replace);
			BS_FUNCTION(ToLower);
			BS_FUNCTION(ToUpper);
		}
	}
}

#endif
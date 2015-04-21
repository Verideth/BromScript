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

#ifndef BROMSCRIPT_LIBARY_MATH_INCLUDED
#define BROMSCRIPT_LIBARY_MATH_INCLUDED

#include "../SIF.h"

namespace BromScript{
	namespace Libaries{
		namespace Math{
			BS_FUNCTION(Sin);
			BS_FUNCTION(Cos);
			BS_FUNCTION(Min);
			BS_FUNCTION(Max);
			BS_FUNCTION(Random);
			BS_FUNCTION(Floor);
			BS_FUNCTION(Ceiling);
			BS_FUNCTION(Round);
			BS_FUNCTION(Atan);
			BS_FUNCTION(Atan2);
			BS_FUNCTION(Tan);
			BS_FUNCTION(Pow);
			BS_FUNCTION(Sign);
			BS_FUNCTION(Sqrt);
			BS_FUNCTION(Abs);
			BS_FUNCTION(Neg);
			BS_FUNCTION(ToRadians);
			BS_FUNCTION(ToDegrees);
		}
	}
}

#endif
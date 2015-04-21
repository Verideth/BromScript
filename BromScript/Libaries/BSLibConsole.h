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

#ifndef BROMSCRIPT_LIBARY_CONSOLE_INCLUDED
#define BROMSCRIPT_LIBARY_CONSOLE_INCLUDED

#include "../SIF.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <iosfwd>

namespace BromScript{
	namespace Libaries{
		namespace Console{
#ifdef _MSC_VER
			enum Colors {
				ColorBlack = 0,
				ColorDarkBlue = 1,
				ColorDarkGreen = 2,
				ColorDarkCyan = 3,
				ColorDarkRed = 4,
				ColorDarkMagenta = 5,
				ColorDarkYellow = 6,
				ColorDarkWhite = 7,
				ColorGray = 8,
				ColorBlue = 9,
				ColorGreen = 10,
				ColorCyan = 11,
				ColorRed = 12,
				ColorMagenta = 13,
				ColorYellow = 14,
				ColorWhite = 15
			};
#else
			enum Colors {
				ColorBlack = 30,
				ColorRed = 31,
				ColorGreen = 32,
				ColorYellow = 33,
				ColorBlue = 34,
				ColorMagenta = 35,
				ColorCyan = 36,
				ColorGray = 37,
				ColorDefault = 39,
				ColorDarkGray = 90,
				ColorLightRed = 91,
				ColorLightGreen = 92,
				ColorLightYellow = 93,
				ColorLightBlue = 94,
				ColorLightMgenta = 95,
				ColorLightCyan = 96,
				ColorWhite = 97,
			};
#endif

#ifdef _MSC_VER
			static HANDLE std_con_out;
#endif

			static Colors textcol, backcol, deftextcol, defbackcol;
			static bool didinit = false;

			BS_FUNCTION(SetForeColor);
			BS_FUNCTION(SetBackColor);
			BS_FUNCTION(SetDefaultColors);
			BS_FUNCTION(Clear);
			BS_FUNCTION(GetLine);
		}
	}
}

#endif
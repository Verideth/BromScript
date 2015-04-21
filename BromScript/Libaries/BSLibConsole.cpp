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

#include "BSLibConsole.h"

#include "../SIF.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"
#include "../Scratch/CString.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <cstdlib>
#endif

#include <iosfwd>

using namespace Scratch;

namespace BromScript{
	namespace Libaries{
		namespace Console{

			void DoInit() {
				if (didinit) return;
				didinit = true;

#ifdef _MSC_VER
				std_con_out = GetStdHandle(STD_OUTPUT_HANDLE);

				CONSOLE_SCREEN_BUFFER_INFO csbi;
				GetConsoleScreenBufferInfo(std_con_out, &csbi);

				textcol = Colors(csbi.wAttributes & 15);
				backcol = Colors((csbi.wAttributes & 0xf0) >> 4);
#else
				textcol = Colors::ColorDefault;
				backcol = Colors::ColorDefault;
#endif

				deftextcol = textcol;
				defbackcol = backcol;
			}

			BS_FUNCTION(SetForeColor) {
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				DoInit();

				Colors col = (Colors)(int)args->GetNumber(0);
				textcol = col;

#ifdef _MSC_VER
				unsigned short wAttributes = ((unsigned int)backcol << 4) | (unsigned int)textcol;
				SetConsoleTextAttribute(std_con_out, wAttributes);
#else
				printf("\0[%dm", col);
#endif

				return null;
			}

			BS_FUNCTION(SetBackColor) {
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				DoInit();

				Colors col = (Colors)(int)args->GetNumber(0);
				backcol = col;

#ifdef _MSC_VER
				unsigned short wAttributes = ((unsigned int)backcol << 4) | (unsigned int)textcol;
				SetConsoleTextAttribute(std_con_out, wAttributes);
#else
				printf("\e[%dm", col + 10);
#endif

				return null;
			}

			BS_FUNCTION(SetDefaultColors) {
				DoInit();

				textcol = deftextcol;
				backcol = defbackcol;

#ifdef _MSC_VER
				unsigned short wAttributes = ((unsigned int)backcol << 4) | (unsigned int)textcol;
				SetConsoleTextAttribute(std_con_out, wAttributes);
#else
				printf("\e[0m");
#endif

				return null;
			}

			BS_FUNCTION(Clear) {
				DoInit();

#ifdef _MSC_VER
				CONSOLE_SCREEN_BUFFER_INFO csbi;
				GetConsoleScreenBufferInfo(std_con_out, &csbi);
				int cellCount = csbi.dwSize.X *csbi.dwSize.Y;
				DWORD count;
				COORD mpos = {0, 0};

				FillConsoleOutputCharacter(std_con_out, (TCHAR)' ', cellCount, mpos, &count);
				FillConsoleOutputAttribute(std_con_out, csbi.wAttributes, cellCount, mpos, &count);
				SetConsoleCursorPosition(std_con_out, mpos);
#else
				std::system("clear");
#endif

				return null;
			}

			BS_FUNCTION(GetLine) {
				CString buffer;
				while (true) {
					char c = fgetc(stdin);
					if (c == -1)
						return null;

					if (c == '\n')
						return Converter::ToVariable(buffer);

					buffer += c;
				}
			}
		}
	}
}
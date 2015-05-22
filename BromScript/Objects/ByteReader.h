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

#ifndef BROMSCRIPT_BR_INCLUDED
#define BROMSCRIPT_BR_INCLUDED

#include "../SIF.h"
#include "../Objects/Function.h"

namespace BromScript {
	class ByteReader {
	public:
		ByteReader(void);
		~ByteReader(void);

		Scratch::CString* StringTable;
		unsigned char* Code;

		inline unsigned char ReadByte() {
			return this->Code[this->Pos++];
		}

		inline unsigned char* ReadBytes(int len) {
			unsigned char* ret = new unsigned char[len];
			memcpy(ret, this->Code + this->Pos, len);
			this->Pos += len;

			return ret;
		}

		inline bool ReadBool() {
			return this->Code[this->Pos++] == 1;
		}

		inline double ReadDouble() {
			double num = 0;
			memcpy(&num, this->Code + this->Pos, 8);
			this->Pos += 8;

			return num;
		}

		inline short ReadShort() {
			short num = 0;
			memcpy(&num, this->Code + this->Pos, 2);

			this->Pos += 2;
			return num;
		}

		inline int ReadInt() {
			int num = 0;
			memcpy(&num, this->Code + this->Pos, 4);

			this->Pos += 4;
			return num;
		}

		inline Scratch::CString& ReadString() {
			return this->StringTable[this->ReadShort()];
		}

		List<Scratch::CString>* ReadStrings();

		int Pos;
		Function* Func;
	};
}

#endif

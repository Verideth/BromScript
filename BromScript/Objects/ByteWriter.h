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

#ifndef BROMSCRIPT_BW_INCLUDED
#define BROMSCRIPT_BW_INCLUDED

#include "../SIF.h"
#include "../Objects/List.h"
#include "../Scratch/CString.h"

namespace BromScript {
	class ByteWriter {
	public:
		int Count;
		byte* Buffer;
		List<Scratch::CString*>* StringTable;

		ByteWriter(void);
		~ByteWriter(void);

		void WriteBool(bool value);
		void WriteByte(byte value);
		void WriteBytes(byte* value, int size, bool writelen);
		void WriteShort(short value);
		void WriteInt(int value);
		void WriteDouble(double value);
		void WriteString(Scratch::CString value);
		void WriteStrings(List<Scratch::CString> value, bool writelen);
		void WriteStrings(Scratch::CStackArray<Scratch::CString>& value, bool writelen);
		void Clear();
	private:
		int BufferSize;

		void CheckSpace(int needed);
		void AllocateMoreSpace(int addsize);
	};
}

#endif
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

#include "../Objects/ByteWriter.h"

#include <cstdlib>
#include <cstring>

using namespace Scratch;

namespace BromScript {
	ByteWriter::ByteWriter() :Count(0), BufferSize(0), Buffer(null), StringTable(null) {
	}

	ByteWriter::~ByteWriter() {
		this->Clear();
	}

	void ByteWriter::Clear() {
		this->Count = 0;
		this->BufferSize = 0;

		free(this->Buffer);
		this->Buffer = null;
	}

	void ByteWriter::CheckSpace(int needed) {
		if (this->Count + needed >= this->BufferSize) {
			this->AllocateMoreSpace(needed < 512 ? 512 : needed);
		}
	}

	void ByteWriter::WriteBool(bool value) {
		this->CheckSpace(1);
		this->Buffer[this->Count++] = value ? 1 : 0;
	}

	void ByteWriter::WriteByte(byte value) {
		this->CheckSpace(1);
		this->Buffer[this->Count++] = value;
	}

	void ByteWriter::WriteBytes(byte* value, int size, bool writesize) {
		this->CheckSpace(size + (writesize ? 4 : 0));
		if (writesize) this->WriteInt(size);

		memcpy(this->Buffer + this->Count, value, size);
		this->Count += size;
	}

	void ByteWriter::WriteShort(short value) {
		this->CheckSpace(2);
		memcpy(this->Buffer + this->Count, &value, 2);
		this->Count += 2;
	}

	void ByteWriter::WriteInt(int value) {
		this->CheckSpace(4);
		memcpy(this->Buffer + this->Count, &value, 4);
		this->Count += 4;
	}

	void ByteWriter::WriteDouble(double value) {
		this->CheckSpace(8);
		memcpy(this->Buffer + this->Count, &value, 8);
		this->Count += 8;
	}

	void ByteWriter::WriteString(CString value) {
		for (int i = 0; i < this->StringTable->Count; i++) {
			if (*this->StringTable->Get(i) == value) {
				this->WriteShort(i);
				return;
			}
		}

		this->StringTable->Add(new CString(value));
		this->WriteShort(this->StringTable->Count - 1);
	}

	void ByteWriter::WriteStrings(List<CString> value, bool writelen) {
		if (writelen) this->WriteInt(value.Count);

		for (int i = 0; i < value.Count; i++)
			this->WriteString(value[i]);
	}

	void ByteWriter::WriteStrings(CStackArray<CString>& value, bool writelen) {
		if (writelen) this->WriteInt(value.Count());

		for (int i = 0; i < value.Count(); i++)
			this->WriteString(value[i]);
	}

	void ByteWriter::AllocateMoreSpace(int addsize) {
		this->BufferSize += addsize;
		byte* newbuff = (byte*)malloc(this->BufferSize);

		if (this->Buffer != null) {
			memcpy(newbuff, this->Buffer, this->BufferSize - addsize);
			free(this->Buffer);
		}

		this->Buffer = newbuff;
	}
}
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

#include "ByteReader.h"

using namespace Scratch;

namespace BromScript {
	ByteReader::ByteReader(void) :Pos(0) {
	}

	ByteReader::~ByteReader(void) {
	}

	List<CString>* ByteReader::ReadStrings() {
		List<CString>* arr = new List<CString>();

		int len = this->ReadInt();
		for (int i = 0; i < len; i++)
			arr->Add(this->ReadString());

		return arr;
	}
}
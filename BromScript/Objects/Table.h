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

#ifndef BROMSCRIPT_TABLE_INCLUDED
#define BROMSCRIPT_TABLE_INCLUDED

#include "../SIF.h"
#include "../Managers/Instance.h"
#include "../Objects/Variable.h"

namespace BromScript{
	class Table {
	public:
		Table(Instance* bromscript);
		~Table();

		Instance* BromScript;
		int Count;

		void SetIndex(int i, Variable* var);
		void Set(const Scratch::CString& key, Variable* var);
		Variable* Get(const Scratch::CString& key);
		Scratch::CString GetKeyByIndex(int index);
		int GetHasKeyIndex(const Scratch::CString& key);
		bool HasKey(const Scratch::CString& key);
		int GetNextIndex(int spos);
		void Defrag();

		void Remove(const Scratch::CString& key);
		void Clear();

		inline Variable* GetByIndex(int index) { return this->Values[index]; }
		inline Variable* operator[](const Scratch::CString& key) { return this->Get(key); }
		inline Variable* operator[](int index) { return this->Values[index]; }

	private:
		Scratch::CString** Keys;
		Variable** Values;
		int BufferSize;

		void AllocateMoreSpace();
	};
}

#endif
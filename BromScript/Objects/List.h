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

#ifndef BROMSCRIPT_LIST_INCLUDED
#define BROMSCRIPT_LIST_INCLUDED

#ifndef null
#define null 0
#endif

namespace BromScript{
	template<class Type>
	class List {
	public:
		List(const List<Type>& copy);
		List(void);
		~List(void);

		int Count;
		Type** Buffer;

		void Clear();
		void Insert(int index, Type object);
		void Set(int index, Type object);
		void Add(Type object);
		Type RemoveAt(int index);
		Type Get(int index);
		Type operator[](int index);

	private:
		int BufferSize;

		void AllocateMoreSpace();
		void AllocateMoreSpace(int num);
	};
}

#include "List.cpp"

#endif
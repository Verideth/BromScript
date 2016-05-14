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

#include <vector>

namespace BromScript{

	template<class Type>
	class List : public std::vector < Type > {
	public:
		List(void) : Count(0), std::vector<Type>() {}
		List(const List<Type>& copy) : std::vector<Type>(copy), Count(copy.Count) {}
		List(const std::vector<Type>& copy) : std::vector<Type>(copy), Count((int)copy.size()) {}

		int Count;

		inline void Clear() {
			this->clear();
			this->Count = 0;
		}

		inline void Insert(int index, const Type& object) {
			this->insert(this->begin() + index, object);
			this->Count++;
		}

		inline int Add(const Type& object) {
			this->push_back(object);
			return this->Count++;
		}

		inline Type RemoveAt(int index) {
			Type object = (*this)[index];

			this->erase(this->begin() + index);
			this->Count--;

			return object;
		}

		inline void Set(int index, const Type& object) { (*this)[index] = object; }
		inline Type& Get(int index) { return (*this)[index]; }
		inline Type* Buffer() { return &(*this)[0]; }
		inline void Reserve(int num) { this->reserve(num); }
		inline bool Contains(const Type& object) {
			for (int i = 0; i < this->Count; i++) {
				if ((*this)[i] == object) {
					return true;
				}
			}

			return false;
		}
	};
}

#include "List.cpp"

#endif
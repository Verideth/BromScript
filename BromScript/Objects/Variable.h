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

#ifndef BROMSCRIPT_VARIABLE_INCLUDED
#define BROMSCRIPT_VARIABLE_INCLUDED

#include "../SIF.h"
#include "../Misc.h"
#include "../Objects/PoolReference.h"

namespace BromScript {
	namespace VariableType{
		enum Enum {
			Null, String, Bool, Number, Table, Function, Class, Userdata = 9
		};
	}

	class Variable {
		int Using;

	public:
		Variable();
		~Variable();

		VariableType::Enum Type;
		void* Value;

		PoolReference PoolRef;

		bool IsCpp;
		bool DeleteOnDestruct;
		bool RegisteredInGC;
		bool ReadOnly;

		void EmptyValue();

		double GetNumber();
		Scratch::CString GetString();
		Scratch::CString ToString();
		Scratch::CString ToString(Instance* bsi);
		bool GetBool();
		void* GetUserdata();
		BromScript::Function* GetFunction();
		BromScript::Table* GetTable();

		inline void IncreeseRefCount() { this->Using++; }
		inline void DecreeseRefCount() { this->Using--; }

#ifdef BS_DEBUG_REFCOUNT
		List<Scratch::CString> RefStackList;

		void IncreeseRefCount(int line, Scratch::CString file) {
			char* padding = new char[this->Using + 1];
			memset(padding, ' - ', this->Using);
			padding[this->Using] = 0;

			RefStackList.Add(Scratch::CString::Format("+ %d %s%s:%d", this->Using + 1, padding, file.str_szBuffer, line));
			delete[] padding;

			this->Using++;
		}

		void DecreeseRefCount(int line, Scratch::CString file) {
			if (this->Using == 0) {
				for (int i = 0; i < RefStackList.Count; i++) {
					printf("%s\n", RefStackList[i].str_szBuffer);
				}

				throw "Fatal Variable reference count error, reference count below is -1";
			}

			char* padding = new char[this->Using];
			memset(padding, ' ', this->Using - 1);
			padding[this->Using - 1] = 0;

			RefStackList.Add(Scratch::CString::Format("- %d %s%s:%d", this->Using - 1, padding, file.str_szBuffer, line));
			delete[] padding;

			this->Using--;
		}
#endif

		inline int GetRefCount() { return this->Using; }
	};
}

#endif
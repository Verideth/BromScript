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

#ifndef BROMSCRIPT_USERDATA_INCLUDED
#define BROMSCRIPT_USERDATA_INCLUDED

#include "../SIF.h"
#include "../Misc.h"
#include "../Objects/MemberData.h"

namespace BromScript{
	class Userdata {
	public:
		Userdata();
		~Userdata();

		int Offset;
		int TypeSize;
		int TypeID;
		bool CallDTor;

		Scratch::CString Name;
		Instance* BromScript;

		BSFunction OperatorsOverrides[(int)Operators::Arithmetic_END - (int)Operators::Arithmetic_START - 1]; // same amount as Misc::ArithmaticFuncs
		List<Userdata*> Members;
		Scratch::CDictionary<Scratch::CString, BSFunction> Functions;

		Userdata* InheritFrom;

		BSFunctionCtor Ctor;
		BSFunctionDtor Dtor;

		BSSetFunction Setter;
		BSGetFunction Getter;

		void RegisterMember(Scratch::CString name, int offset, int type);
		void RegisterMember(Scratch::CString name, int offset, int type, BSSetFunction setter, BSGetFunction getter);
		void RegisterFunction(Scratch::CString name, BSFunction function);
		void RegisterOperator(Operators opcode, BSFunction function);

		Userdata* Copy();
	};
}
#endif
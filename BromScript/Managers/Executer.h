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

#ifndef BROMSCRIPT_EXECUTER_INCLUDED
#define BROMSCRIPT_EXECUTER_INCLUDED

#include "../SIF.h"
#include "../Misc.h"

namespace BromScript {
	class Executer {
	public:
		static void Duplicate(ExecuteData* data);
		static void AddIndex(ExecuteData* data);
		static void GetCount(ExecuteData* data);
		static void GetIndex(ExecuteData* data);
		static void SetIndex(ExecuteData* data);
		static void Set(ExecuteData* data);
		static void SetL(ExecuteData* data);
		static void New(ExecuteData* data);
		static void Call(ExecuteData* data);
		static void CallThis(ExecuteData* data);
		static void PostIncrement(ExecuteData* data);
		static void PreIncrement(ExecuteData* data);
		static void Pop(ExecuteData* data);
		static void Delete(ExecuteData* data);
		static void StackNull(ExecuteData* data);
		static void StackBool(ExecuteData* data);
		static void StackNumber(ExecuteData* data);
		static void StackTable(ExecuteData* data);
		static void StackString(ExecuteData* data);
		static void StackFunction(ExecuteData* data);
		static void JumpNT(ExecuteData* data);
		static void Jump(ExecuteData* data);
		static void Get(ExecuteData* data);
		static void GetL(ExecuteData* data);
		static void StringTable(ExecuteData* data);
		static void GlobalLocals(ExecuteData* data);
		static void CurrentLine(ExecuteData* data);

		static Variable* Arithmatic(ExecuteData* data, Variable* vara, Variable* varb, Operators func);
	};
}

#endif
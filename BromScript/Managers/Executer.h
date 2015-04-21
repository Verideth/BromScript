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

#include "../Objects/Variable.h"
#include "../Objects/ExecuteData.h"
#include "../Misc.h"

namespace BromScript {
	class Executer {
	public:
		static void SetVar(ExecuteData* data);
		static void LSetVar(ExecuteData* data);
		static void SetTableIndex(ExecuteData* data);

		static void CreateFunction(ExecuteData* data);
		static Variable* CreateAnonFunction(ExecuteData* data);

		static void If(ExecuteData* data);
		static void Else(ExecuteData* data);

		static void While(ExecuteData* data);
		static void For(ExecuteData* data);
		static void ForEach(ExecuteData* data);
		static void Loop(ExecuteData* data);
		static void Goto(ExecuteData* data);
		static void Rewind(ExecuteData* data);
		static void Delete(ExecuteData* data);

		static void CreateClass(ExecuteData* data);

		static void StringTable(ExecuteData* data);
		static void GlobalLocals(ExecuteData* data);

		static Variable* Arithmatic(ExecuteData* data, Variable* vara, Variable* varb, Misc::ArithmaticFuncs func);

		static Variable* GetVar(ExecuteData* data);
		static Variable* GetTableIndex(ExecuteData* data);

		static Variable* New(ExecuteData* data);
		static Variable* ExecuteFunction(ExecuteData* data);
		static Variable* Merge(ExecuteData* data);
		static Variable* Return(ExecuteData* data);

		static Variable* GetCount(ExecuteData* data);
		static Variable* GetBool(ExecuteData* data);
		static Variable* GetEnum(ExecuteData* data);
		static Variable* GetTable(ExecuteData* data);
		static Variable* GetString(ExecuteData* data);
		static Variable* GetNumber(ExecuteData* data);

		static Variable* PreIncrement(ExecuteData* data);
		static Variable* PostIncrement(ExecuteData* data);
		static Variable* PreDecrement(ExecuteData* data);
		static Variable* PostDecrement(ExecuteData* data);
	};
}

#endif
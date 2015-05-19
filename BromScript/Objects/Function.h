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

#ifndef BROMSCRIPT_FUNCTION_INCLUDED
#define BROMSCRIPT_FUNCTION_INCLUDED

#include "../SIF.h"

#include "../Misc.h"
#include "../Managers/Instance.h"
#include "../Managers/Executer.h"
#include "../Objects/Variable.h"
#include "../Objects/ExecuteData.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Table.h"

namespace BromScript {
	class Function {
	public:
		Function(Instance* bromscript);
		~Function();

		Variable* Run();
		Variable* Run(Variable* thisobject);
		Variable* Run(ArgumentData* args);
		Variable* Run(ArgumentData* args, Variable* thisobject);
		Variable* Run(List<Variable*>* args);
		Variable* Run(List<Variable*>* args, Variable* thisobject);
		Variable* InternalRun(ExecuteData* data);

		void SetReferences(Function* func, int entrypoint);

		void SetVar(Variable* var, int localindex);
		void SetVar(Scratch::CString key, Variable* var, bool global);
		Variable* GetVar(Scratch::CString& var);

		Scratch::CString* FixedLocalKeys;
		Variable** FixedLocalVars;
		int* FixedLocalTypes;
		bool* FixedLocalIsRef;
		int FixedLocalsCount;

		Instance* BromScript;
		Function* Parent;
		List<Scratch::CString> Parameters;

		int StringTableCount;
		Scratch::CString* StringTable;
		Variable** StringTableVars;

		Scratch::CString Name;
		Scratch::CString Filename;
		byte* Code;
		int CodeLength;
		int CodeOffset;

		int CurrentSourceFileLine;
		bool ForceReturn;
		bool IsCpp;
		BSFunction CppFunc;
		Variable* CurrentThisObject;

		Function* CodeReferenceFunc;
		List<Function*> CodeReferenceChilds;

	private:
	};
}

#endif
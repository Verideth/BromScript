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

#ifndef BROMSCRIPT_ARGUMENTDATA_INCLUDED
#define BROMSCRIPT_ARGUMENTDATA_INCLUDED

#include "../Objects/Variable.h"
#include "../Managers/Instance.h"
#include "../Objects/Function.h"
#include "../Objects/Table.h"
#include "List.h"
#include "../Managers/Converter.h"

namespace BromScript {
	class ArgumentData {
	public:
		ArgumentData(void);
		~ArgumentData(void);

		int Count;
		Variable* ThisObject;

		Instance* BromScript;
		Function* Caller;

		void SetVariableData(List<Variable*>* vars);
		void InsertVariable(int index, Variable* var);
		void AddVariable(Variable* var);

		void Error(Scratch::CString msg);
		void Error(Scratch::CString msg, int linenumber, const char* file);

		VariableType::Enum GetType(int index);
		bool CheckType(int index, VariableType::Enum type);
		bool CheckType(int index, VariableType::Enum type, bool throwerror);
		bool CheckType(int index, int type);
		bool CheckType(int index, int type, bool throwerror);

		bool CheckThisObject(VariableType::Enum type);
		bool CheckThisObject(VariableType::Enum type, bool throwerror);
		bool CheckThisObject(int type);
		bool CheckThisObject(int type, bool throwerror);

		bool ErrorOccured();
		void Clear();
		void SetThisObject(Variable* var);

		const char* GetString(int index);
		double GetNumber(int index);
		bool GetBool(int index);
		Table* GetTable(int index);
		Function* GetFunction(int index);
		Variable* GetVariable(int index);
		void* GetUserdata(int index, int datatype);
		void* GetThisObjectData();

	private:
		List<Variable*> Vars;
	};
}

#endif
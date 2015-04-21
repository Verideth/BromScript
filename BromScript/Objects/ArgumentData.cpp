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

#include "../Objects/ArgumentData.h"

using namespace Scratch;

namespace BromScript {
	ArgumentData::ArgumentData(void) :Caller(null), BromScript(null), Count(0) {
	}

	ArgumentData::~ArgumentData(void) {
	}

	VariableType::Enum ArgumentData::GetType(int index) {
		if (index >= this->Count) return VariableType::Null;
		return this->Vars[index]->Type;
	}

	bool ArgumentData::ErrorOccured(void) {
		return this->BromScript->GetCurrentFunction()->ForceReturn;
	}

	bool ArgumentData::CheckType(int index, int type) { return this->CheckType(index, (VariableType::Enum)type); }
	bool ArgumentData::CheckType(int index, VariableType::Enum type) {
		if (index >= this->Count) return false;
		return this->Vars[index]->Type == type;
	}

	bool ArgumentData::CheckType(int index, int type, bool throwerror) { return this->CheckType(index, (VariableType::Enum)type, throwerror); }
	bool ArgumentData::CheckType(int index, VariableType::Enum type, bool throwerror) {
		if (index >= this->Count) {
			if (throwerror)
				BS_THROW_ERROR(this->BromScript, CString::Format("Missing argument!, Expected %s at argument index %d", Converter::TypeToString(this->BromScript, type).str_szBuffer, index));

			return false;
		}

		if (this->Vars[index]->Type != type) {
			if (throwerror)
				BS_THROW_ERROR(this->BromScript, CString::Format("Invalid type given as argument!, Expected %s got %s", Converter::TypeToString(this->BromScript, type).str_szBuffer, Converter::TypeToString(this->BromScript, this->Vars[index]->Type).str_szBuffer));

			return false;
		}

		return true;
	}

	bool ArgumentData::CheckThisObject(int type) { return this->CheckThisObject((VariableType::Enum)type); }
	bool ArgumentData::CheckThisObject(VariableType::Enum type) {
		return this->ThisObject != null && this->ThisObject->Type == type;
	}

	bool ArgumentData::CheckThisObject(int type, bool throwerror) { return this->CheckThisObject((VariableType::Enum)type, throwerror); }
	bool ArgumentData::CheckThisObject(VariableType::Enum type, bool throwerror) {
		if (this->ThisObject == null) {
			if (throwerror)
				BS_THROW_ERROR(this->BromScript, CString::Format("Invalid Userdata type, Expected %s got NULL", Converter::TypeToString(this->BromScript, type).str_szBuffer));

			return false;
		}

		if (this->ThisObject->Type != type) {
			if (throwerror) {
				BS_THROW_ERROR(this->BromScript, CString::Format("Invalid type given as argument!, Expected %s got %s", Converter::TypeToString(type).str_szBuffer, Converter::TypeToString(this->ThisObject).str_szBuffer));
			}

			return false;
		}

		return true;
	}

	const char* ArgumentData::GetString(int index) {
		if (index >= this->Count) return null;
		if (!this->CheckType(index, VariableType::String)) return null;

		return *(CString*)this->Vars[index]->Value;
	}

	double ArgumentData::GetNumber(int index) {
		if (index >= this->Count) return 0;
		if (!this->CheckType(index, VariableType::Number)) return 0;

		return *(double*)this->Vars[index]->Value;
	}

	bool ArgumentData::GetBool(int index) {
		if (index >= this->Count) return false;
		if (!this->CheckType(index, VariableType::Bool)) return false;

		return this->Vars[index]->Value != null;
	}

	Table* ArgumentData::GetTable(int index) {
		if (index >= this->Count) return null;
		if (!this->CheckType(index, VariableType::Table)) return null;

		return (Table*)this->Vars[index]->Value;
	}

	Function* ArgumentData::GetFunction(int index) {
		if (index >= this->Count) return null;
		if (!this->CheckType(index, VariableType::Function)) return null;

		return (Function*)this->Vars[index]->Value;
	}

	void* ArgumentData::GetUserdata(int index, int datatype) {
		if (index >= this->Count) return null;
		if (!this->CheckType(index, (VariableType::Enum)datatype)) return null;

		return ((UserdataInstance*)(this->Vars[index]->Value))->Ptr;
	}

	void* ArgumentData::GetThisObjectData() {
		return ((UserdataInstance*)(this->ThisObject->Value))->Ptr;
	}

	Variable* ArgumentData::GetVariable(int index) {
		if (index >= this->Count) return null;

		return this->Vars[index];
	}

	void ArgumentData::SetVariableData(List<Variable*>* vars) {
		this->Count = vars->Count;
		this->Vars.Clear();
		for (int i = 0; i < vars->Count; i++)
			this->Vars.Add(vars->Get(i));
	}

	void ArgumentData::InsertVariable(int pos, Variable* var) {
		this->Count++;
		this->Vars.Insert(pos, var);
	}

	void ArgumentData::AddVariable(Variable* var) {
		this->Count++;
		this->Vars.Add(var);
	}

	void ArgumentData::Error(Scratch::CString msg, int linenumber, const char* file) {
		this->BromScript->Error(msg, linenumber, file);
	}

	void ArgumentData::Error(CString msg) {
		this->BromScript->Error(msg);
	}
}

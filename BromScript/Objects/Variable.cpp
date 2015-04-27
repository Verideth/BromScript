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

#ifndef SCRATCH_NO_GLOBFUNC
#define SCRATCH_NO_GLOBFUNC
#endif

#include "../Scratch/Scratch.h"
#include "../Managers/Converter.h"
#include "../Objects/Variable.h"

using namespace Scratch;

namespace BromScript {
	Variable::Variable() :IsCpp(false), ReadOnly(false), Value(null), Type(VariableType::Null), DeleteOnDestruct(true), Using(0), RegisteredInGC(false) {
	}

	Variable::~Variable() {
		this->EmptyValue();
	}

	void Variable::EmptyValue() {
		if (!this->DeleteOnDestruct) {
			this->Type = VariableType::Null;
			this->Value = null;
			return;
		}

		switch (this->Type) {
			case VariableType::Null: break;
			case VariableType::Bool: break;
			case VariableType::Class: delete (BromScript::Function*)(this->Value); break;
			case VariableType::Function: delete (BromScript::Function*)(this->Value); break;
			case VariableType::String: delete (CString*)(this->Value); break;
			case VariableType::Number: delete (double*)(this->Value); break;
			case VariableType::Table: delete (BromScript::Table*)(this->Value); break;
			case VariableType::Userdata: break;
			default: // userdata
				delete (BromScript::UserdataInstance*)(this->Value);
				break;
		}

		this->Type = VariableType::Null;
		this->Value = null;
	}

	CString Variable::ToString() {
		return Converter::VariableToString(this);
	}

	CString Variable::ToString(Instance* bsi) {
		return Converter::VariableToString(bsi, this);
	}

	CString Variable::GetString() {
		if (this->Type != VariableType::String) return "";
		return *(CString*)this->Value;
	}

	double Variable::GetNumber() {
		if (this->Type != VariableType::Number) return 0;
		return *(double*)this->Value;
	}

	bool Variable::GetBool() {
		if (this->Type != VariableType::Bool) return false;
		return this->Value != null;
	}

	void* Variable::GetUserdata() {
		return ((BromScript::UserdataInstance*)(this->Value))->Ptr;
	}

	Table* Variable::GetTable() {
		if (this->Type != VariableType::Table) return null;
		return (BromScript::Table*)this->Value;
	}

	Function* Variable::GetFunction() {
		if (this->Type != VariableType::Function) return null;
		return (BromScript::Function*)this->Value;
	}
}
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

#include "../Managers/Converter.h"

using namespace Scratch;

namespace BromScript{
	bool Converter::SetMember(Instance* bromscript, Variable* member, Variable* value, const CString &key) {
		UserdataInstance* udi = (UserdataInstance*)member->Value;

		if (udi->TypeData->Setter != null) {
			udi->TypeData->Setter(bromscript, udi->Ptr, value);
			return true;
		}

		if (udi->TypeData->TypeID == MemberType::Bool && value->Type == VariableType::Bool) {
			*(bool*)udi->Ptr = value->Value != null;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Byte && value->Type == VariableType::Number) {
			*(byte*)udi->Ptr = (byte)*(double*)value->Value;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Int && value->Type == VariableType::Number) {
			*(int*)udi->Ptr = (int)*(double*)value->Value;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Short && value->Type == VariableType::Number) {
			*(short*)udi->Ptr = (short)*(double*)value->Value;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Float && value->Type == VariableType::Number) {
			*(float*)udi->Ptr = (float)*(double*)value->Value;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Double && value->Type == VariableType::Number) {
			*(double*)udi->Ptr = *(double*)value->Value;
			return true;
		} else if (udi->TypeData->TypeID == MemberType::Long && value->Type == VariableType::Number) {
			*(long long*)udi->Ptr = (long long)*(double*)value->Value;
			return true;
		} else if (member->Type == value->Type && member->Type > MemberType::Userdata) {
			memcpy(udi->Ptr, ((UserdataInstance*)value->Value)->Ptr, udi->TypeData->TypeSize);
			return true;
		}

		return false;
	}

	Variable* Converter::MemberToVariable(Instance* bromscript, Variable* member) {
		UserdataInstance* udi = (UserdataInstance*)member->Value;

		if (udi->TypeData->Getter != null) {
			Variable* ret = udi->TypeData->Getter(bromscript, udi->Ptr);
			if (ret == null) ret = bromscript->GetDefaultVarNull();
			else bromscript->GC.RegisterVariable(ret);

			return ret;
		}

		if (member->Type > MemberType::Bool)
			return member;

		Variable* var = bromscript->GC.GetPooledVariable();
		var->IsCpp = member->IsCpp;
		var->Value = member->Value;
		var->Type = member->Type;
		var->DeleteOnDestruct = false;

		switch (udi->TypeData->TypeID) {
			case MemberType::Bool:
				var->Value = Converter::BoolToPointer(*(bool*)udi->Ptr);
				var->Type = VariableType::Bool;
				break;
			case MemberType::Double:
				var->Value = Converter::NumberToPointer(bromscript, *(double*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Byte:
				var->Value = Converter::NumberToPointer(bromscript, (double)*(byte*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Int:
				var->Value = Converter::NumberToPointer(bromscript, (double)*(int*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Short:
				var->Value = Converter::NumberToPointer(bromscript, (double)*(short*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Float:
				var->Value = Converter::NumberToPointer(bromscript, (double)*(float*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Long:
				var->Value = Converter::NumberToPointer(bromscript, (double)*(long long*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
		}

		return var;
	}

	void* Converter::NumberToPointer(Instance* bromscript, double val) {
		double* ret = bromscript->GC.NumberPool.GetNext();
		if (ret == null) {
			*ret = val;
			return ret;
		}
		
		return new double(val);
	}

	Variable* Converter::ToVariable(Instance* bromscript, const CString &key, BSFunction value) {
		Function* varfunc = new Function(bromscript);
		varfunc->CppFunc = value;
		varfunc->Filename = "C++";
		varfunc->IsCpp = true;
		varfunc->Name = key;

		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Function;
		ret->Value = varfunc;

		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, const char* key, BSFunction value) {
		Function* varfunc = new Function(bromscript);
		varfunc->CppFunc = value;
		varfunc->Filename = "C++";
		varfunc->IsCpp = true;
		varfunc->Name = key;

		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Function;
		ret->Value = varfunc;

		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, bool value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Bool;
		ret->Value = Converter::BoolToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, double value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(bromscript, value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, float value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(bromscript, value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, int value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(bromscript, value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, long long value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(bromscript, (double)value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, const CString &value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, const char* value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(Instance* bromscript, Table* value) {
		Variable* ret = bromscript->GC.GetPooledVariable();
		ret->Type = VariableType::Table;
		ret->Value = value;
		return ret;
	}

	CString Converter::VariableToString(Variable* var) {
		switch (var->Type) {
			case VariableType::String: return var->GetString(); break;
			case VariableType::Number: return CString::Format("%d", (int)var->GetNumber()); break;
			case VariableType::Function: return "Function"; break;
			case VariableType::Bool: return var->GetBool() ? "True" : "False"; break;
			case VariableType::Null: return "NULL"; break;
			case VariableType::Table:
				if (var->GetTable()->Get("__TYPE")->Type != VariableType::Null)
					return CString::Format("Class{%s}", var->GetTable()->Get("__TYPE")->GetString().str_szBuffer);

				return CString::Format("{table count=%d}", var->GetTable()->Count);
				break;

			default:
				return CString::Format("Class{%s}", ((UserdataInstance*)var->Value)->TypeData->Name.str_szBuffer);
		}
	}

	CString Converter::VariableToString(Instance* bromscript, Variable* var) {
		switch (var->Type) {
			case VariableType::String: return var->GetString(); break;
			case VariableType::Number: return CString::Format("%d", (int)var->GetNumber()); break;
			case VariableType::Function: return "function"; break;
			case VariableType::Bool: return var->GetBool() ? "true" : "false"; break;
			case VariableType::Null: return "null"; break;
			case VariableType::Table:
				if (var->GetTable()->Get("__TYPE")->Type != VariableType::Null)
					return CString::Format("class{%s}", var->GetTable()->Get("__TYPE")->GetString().str_szBuffer);

				return CString::Format("{table count=%d}", var->GetTable()->Count);
				break;

			default:
				Userdata* ud = ((UserdataInstance*)var->Value)->TypeData;
				if (ud->OperatorsOverrides[(int)Operators::ArithmeticToString - (int)Operators::Arithmetic_START - 1] != null) {
					ArgumentData args;
					args.ThisObject = var;

					Variable* ret = ud->OperatorsOverrides[(int)Operators::ArithmeticToString - (int)Operators::Arithmetic_START - 1](bromscript, &args);
					if (ret == null) return "NULL";
					else bromscript->GC.RegisterVariable(ret);

					return *(CString*)ret->Value;
				}

				return CString::Format("Class{%s}", ((UserdataInstance*)var->Value)->TypeData->Name.str_szBuffer);
		}
	}
	
	CString Converter::TypeToString(VariableType::Enum type) {
		switch (type) {
			case VariableType::Bool: return "Bool";
			case VariableType::Class: return "Class";
			case VariableType::Function: return "Function";
			case VariableType::Null: return "Null";
			case VariableType::Number: return "Number";
			case VariableType::String: return "String";
			case VariableType::Table: return "Table";
			default:
				return "Userdata";
		}
	}

	CString Converter::TypeToString(Instance* bromscript, VariableType::Enum type) {
		switch (type) {
			case VariableType::Bool: return "Bool";
			case VariableType::Class: return "Class";
			case VariableType::Function: return "Function";
			case VariableType::Null: return "Null";
			case VariableType::Number: return "Number";
			case VariableType::String: return "String";
			case VariableType::Table: return "Table";
			default:
				for (int i = 0; i < bromscript->RegisteredUserdataTypes.Count; i++) {
					if (bromscript->RegisteredUserdataTypes[i]->TypeID == type) {
						return bromscript->RegisteredUserdataTypes[i]->Name;
					}
				}

				return "Userdata";
		}
	}

	CString Converter::TypeToString(Variable* var) {
		if (var == null) return "Null";

		switch (var->Type) {
			case VariableType::Bool: return "Bool";
			case VariableType::Class: return "Class";
			case VariableType::Function: return "Function";
			case VariableType::Null: return "Null";
			case VariableType::Number: return "Number";
			case VariableType::String: return "String";
			case VariableType::Table:
				if (var->GetTable()->Get("__TYPE")->Type != VariableType::Null) {
					return var->GetTable()->Get("__TYPE")->GetString();
				}

				return "Table";
			default:
				return ((UserdataInstance*)var->Value)->TypeData->Name;
		}
	}

	CString Converter::ArithmaticToString(Operators type) {
		switch (type) {
			case Operators::ArithmeticAdd: return "Add";
			case Operators::ArithmeticAnd: return "And";
			case Operators::ArithmeticDivide: return "Divide";
			case Operators::ArithmeticEqual: return "Equal";
			case Operators::ArithmeticGreaterOrEqual: return "GreaterOrEqual";
			case Operators::ArithmeticGreaterThan: return "GreaterThan";
			case Operators::ArithmeticLessOrEqual: return "LessOrEqual";
			case Operators::ArithmeticLessThan: return "LessThan";
			case Operators::ArithmeticModulo: return "Modulo";
			case Operators::ArithmeticMultiply: return "Multiply";
			case Operators::ArithmeticNotEqual: return "NotEqual";
			case Operators::ArithmeticOr: return "Or";
			case Operators::ArithmeticSubstract: return "Substract";
			case Operators::ArithmeticBitwiseAnd: return "BitwiseAnd";
			case Operators::ArithmeticBitwiseOr: return "BitwiseOr";
			case Operators::ArithmeticBitwiseLeft: return "BitwiseLeft";
			case Operators::ArithmeticBitwiseRight: return "BitwiseRight";
			default: return "Unknown";
		}
	}
}
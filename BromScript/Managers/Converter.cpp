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
	bool Converter::SetMember(Instance* bromscript, Variable* member, Variable* value, CString key) {
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

		Variable* var = bromscript->GC.RegisterVariable();
		var->IsCpp = member->IsCpp;
		var->Value = member->Value;
		var->Type = member->Type;

		switch (udi->TypeData->TypeID) {
			case MemberType::Bool:
				var->Value = Converter::BoolToPointer(*(bool*)udi->Ptr);
				var->Type = VariableType::Bool;
				break;
			case MemberType::Double:
				var->Value = Converter::NumberToPointer(*(double*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Byte:
				var->Value = Converter::NumberToPointer((double)*(byte*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Int:
				var->Value = Converter::NumberToPointer((double)*(int*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Short:
				var->Value = Converter::NumberToPointer((double)*(short*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Float:
				var->Value = Converter::NumberToPointer((double)*(float*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
			case MemberType::Long:
				var->Value = Converter::NumberToPointer((double)*(long long*)udi->Ptr);
				var->Type = VariableType::Number;
				break;
		}

		return var;
	}

	Variable* Converter::ToVariable(Instance* bromscript, const char* key, BSFunction value) {
		return Converter::ToVariable(bromscript, CString(key), value);
	}

	Variable* Converter::ToVariable(Instance* bromscript, CString key, BSFunction value) {
		Function* varfunc = new Function(bromscript);
		varfunc->CppFunc = value;
		varfunc->Filename = "C++";
		varfunc->IsCpp = true;
		varfunc->Name = key;

		Variable* ret = new Variable();
		ret->Type = VariableType::Function;
		ret->Value = varfunc;
		ret->DeleteOnDestruct = false;

		return ret;
	}

	Variable* Converter::ToVariable(bool value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Bool;
		ret->Value = Converter::BoolToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(double value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(float value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(int value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(long long value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer((double)value);
		return ret;
	}

	Variable* Converter::ToVariable(const char* value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(CString value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(value);
		return ret;
	}

	Variable* Converter::ToVariable(Table* value) {
		Variable* ret = new Variable();
		ret->Type = VariableType::Table;
		ret->Value = value;
		return ret;
	}

	CString Converter::VariableToString(Variable* var) {
		switch (var->Type) {
			case VariableType::String: return var->GetString(); break;
			case VariableType::Number: return CString(var->GetNumber()); break;
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
			case VariableType::Number: return CString(var->GetNumber()); break;
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
				if (ud->Operators[Misc::ArithmaticFuncs::ToString - 100] != null) {
					ArgumentData args;
					args.ThisObject = var;

					Variable* ret = ud->Operators[Misc::ArithmaticFuncs::ToString - 100](bromscript, &args);
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

	CString Converter::ArithmaticToString(Misc::ArithmaticFuncs type) {
		switch (type) {
			case Misc::ArithmaticFuncs::Add: return "Add";
			case Misc::ArithmaticFuncs::And: return "And";
			case Misc::ArithmaticFuncs::Divide: return "Divide";
			case Misc::ArithmaticFuncs::Equal: return "Equal";
			case Misc::ArithmaticFuncs::GreaterOrEqual: return "GreaterOrEqual";
			case Misc::ArithmaticFuncs::GreaterThan: return "GreaterThan";
			case Misc::ArithmaticFuncs::LessOrEqual: return "LessOrEqual";
			case Misc::ArithmaticFuncs::LessThan: return "LessThan";
			case Misc::ArithmaticFuncs::Modulo: return "Modulo";
			case Misc::ArithmaticFuncs::Multiply: return "Multiply";
			case Misc::ArithmaticFuncs::NotEqual: return "NotEqual";
			case Misc::ArithmaticFuncs::Or: return "Or";
			case Misc::ArithmaticFuncs::Substract: return "Substract";
			case Misc::ArithmaticFuncs::BitwiseAnd: return "BitwiseAnd";
			case Misc::ArithmaticFuncs::BitwiseOr: return "BitwiseOr";
			case Misc::ArithmaticFuncs::BitwiseLeft: return "BitwiseLeft";
			case Misc::ArithmaticFuncs::BitwiseRight: return "BitwiseRight";
			default: return "Unknown";
		}
	}
}
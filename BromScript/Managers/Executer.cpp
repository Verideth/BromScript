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


#include "Executer.h"
#include "Converter.h"
#include "../Objects/Function.h"
#include "../Objects/Variable.h"
#include "../Objects/Table.h"
#include "../Objects/ExecuteData.h"
#include "../Objects/UserdataInstance.h"

using namespace Scratch;

namespace BromScript {
	bool CheckIfVarTrue(Variable* var) {
		switch (var->Type) {
			case VariableType::Null: return false;
			case VariableType::Number: return *(double*)var->Value >= 0; break;
			case VariableType::String: return strlen(*(CString*)var->Value) > 0; break;
			case VariableType::Table: return ((Table*)var->Value)->Count > 0; break;
			case VariableType::Bool: return var->Value != null; break;
			default: return var->Value != null; break;
		}
	}

	inline void Arithmatic_DivideNumber(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(*(double*)a->Value / *(double*)b->Value);
	}

	inline void Arithmatic_MultiplyNumber(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(*(double*)a->Value * *(double*)b->Value);
	}

	void Arithmatic_MultiplyString(Variable* ret, Variable* num, Variable* string) {
		CString strtoadd(*(CString*)string->Value);
		CString* str = new CString();
		int loops = (int)*(double*)num->Value;

		if (loops <= 0) {
			ret->Type = VariableType::String;
			ret->Value = Converter::StringToPointer("");
			return;
		}

		int size = strtoadd.Size();
		int totalsize = loops * size + (int)((*(double*)num->Value - loops) * size);
		char* newp = new char[totalsize + 1];

		str->str_szBuffer = newp;

		for (int i = 0; i < loops; i++)
			memcpy(newp + i * size, strtoadd.str_szBuffer, size);

		if (*(double*)num->Value - loops > 0)
			memcpy(newp + loops * size, strtoadd.str_szBuffer, (int)((*(double*)num->Value - loops) * size));

		newp[totalsize] = null;

		ret->Type = VariableType::String;
		ret->Value = str;
	}

	inline void Arithmatic_SubstractNumber(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(*(double*)a->Value - *(double*)b->Value);
	}

	void Arithmatic_DivideString(Instance* bs, Variable* ret, Variable* a, Variable* b) {
		CString str = *(CString*)a->Value;
		Table* tbl = new Table(bs);

		int strlen = str.Size();
		int partlen = (int)(strlen / *(double*)b->Value);

		ret->Type = VariableType::Table;
		ret->Value = tbl;

		if (partlen <= 0)
			return;

		for (int i = 0; i < strlen; i += partlen)
			tbl->Set(CString((double)i), Converter::ToVariable(bs, str.Substring(i, strlen - i < partlen ? strlen - i : partlen)));
	}

	inline void Arithmatic_AddNumber(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::Number;
		ret->Value = Converter::NumberToPointer(*(double*)a->Value + *(double*)b->Value);
	}

	inline void Arithmatic_AddString(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(*(CString*)a->Value + *(CString*)b->Value);
	}

	inline void Arithmatic_AddNumberToString(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(*(CString*)b->Value + CString(*(double*)a->Value));
	}

	inline void Arithmatic_AddNumberToString2(Variable* ret, Variable* a, Variable* b) {
		ret->Type = VariableType::String;
		ret->Value = Converter::StringToPointer(CString::Format("%f%s", *(double*)a->Value, ((CString*)b->Value)->str_szBuffer));
	}

	Variable* Executer::Arithmatic(ExecuteData* data, Variable* a, Variable* b, Misc::ArithmaticFuncs af) {
		Variable* ret;

		if (a->Type > VariableType::Userdata || b->Type > VariableType::Userdata) {
			UserdataInstance* udi;

			if (a->Type > VariableType::Userdata) udi = (UserdataInstance*)a->Value;
			else udi = (UserdataInstance*)b->Value;

			/*
			if (udi->TypeData->Operators[af - 100] != null) {
				ArgumentData args;
				args.AddVariable(a);
				args.AddVariable(b);

				ret = udi->TypeData->Operators[af - 100](data->BromScript, &args);
				if (ret == null) ret = data->BromScript->GetDefaultVarNull();
				else data->BromScript->GC.RegisterVariable(ret);

				return ret;
			} else {
				BS_THROW_ERROR(data->BromScript, CString::Format("Invalid arithmatic function!, trying to do %s %s %s", Converter::TypeToString(a).str_szBuffer, Converter::ArithmaticToString(af).str_szBuffer, Converter::TypeToString(b).str_szBuffer));
				return data->BromScript->GetDefaultVarNull();
			}
			*/
		}

		Variable* bsobj = null;
		if (a->Type == VariableType::Table && a->GetTable()->Get("__TYPE")->Type != VariableType::Null) bsobj = a;
		else if (b->Type == VariableType::Table && b->GetTable()->Get("__TYPE")->Type != VariableType::Null) bsobj = b;

		if (bsobj != null) {
			Variable* afunc = bsobj->GetTable()->Get(CString::Format("__%s", Converter::ArithmaticToString(af).str_szBuffer));

			if (afunc->Type == VariableType::Null) {
				BS_THROW_ERROR(data->BromScript, CString::Format("Invalid arithmatic function!, trying to do %s %s %s", Converter::TypeToString(a).str_szBuffer, Converter::ArithmaticToString(af).str_szBuffer, Converter::TypeToString(b).str_szBuffer));
				return data->BromScript->GetDefaultVarNull();
			}

			if (afunc->Type != VariableType::Function) {
				BS_THROW_ERROR(data->BromScript, CString::Format("Invalid arithmatic function!, trying to call a non-function while trying to do %s %s %s", Converter::TypeToString(a).str_szBuffer, Converter::ArithmaticToString(af).str_szBuffer, Converter::TypeToString(b).str_szBuffer));
				return data->BromScript->GetDefaultVarNull();
			}

			ArgumentData args;
			args.AddVariable(a);
			args.AddVariable(b);

			Variable* ret = afunc->GetFunction()->Run(&args, bsobj);

			return ret;
		}

		ret = data->BromScript->GC.GetPooledVariable();

		switch (af) {
			case Misc::ArithmaticFuncs::Add:
				if (a->Type == VariableType::Number) {
					if (b->Type == VariableType::Number) Arithmatic_AddNumber(ret, a, b);
					if (b->Type == VariableType::String) Arithmatic_AddNumberToString2(ret, a, b);
				} else if (a->Type == VariableType::String) {
					if (b->Type == VariableType::String) Arithmatic_AddString(ret, a, b);
					if (b->Type == VariableType::Number) Arithmatic_AddNumberToString(ret, b, a);
				}
				break;

			case Misc::ArithmaticFuncs::Substract:
				if (a->Type == VariableType::Number) {
					if (b->Type == VariableType::Number) Arithmatic_SubstractNumber(ret, a, b);
				}
				break;

			case Misc::ArithmaticFuncs::Divide:
				if (a->Type == VariableType::Number) {
					if (b->Type == VariableType::Number) Arithmatic_DivideNumber(ret, a, b);
				} else if (a->Type == VariableType::String) {
					if (b->Type == VariableType::Number) Arithmatic_DivideString(data->BromScript, ret, a, b);
				}
				break;

			case Misc::ArithmaticFuncs::Multiply:
				if (a->Type == VariableType::Number) {
					if (b->Type == VariableType::Number) Arithmatic_MultiplyNumber(ret, a, b);
					else if (b->Type == VariableType::String) Arithmatic_MultiplyString(ret, a, b);
				} else if (a->Type == VariableType::String) {
					if (b->Type == VariableType::Number) Arithmatic_MultiplyString(ret, b, a); // rotated a and b, usage num and string
				}
				break;

			case Misc::ArithmaticFuncs::NotEqual: // we reverse this later on, saves code
			case Misc::ArithmaticFuncs::Equal:
				ret->Type = VariableType::Bool;
				if (a->Type == VariableType::Bool && b->Type == VariableType::Null && a->Value == null) ret->Value = Converter::BoolToPointer(true);
				else if (b->Type == VariableType::Bool && a->Type == VariableType::Null && b->Value != null) ret->Value = Converter::BoolToPointer(true);
				else if (a->Type != b->Type) ret->Value = Converter::BoolToPointer(false);
				else if (a->Type == VariableType::Bool) ret->Value = Converter::BoolToPointer(a->Value == b->Value);
				else if (a->Type == VariableType::Number) ret->Value = Converter::BoolToPointer(*(double*)a->Value == *(double*)b->Value);
				else if (a->Type == VariableType::String) ret->Value = Converter::BoolToPointer(*(CString*)a->Value == *(CString*)b->Value);
				else ret->Value = Converter::BoolToPointer(a->Value == b->Value);
				break;

			case Misc::ArithmaticFuncs::GreaterThan:
				ret->Type = VariableType::Bool;
				if (a->Type != b->Type || a->Type != VariableType::Number) {
					ret->Value = Converter::BoolToPointer(false);
				} else {
					ret->Value = Converter::BoolToPointer(*(double*)a->Value > *(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::GreaterOrEqual:
				ret->Type = VariableType::Bool;
				if (a->Type != b->Type || a->Type != VariableType::Number) {
					ret->Value = Converter::BoolToPointer(false);
				} else {
					ret->Value = Converter::BoolToPointer(*(double*)a->Value >= *(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::LessThan:
				ret->Type = VariableType::Bool;
				if (a->Type != b->Type || a->Type != VariableType::Number) {
					ret->Value = Converter::BoolToPointer(false);
				} else {
					ret->Value = Converter::BoolToPointer(*(double*)a->Value < *(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::LessOrEqual:
				ret->Type = VariableType::Bool;
				if (a->Type != b->Type || a->Type != VariableType::Number) {
					ret->Value = Converter::BoolToPointer(false);
				} else {
					ret->Value = Converter::BoolToPointer(*(double*)a->Value <= *(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::Modulo:
				if (a->Type == VariableType::Number && b->Type == VariableType::Number) {
					ret->Type = VariableType::Number;
					ret->Value = Converter::NumberToPointer(fmod((double)*(double*)a->Value, *(double*)b->Value));
				} else if (a->Type == VariableType::String && b->Type == VariableType::Table) {
					ret->Type = VariableType::String;

					Table* tbl = b->GetTable();
					CString strRet;

					char* szOffset = ((CString*)a->Value)->str_szBuffer;
					char* szOffsetPrev = szOffset;

					int tblpos = tbl->GetNextIndex(0);
					do {
						szOffset = strstr(szOffset, "%");

						if (szOffset != NULL) {
							if (szOffsetPrev != szOffset && szOffset[-1] == '\\') { // -1 to go back, this is allowed, however, it feels like YOLO.
								szOffset++;
							} else {
								strRet.AppendToBuffer(szOffsetPrev, szOffset - szOffsetPrev);
								strRet += tbl->GetByIndex(tblpos)->ToString();

								szOffset++;
								szOffsetPrev = szOffset;

								tblpos = tbl->GetNextIndex(++tblpos);
							}
						} else {
							strRet.AppendToBuffer(szOffsetPrev, strlen(szOffsetPrev));
						}
					} while (szOffset != NULL);

					ret->Value = Converter::StringToPointer(strRet.Replace("\\%", "%"));
				}
				break;

			case Misc::ArithmaticFuncs::And:
				ret->Type = VariableType::Bool;
				ret->Value = Converter::BoolToPointer(CheckIfVarTrue(a) && CheckIfVarTrue(b));
				break;

			case Misc::ArithmaticFuncs::Or:
				ret->Type = VariableType::Bool;
				ret->Value = Converter::BoolToPointer(CheckIfVarTrue(a) || CheckIfVarTrue(b));
				break;

			case Misc::ArithmaticFuncs::BitwiseLeft:
				if (a->Type == VariableType::Number && b->Type == VariableType::Number) {
					ret->Type = VariableType::Number;
					ret->Value = Converter::NumberToPointer((int)*(double*)a->Value << (int)*(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::BitwiseRight:
				if (a->Type == VariableType::Number && b->Type == VariableType::Number) {
					ret->Type = VariableType::Number;
					ret->Value = Converter::NumberToPointer((int)*(double*)a->Value >> (int)*(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::BitwiseOr:
				if (a->Type == VariableType::Number && b->Type == VariableType::Number) {
					ret->Type = VariableType::Number;
					ret->Value = Converter::NumberToPointer((int)*(double*)a->Value | (int)*(double*)b->Value);
				}
				break;

			case Misc::ArithmaticFuncs::BitwiseAnd:
				if (a->Type == VariableType::Number && b->Type == VariableType::Number) {
					ret->Type = VariableType::Number;
					ret->Value = Converter::NumberToPointer((int)*(double*)a->Value & (int)*(double*)b->Value);
				}
				break;
		}

		if (af == Misc::ArithmaticFuncs::NotEqual) // just rotate around equal, less code, same outcome.
			ret->Value = Converter::BoolToPointer(ret->Value == null);

		if (ret->Type == VariableType::Null)
			BS_THROW_ERROR(data->BromScript, CString::Format("Invalid arithmatic function!, trying to do %s %s %s", Converter::TypeToString(a).str_szBuffer, Converter::ArithmaticToString(af).str_szBuffer, Converter::TypeToString(b).str_szBuffer));

		return ret;
	}

	void Executer::StringTable(ExecuteData* data) {
		int jumppos = data->Reader->ReadInt();
		int oldpos = data->Reader->Pos;

		data->Reader->Pos = jumppos;

		int num = data->Reader->ReadShort();
		data->Function->StringTableCount = num;
		data->Function->StringTableVars = new Variable*[num];
		data->Function->StringTable = new CString[num];
		for (int i = 0; i < num; i++) {
			int strlen = data->Reader->ReadShort();
			byte* tmpbuff = data->Reader->ReadBytes(strlen);

			char* destbuff = new char[strlen + 1];
			memcpy(destbuff, tmpbuff, strlen);
			destbuff[strlen] = 0;

			data->Function->StringTable[i].str_szBuffer = destbuff;

			data->Function->StringTableVars[i] = data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, destbuff));
			BS_REF_INCREESE(data->Function->StringTableVars[i]);

			delete[] tmpbuff;
		}

		data->Reader->Pos = oldpos;
		data->Reader->StringTable = data->Function->StringTable; // we need to reset this due we create if above
	}

	void Executer::GlobalLocals(ExecuteData* data) {
		int jumppos = data->Reader->ReadInt();
		int oldpos = data->Reader->Pos;

		data->Reader->Pos = jumppos;

		int num = data->Reader->ReadInt();
		data->Function->FixedLocalsCount = num;
		data->Function->FixedLocalVars = new Variable*[num];
		data->Function->FixedLocalKeys = new CString[num];

		for (int i = 0; i < num; i++) {
			CString key = data->Reader->ReadString();
			int type = data->Reader->ReadInt();

			data->Function->FixedLocalKeys[i] = key;
			data->Function->FixedLocalVars[i] = null;
		}

		data->Reader->Pos = oldpos;
	}

	void Executer::Duplicate(ExecuteData* data) {
		data->PushStack(data->Stack[data->StackPos - 1]);
	}

	void Executer::AddIndex(ExecuteData* data) {
		Variable* value = data->PopStack();
		Variable* tbl = data->Stack[data->StackPos - 1]; // don't pop on AddIndex, this is used for initializers

		bool haskey = data->Reader->ReadBool();

		CString key = haskey ? data->Reader->ReadString() : CString::Format("%d", tbl->GetTable()->Count);
		if (tbl->Type == VariableType::Table) {
			tbl->GetTable()->Set(key, value);
			return;
		}

		if (tbl->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)tbl->Value;
			udi->SetIndex(tbl, data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, key)), value);
			return;
		}

		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot add an index on a %s", Converter::TypeToString(data->BromScript, tbl->Type).str_szBuffer));
	}

	void Executer::GetCount(ExecuteData* data) {
		Variable* var = data->PopStack();

		if (var->Type == VariableType::Table) {
			data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, var->GetTable()->Count)));
			return;
		}

		if (var->Type == VariableType::String) {
			data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, var->GetString().Size())));
			return;
		}

		if (var->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)var->Value;
			if (udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticGetCount)] != null) {
				ArgumentData args;
				args.BromScript = data->BromScript;
				args.Caller = data->Function;
				args.SetThisObject(var);

				Variable* ret = udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticGetCount)](data->BromScript, &args);
				args.Clear();

				if (ret == null) ret = data->BromScript->GetDefaultVarNull();
				else data->BromScript->GC.RegisterVariable(ret);

				data->PushStack(ret);
				return;
			}
		}

		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot count object %s", Converter::TypeToString(data->BromScript, var->Type).str_szBuffer));
	}

	void Executer::GetIndex(ExecuteData* data) {
		Variable* keyvar = data->PopStack();
		Variable* tbl = data->PopStack();

		if (tbl->Type == VariableType::Table) {
			data->PushStack(tbl->GetTable()->Get(keyvar->ToString(data->BromScript)));
			return;
		}

		if (tbl->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)tbl->Value;

			data->PushStack(udi->GetIndex(tbl, keyvar));
			return;
		}

		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot get an index on a %s", Converter::TypeToString(data->BromScript, tbl->Type).str_szBuffer));
	}

	void Executer::SetIndex(ExecuteData* data) {
		Variable* key = data->PopStack();
		Variable* tbl = data->PopStack();
		Variable* value = data->PopStack();

		if (tbl->Type == VariableType::Table) {
			if (value->Type == VariableType::Null) {
				tbl->GetTable()->Remove(key->ToString(data->BromScript));
			} else {
				tbl->GetTable()->Set(key->ToString(data->BromScript), value);
			}
			return;
		}

		if (tbl->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)tbl->Value;
			udi->SetIndex(tbl, key, value);
			return;
		}

		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot set an index on a %s", Converter::TypeToString(data->BromScript, tbl->Type).str_szBuffer));
	}

	void Executer::Set(ExecuteData* data) {
		CString name = data->Reader->ReadString();
		Variable* var = data->PopStack();
		data->Function->SetVar(name, var, true);

		// for better debug, else we'd have hard to debug function names
		if (var->Type == VariableType::Function) {
			var->GetFunction()->Name = name;
		}
	}

	void Executer::SetL(ExecuteData* data) {
		Variable* var = data->PopStack();
		int lindex = data->Reader->ReadInt();
		data->Function->SetVar(var, lindex);

		// for better debug, else we'd have hard to debug function names
		if (var->Type == VariableType::Function) {
			var->GetFunction()->Name = data->Function->FixedLocalKeys[lindex];
		}
	}

	void Executer::New(ExecuteData* data) {
		Variable* classobj = data->PopStack();

		if (classobj->Type == VariableType::Null) {
			BS_THROW_ERROR(data->BromScript, "Cannot create an instance of null");
			return;
		}

		if (classobj->Type != VariableType::Class) {
			BS_THROW_ERROR(data->BromScript, "Attempting to create an instance of non class!");
			return;
		}

		int params = data->Reader->ReadInt();
		ArgumentData args;
		args.BromScript = data->BromScript;
		for (int i = 0; i < params; i++) {
			args.AddVariable(data->PopStack());
		}

		Function* func = (Function*)classobj->Value;
		if (func->IsCpp) {
			Userdata* ud = data->BromScript->GetRegisteredUserdata(func->Name);

			if (ud == null) {
				BS_THROW_ERROR(data->BromScript, "Trying to create an unknown class");
				return;
			}

			if (ud->Ctor == null) {
				BS_THROW_ERROR(data->BromScript, "Cannot create type, no constructor");
				return;
			}

			void* dataptr = ud->Ctor(data->BromScript, &args);
			if (dataptr == null) {
				data->PushStack(data->BromScript->GetDefaultVarNull());
			} else {
				UserdataInstance* udi = new UserdataInstance();
				udi->TypeData = ud;
				udi->Ptr = dataptr;
				udi->CallDTor = true;

				Variable* ret = data->BromScript->GC.GetPooledVariable();
				ret->Type = (VariableType::Enum)ud->TypeID;
				ret->Value = udi;

				data->PushStack(ret);
			}
		} else {
			// TODO: do ieeet
			BS_THROW_ERROR(data->BromScript, "BS classes not supported yet :(");
		}
	}

	void Executer::Call(ExecuteData* data) {
		ArgumentData* args = data->BromScript->GC.ArgumentsPool.GetNext();
		if (args == null) args = new ArgumentData();


		args->BromScript = data->BromScript;
		args->Caller = data->Function;
		args->BromScript = data->BromScript;

		int argcount = data->Reader->ReadInt();
		for (int i = 0; i < argcount; i++) {
			args->AddVariable(data->PopStack());
		}

		Variable* var = data->PopStack();
		if (var->Type == VariableType::Function) {
			data->PushStack(var->GetFunction()->Run(args));

			args->Clear();
			data->BromScript->GC.ArgumentsPool.Free(args);
			return;
		}

		if (var->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)var->Value;
			if (udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticCall)] != null) {
				Variable* ret = udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticCall)](data->BromScript, args);
				args->Clear();

				if (ret == null) ret = data->BromScript->GetDefaultVarNull();
				else data->BromScript->GC.RegisterVariable(ret);

				data->PushStack(ret);

				args->Clear();
				data->BromScript->GC.ArgumentsPool.Free(args);
				return;
			}
		}

		args->Clear();
		data->BromScript->GC.ArgumentsPool.Free(args);
		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot call %s", Converter::TypeToString(data->BromScript, var->Type).str_szBuffer).str_szBuffer);
	}

	void Executer::CallThis(ExecuteData* data) {
		ArgumentData* args = data->BromScript->GC.ArgumentsPool.GetNext();
		if (args == null) args = new ArgumentData();

		args->BromScript = data->BromScript;

		int argcount = data->Reader->ReadInt();
		for (int i = 0; i < argcount; i++) {
			args->AddVariable(data->PopStack());
		}

		Variable* var = data->PopStack();
		Variable* thisvar = data->PopStack();
		args->Caller = data->Function;
		args->SetThisObject(thisvar);

		if (var->Type == VariableType::Function) {
			data->PushStack(var->GetFunction()->Run(args, thisvar));

			args->Clear();
			data->BromScript->GC.ArgumentsPool.Free(args);
			return;
		}

		if (var->Type > VariableType::Userdata) {
			UserdataInstance* udi = (UserdataInstance*)var->Value;
			if (udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticCall)] != null) {
				Variable* ret = udi->TypeData->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticCall)](data->BromScript, args);
				args->Clear();

				if (ret == null) ret = data->BromScript->GetDefaultVarNull();
				else data->BromScript->GC.RegisterVariable(ret);

				data->PushStack(ret);

				args->Clear();
				data->BromScript->GC.ArgumentsPool.Free(args);
				return;
			}
		}

		args->Clear();
		data->BromScript->GC.ArgumentsPool.Free(args);
		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot call %s", Converter::TypeToString(data->BromScript, var->Type).str_szBuffer).str_szBuffer);
	}

	void Executer::PostIncrement(ExecuteData* data) {
		Variable* var = data->PopStack();

		if (var->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Post-Increment a non-number");
			return;
		}

		(*(double*)var->Value)++;
		data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, var->GetNumber() - 1)));
	}

	void Executer::PreIncrement(ExecuteData* data) {
		Variable* var = data->PopStack();

		if (var->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Pre-Increment a non-number");
			return;
		}

		(*(double*)var->Value)++;
		data->PushStack(var);
	}

	void Executer::Pop(ExecuteData* data) {
		data->PopStack();
	}

	void Executer::Delete(ExecuteData* data) {
		data->PopStack()->EmptyValue();
	}

	void Executer::StackNull(ExecuteData* data) {
		data->PushStack(data->BromScript->GetDefaultVarNull());
	}

	void Executer::StackBool(ExecuteData* data) {
		data->PushStack(data->Reader->ReadBool() ? data->BromScript->GetDefaultVarTrue() : data->BromScript->GetDefaultVarFalse());
	}

	void Executer::StackNumber(ExecuteData* data) {
		data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, data->Reader->ReadDouble())));
	}

	void Executer::StackTable(ExecuteData* data) {
		data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, new Table(data->BromScript))));
	}

	void Executer::StackString(ExecuteData* data) {
		data->PushStack(data->BromScript->GC.RegisterVariable(Converter::ToVariable(data->BromScript, data->Reader->ReadString())));
	}

	void Executer::StackFunction(ExecuteData* data) {
		CString filename = data->Reader->ReadString();

		Function* func = new Function(data->BromScript);
		func->Name = "AnonFunction";
		func->Filename = filename;
		func->Parent = data->Function;

		Variable* var = data->BromScript->GC.GetPooledVariable();
		var->Value = func;
		var->Type = VariableType::Function;

		List<CString>* params = data->Reader->ReadStrings();
		for (int i = 0; i < params->Count; i++) {
			func->Parameters.Add(CString(params->Get(i)));
		}

		int localscount = data->Reader->ReadInt();
		func->FixedLocalsCount = localscount;
		func->FixedLocalVars = new Variable*[localscount];
		func->FixedLocalKeys = new CString[localscount];
		for (int i = 0; i < localscount; i++) {
			func->FixedLocalKeys[i] = data->Reader->ReadString();

			// TODO: make type checking
			data->Reader->ReadInt(); // we don't care about the type, we're dynamic maaaaan, whooooooo =)
			func->FixedLocalVars[i] = null;
		}

		int codelen = data->Reader->ReadInt();
		func->SetReferences(data->Function, data->Reader->Pos);
		data->Reader->Pos += codelen;

		delete params;

		data->PushStack(var);
	}

	void Executer::JumpNT(ExecuteData* data) {
		Variable* var = data->PopStack();
		int jumppos = data->Reader->ReadInt();

		if (!CheckIfVarTrue(var)) {
			data->Reader->Pos = jumppos;
		}
	}

	void Executer::Jump(ExecuteData* data) {
		data->Reader->Pos = data->Reader->ReadInt();
	}

	void Executer::Get(ExecuteData* data) {
		data->PushStack(data->Function->GetVar(data->Reader->ReadString()));
	}

	void Executer::GetL(ExecuteData* data) {
		Variable* var = data->Function->FixedLocalVars[data->Reader->ReadInt()];
		data->PushStack(var != null ? var : data->BromScript->GetDefaultVarNull());
	}

	void Executer::CurrentLine(ExecuteData* data) {
		data->Function->CurrentSourceFileLine = data->Reader->ReadInt();
		data->BromScript->Debug->Update();
	}
}
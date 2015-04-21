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

#include "../Managers/Executer.h"
#include "../Managers/Converter.h"
#include "../Objects/Variable.h"
#include "../Objects/Function.h"
#include "../Objects/Table.h"
#include "../Objects/UserdataInstance.h"

#include <cstring>

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

	Variable* Executer::PostIncrement(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* obj = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (obj->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Post-Increment a non-number");
			return data->BromScript->GetDefaultVarNull();
		}

		(*(double*)obj->Value)++;

		return data->BromScript->GC.RegisterVariable(Converter::ToVariable(obj->GetNumber() - 1));
	}

	Variable* Executer::PreIncrement(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* obj = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (obj->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Pre-Increment a non-number");
			return data->BromScript->GetDefaultVarNull();
		}

		(*(double*)obj->Value)++;

		return obj;
	}

	Variable* Executer::PostDecrement(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* obj = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (obj->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Post-Decrement a non-number");
			return data->BromScript->GetDefaultVarNull();
		}

		(*(double*)obj->Value)--;

		return data->BromScript->GC.RegisterVariable(Converter::ToVariable(obj->GetNumber() + 1));
	}

	Variable* Executer::PreDecrement(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* obj = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (obj->Type != VariableType::Number) {
			BS_THROW_ERROR(data->BromScript, "Cannot Pre-Decrement a non-number");
			return data->BromScript->GetDefaultVarNull();
		}

		(*(double*)obj->Value)--;

		return obj;
	}

	Variable* Executer::New(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* classobj = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (classobj->Type == VariableType::Null) {
			BS_THROW_ERROR(data->BromScript, "Cannot create an instance of null");
			return data->BromScript->GetDefaultVarNull();
		}

		if (classobj->Type != VariableType::Class) {
			BS_THROW_ERROR(data->BromScript, "Attempting to create a non class!");
			return data->BromScript->GetDefaultVarNull();
		}

		int params = data->Reader->ReadInt();
		List<Variable*> vars;
		for (int i = 0; i < params; i++) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			vars.Add(data->Function->InternalRun(data));
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return data->Returning;
		}

		List<CString> setskeys;
		List<Variable*> setsvalues;
		int sets = data->Reader->ReadInt();
		for (int i = 0; i < sets; i++) {
			setskeys.Add(data->Reader->ReadString());

			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			setsvalues.Add(data->Function->InternalRun(data));
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return data->Returning;
		}

		ArgumentData* args = new ArgumentData();
		args->BromScript = data->BromScript;
		args->SetVariableData(&vars);

		Function* func = (Function*)classobj->Value;
		if (func->IsCpp) {
			Userdata* ud = null;
			for (int i = 0; i < data->BromScript->RegisteredUserdataTypes.Count; i++) {
				if (data->BromScript->RegisteredUserdataTypes[i]->Name == func->Name) {
					ud = data->BromScript->RegisteredUserdataTypes[i];
					break;
				}
			}

			if (ud->Ctor == null) {
				BS_THROW_ERROR(data->BromScript, "Cannot create type, no constructor found!");
				return data->BromScript->GetDefaultVarNull();
			}

			UserdataInstance* udi = new UserdataInstance();
			udi->TypeData = ud;
			udi->Ptr = ud->Ctor(data->BromScript, args);
			udi->CallDTor = true;

			Variable* ret = data->BromScript->GC.RegisterVariable();
			ret->Type = (VariableType::Enum)ud->TypeID;
			ret->Value = udi;

			for (int i = 0; i < sets; i++) {
				bool found = false;
				for (int i2 = 0; i2 < ud->Members.Count; i2++) {
					if (ud->Members[i2]->Name == setskeys[i]) {
						UserdataInstance* udi2 = new UserdataInstance();
						udi2->TypeData = ud->Members[i2];
						udi2->Ptr = (char*)udi->Ptr + ud->Members[i2]->Offset;
						udi2->CallDTor = true;

						Variable* tmpvar = new Variable();
						tmpvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
						tmpvar->Value = udi2;
						Converter::SetMember(data->BromScript, tmpvar, setsvalues[i], setskeys[i]);
						delete tmpvar;

						found = true;
						break;
					}
				}

				if (!found) {
					delete args;
					BS_THROW_ERROR(data->BromScript, CString::Format("Cannot find index %s on object type %s", setskeys[i].str_szBuffer, ud->Name.str_szBuffer));
					return ret;
				}
			}

			delete args;
			return ret;

		} else {
			Table* tbl = new Table(data->BromScript);
			Variable* ret = data->BromScript->GC.RegisterVariable(Converter::ToVariable(tbl));

			func->Run(args, ret);

			for (int i = 0; i < sets; i++) {
				tbl->Set(setskeys[i], setsvalues[i]);
			}

			tbl->Set("__TYPE", Converter::ToVariable(func->Name.Substring(0, -5)));
			tbl->Get("__TYPE")->ReadOnly = true;

			delete args;
			return ret;
		}
	}

	void Executer::CreateClass(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();
		CString classname = data->Reader->ReadString();
		List<CString>* paras = data->Reader->ReadStrings();

		int codelen = data->Reader->ReadInt();
		byte* code = data->Reader->ReadBytes(codelen);

		Function* func = new Function(data->BromScript);
		func->Name = classname + "_CTOR";
		func->Filename = data->Function->Filename;
		func->Parent = data->Function;
		func->Code = code;
		func->SetReferences(data->Function, data->Reader->Pos - codelen);

		Variable* var = data->BromScript->GC.RegisterVariable();
		var->Value = func;
		var->Type = VariableType::Class;

		for (int i = 0; i < paras->Count; i++)
			func->Parameters.Add(CString(paras->Get(i)));

		delete paras;

		data->Function->SetVar(classname, var, !islocal);
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

			data->Function->StringTableVars[i] = data->BromScript->GC.RegisterVariable(Converter::ToVariable(destbuff));
			BS_REF_INCREESE(data->Function->StringTableVars[i]);

			delete[] tmpbuff;
		}

		data->Reader->Pos = oldpos;
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

	Variable* Executer::CreateAnonFunction(ExecuteData* data) {
		CString filename = data->Reader->ReadString();
		int codelen = data->Reader->ReadInt();

		Function* func = new Function(data->BromScript);
		func->Name = "AnonFunction";
		func->Filename = filename;
		func->Parent = data->Function;
		func->SetReferences(data->Function, data->Reader->Pos);

		data->Reader->Pos += codelen;

		Variable* var = data->BromScript->GC.RegisterVariable();
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

			data->Reader->ReadInt(); // we don't care about the type, we're dynamic maaaaan =)
			func->FixedLocalVars[i] = null;
		}

		delete params;
		return var;
	}

	void Executer::CreateFunction(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();

		bool islocalvar = false;
		int localindex = 0;
		if (islocal) {
			localindex = data->Reader->ReadInt();
			islocalvar = true;
		} else {
			islocalvar = data->Reader->ReadBool();
			localindex = islocalvar ? data->Reader->ReadInt() : 0;
		}

		CString funcname = islocalvar ? data->Function->FixedLocalKeys[localindex] : data->Reader->ReadString();
		CString filename = data->Reader->ReadString();
		int codelen = data->Reader->ReadInt();

		Function* func = new Function(data->BromScript);
		func->Name = funcname;
		func->Filename = filename;
		func->Parent = data->Function;
		func->SetReferences(data->Function, data->Reader->Pos);

		data->Reader->Pos += codelen;

		Variable* var = data->BromScript->GC.RegisterVariable();
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

			data->Reader->ReadInt(); // we don't care about the type, we're dynamic maaaaan =)
			func->FixedLocalVars[i] = null;
		}

		delete params;

		List<CString>* keys = data->Reader->ReadStrings();

		if (keys->Count == 1) {
			if (islocalvar) {
				data->Function->SetVar(funcname, var, localindex);
			} else {
				data->Function->SetVar(funcname, var, !islocal);
			}
		} else {
			Variable* curvar = islocalvar ? data->Function->FixedLocalVars[localindex] : data->Function->GetVar(funcname);
			Variable* prevvar = null;

			if (curvar->Type == VariableType::Null) {
				BS_THROW_ERROR(data->BromScript, "Cannot index a null value");
				delete keys;
				return;
			}

			if (curvar->Type != VariableType::Table) {
				BS_THROW_ERROR(data->BromScript, "Cannot index a non table value");
				delete keys;
				return;
			}

			for (int i = 1; i < keys->Count; i++) {
				if (i == keys->Count - 1) {
					if (curvar->Type == VariableType::Null) {
						BS_THROW_ERROR(data->BromScript, "Cannot index a null value");
						delete keys;
						return;
					}

					if (curvar->Type != VariableType::Table && curvar->Type <= VariableType::Userdata) {
						BS_THROW_ERROR(data->BromScript, "Cannot index a non table value");
						delete keys;
						return;
					}

					CString endkey = keys->Get(keys->Count - 1);
					if (curvar->Type == VariableType::Table) {
						Table* tbl = (Table*)curvar->Value;
						tbl->Set(endkey, var);
						delete keys;
						return;
					} else {
						bool found = false;
						UserdataInstance* udi = (UserdataInstance*)prevvar->Value;
						for (int i2 = 0; i2 < udi->TypeData->Members.Count; i2++) {
							if (udi->TypeData->Members[i2]->Name == endkey) {
								UserdataInstance* udi2 = new UserdataInstance();
								udi2->TypeData = udi->TypeData->Members[i2];
								udi2->Ptr = (byte*)udi->Ptr + udi2->TypeData->Offset;

								curvar->Value = udi2;
								curvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
								curvar->IsCpp = true;
								found = true;

								if (!Converter::SetMember(data->BromScript, curvar, var, endkey)) {
									BS_THROW_ERROR(data->BromScript, "Failed to set member");
									delete keys;
									return;
								}

								delete keys;
								return;
							}
						}

						BS_THROW_ERROR(data->BromScript, CString::Format("Unknown userdata index '%s'", endkey.str_szBuffer));
					}

					delete keys;
					return;
				}

				if (curvar->Type == VariableType::Table) {
					Table* CurrentTable = (Table*)curvar->Value;

					prevvar = curvar;
					curvar = CurrentTable->Get(keys->Get(i));
				} else if (curvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)curvar->Value;

					prevvar = curvar;
					curvar = data->BromScript->GC.RegisterVariable();

					bool found = false;
					for (int i2 = 0; i2 < udi->TypeData->Members.Count; i2++) {
						if (udi->TypeData->Members[i2]->Name == keys->Get(i)) {
							UserdataInstance* udi2 = new UserdataInstance();
							udi2->TypeData = udi->TypeData->Members[i2];
							udi2->Ptr = (byte*)udi->Ptr + udi2->TypeData->Offset;

							curvar->Value = udi2;
							curvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
							curvar->IsCpp = true;
							found = true;
							break;
						}
					}

					if (!found) {
						for (int i2 = 0; i2 < udi->TypeData->Functions.Count(); i2++) {
							if (udi->TypeData->Functions.GetKeyByIndex(i2) == keys->Get(i)) {
								Function* func = new Function(data->BromScript);
								func->CppFunc = udi->TypeData->Functions.GetValueByIndex(i2);
								func->IsCpp = true;
								func->Name = keys->Get(i);
								func->Filename = "C++";
								func->SetReferences(data->Function, 0);

								curvar->Type = VariableType::Function;
								curvar->Value = func;

								found = true;
								break;
							}
						}

						if (!found) {
							if (udi->TypeData->Operators[Misc::ArithmaticFuncs::GetIndex - 100] != null) {
								ArgumentData* args = new ArgumentData();
								args->BromScript = data->BromScript;
								args->Caller = data->Function;
								args->AddVariable(data->BromScript->GC.RegisterVariable(Converter::ToVariable(keys->Get(i).str_szBuffer)));
								args->ThisObject = prevvar;

								Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::GetIndex - 100](data->BromScript, args);
								delete args;

								if (ret == null) ret = data->BromScript->GetDefaultVarNull();
								else data->BromScript->GC.RegisterVariable(ret);

								prevvar = curvar;
								curvar = ret;
							} else {
								BS_THROW_ERROR(data->BromScript, CString::Format("Unknown userdata index '%s'", keys->Get(i).str_szBuffer));
								delete keys;
								return;
							}
						}
					}
				} else {
					BS_THROW_ERROR(data->BromScript, "Cannot index a non table value");
					delete keys;
					return;
				}
			}
		}

		delete keys;
	}

	void Executer::SetTableIndex(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();

		Variable* curvar = null;
		CString tblname;
		if (islocal) {
			int localindex = data->Reader->ReadInt();
			tblname = data->Function->FixedLocalKeys[localindex];
			curvar = data->Function->FixedLocalVars[localindex];
		} else {
			tblname = data->Reader->ReadString();
			curvar = data->Function->GetVar(tblname);
		}

		BS_REF_INCREESE(curvar);

		CString fullvarname = tblname;
		int loops = data->Reader->ReadByte();

		Variable* prevkeyvar = null;
		Variable* prevvar = null;

		Variable* var = null;

		CString key;

		for (int i = 0; i <= loops; i++) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			Variable* keyvar = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return;

			if (i == loops) {
				if (curvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)curvar->Value;
					if (udi->TypeData->Operators[Misc::ArithmaticFuncs::SetIndex - 100] != null) {
						ArgumentData* args = new ArgumentData();
						args->BromScript = data->BromScript;
						args->Caller = data->Function;
						args->AddVariable(prevkeyvar);
						args->AddVariable(keyvar);
						args->ThisObject = curvar;

						Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::GetIndex - 100](data->BromScript, args);
						if (ret != null) data->BromScript->GC.RegisterVariable(ret);

						delete args;

						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return;
					}

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);

					if (!Converter::SetMember(data->BromScript, curvar, keyvar, key)) {
						BS_THROW_ERROR(data->BromScript, "Failed to set member");
						return;
					}

					return;
				} else if (prevvar->Type == VariableType::Table) {
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);

					Table* tbl = (Table*)prevvar->Value;
					if (tbl->Get(key)->ReadOnly) {
						BS_THROW_ERROR(data->BromScript, CString::Format("Cannot override '%s' it's read only!", fullvarname.str_szBuffer));
						return;
					}

					if (keyvar->Type == VariableType::Null) {
						tbl->Remove(key);
					} else {
						tbl->Set(key, keyvar);
					}
					return;
				} else {
					BS_THROW_ERROR(data->BromScript, CString::Format("Cannot set index of non-indexable value", fullvarname.str_szBuffer));

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);
					return;
				}
			}

			if (keyvar->Type == VariableType::Bool) key = keyvar->Value == null ? "False" : "True";
			else if (keyvar->Type == VariableType::Number) key = CString(*(double*)keyvar->Value);
			else if (keyvar->Type == VariableType::String) key = *(CString*)keyvar->Value;
			else {
				BS_THROW_ERROR(data->BromScript, CString::Format("Cannot index a table value using keytype %s", Converter::TypeToString(keyvar).str_szBuffer));

				if (prevvar != null) BS_REF_DECREESE(prevvar);
				BS_REF_DECREESE(curvar);
				return;
			}

			if (key.Size() > 0) {
				if (curvar->Type == VariableType::Table) {
					Table* CurrentTable = (Table*)curvar->Value;

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = CurrentTable->Get(key);
					BS_REF_INCREESE(curvar);
				} else if (curvar->Type == VariableType::String) {
					if (keyvar->Type != VariableType::Number) {
						BS_THROW_ERROR(data->BromScript, CString::Format("Cannot index a string value using keytype %s", Converter::TypeToString(keyvar).str_szBuffer));

						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return;
					}

					CString tmpstr = ((CString*)curvar->Value)->str_szBuffer[(int)keyvar->GetNumber()];

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = Converter::ToVariable(tmpstr);
					BS_REF_INCREESE(curvar);
				} else if (curvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)curvar->Value;

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = data->BromScript->GC.RegisterVariable();
					BS_REF_INCREESE(curvar);

					bool found = false;
					for (int i = 0; i < udi->TypeData->Members.Count; i++) {
						if (udi->TypeData->Members[i]->Name == key) {
							UserdataInstance* udi2 = new UserdataInstance();
							udi2->TypeData = udi->TypeData->Members[i];
							udi2->Ptr = (byte*)udi->Ptr + udi2->TypeData->Offset;

							curvar->Value = udi2;
							curvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
							curvar->IsCpp = true;
							found = true;
							break;
						}
					}

					if (!found) {
						for (int i = 0; i < udi->TypeData->Functions.Count(); i++) {
							if (udi->TypeData->Functions.GetKeyByIndex(i) == key) {
								Function* func = new Function(data->BromScript);
								func->CppFunc = udi->TypeData->Functions.GetValueByIndex(i);
								func->IsCpp = true;
								func->Name = key;
								func->Filename = "C++";
								func->SetReferences(data->Function, 0);

								curvar->Type = VariableType::Function;
								curvar->Value = func;

								found = true;
								break;
							}
						}

						if (!found) {
							if (udi->TypeData->Operators[Misc::ArithmaticFuncs::GetIndex - 100] != null) {
								ArgumentData* args = new ArgumentData();
								args->BromScript = data->BromScript;
								args->Caller = data->Function;
								args->AddVariable(keyvar);
								args->ThisObject = prevvar;

								Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::GetIndex - 100](data->BromScript, args);
								delete args;

								if (ret == null) ret = data->BromScript->GetDefaultVarNull();
								else data->BromScript->GC.RegisterVariable(ret);

								if (prevvar != null) BS_REF_DECREESE(prevvar);
								prevvar = curvar;
								curvar = ret;
								BS_REF_INCREESE(curvar);
							} else {
								BS_THROW_ERROR(data->BromScript, CString::Format("Unknown userdata index '%s'", fullvarname.str_szBuffer));

								if (prevvar != null) BS_REF_DECREESE(prevvar);
								BS_REF_DECREESE(curvar);
								return;
							}
						}
					}
				} else {
					BS_THROW_ERROR(data->BromScript, "Cannot index a non table value");

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);
					return;
				}
			}

			bool exec = data->Reader->ReadBool();
			if (exec) {
				if (curvar->Type != VariableType::Function && (prevvar->Type != VariableType::Userdata || ((UserdataInstance*)prevvar->Value)->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100])) {
					BS_THROW_ERROR(data->BromScript, CString::Format("Cannot execute a non function value, varname: %s", fullvarname.str_szBuffer));

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);
					return;
				}

				int params = data->Reader->ReadInt();
				List<Variable*> vars;

				for (int i = 0; i < params; i++) {
					bool oldcall = data->FuncCallback;
					data->FuncCallback = true;
					vars.Add(data->Function->InternalRun(data));
					data->FuncCallback = oldcall;

					if (data->HasReturnValue || data->Function->ForceReturn) {
						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return;
					}
				}

				if (curvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)curvar->Value;
					ArgumentData* args = new ArgumentData();
					args->BromScript = data->BromScript;
					args->Caller = data->Function;
					args->SetVariableData(&vars);
					args->ThisObject = prevvar;

					Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100](data->BromScript, args);
					delete args;

					if (ret == null) ret = data->BromScript->GetDefaultVarNull();
					else data->BromScript->GC.RegisterVariable(ret);

					Variable* tmp = prevvar;
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = ret;
					BS_REF_INCREESE(curvar);
				} else {
					Function* func = (Function*)curvar->Value;
					Variable* tmp = prevvar;
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = func->Run(&vars, tmp);
					BS_REF_INCREESE(curvar);

					if (data->HasReturnValue || data->Function->ForceReturn) {
						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return;
					}
				}
			}

			fullvarname += CString::Format("[%s]", key.str_szBuffer);
			prevkeyvar = keyvar;
		}
	}

	Variable* Executer::GetTableIndex(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();

		Variable* curvar = null;
		CString tblname;
		if (islocal) {
			int localindex = data->Reader->ReadInt();
			tblname = data->Function->FixedLocalKeys[localindex];
			curvar = data->Function->FixedLocalVars[localindex];
			if (curvar == null) curvar = data->BromScript->GetDefaultVarNull();
		} else {
			tblname = data->Reader->ReadString();
			curvar = data->Function->GetVar(tblname);
		}
		BS_REF_INCREESE(curvar);

		CString fullvarname = tblname;
		int loops = data->Reader->ReadByte();

		Variable* keyvar = null;
		Variable* prevvar = null;

		for (int i = 0; i < loops; i++) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			keyvar = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn) {
				if (prevvar != null) BS_REF_DECREESE(prevvar);
				BS_REF_DECREESE(curvar);
				return data->BromScript->GetDefaultVarNull();
			}

			CString key;
			if (keyvar->Type == VariableType::Bool) key = keyvar->Value == null ? "False" : "True";
			else if (keyvar->Type == VariableType::Number) key = CString(*(double*)keyvar->Value);
			else if (keyvar->Type == VariableType::String) key = *(CString*)keyvar->Value;
			else {
				BS_THROW_ERROR(data->BromScript, CString::Format("Cannot index a table value using keytype %s", Converter::TypeToString(keyvar).str_szBuffer));
				if (prevvar != null) BS_REF_DECREESE(prevvar);
				BS_REF_DECREESE(curvar);
				return data->BromScript->GetDefaultVarNull();
			}

			if (key.Size() > 0) {
				if (curvar->Type == VariableType::Table) {
					Table* CurrentTable = (Table*)curvar->Value;

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = CurrentTable->Get(key);
					BS_REF_INCREESE(curvar);
				} else if (curvar->Type == VariableType::String) {
					if (keyvar->Type != VariableType::Number) {
						BS_THROW_ERROR(data->BromScript, CString::Format("Cannot index a string value using keytype %s", Converter::TypeToString(keyvar).str_szBuffer));
						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return data->BromScript->GetDefaultVarNull();
					}

					CString tmpstr = ((CString*)curvar->Value)->str_szBuffer[(int)keyvar->GetNumber()];

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = Converter::ToVariable(tmpstr);
					BS_REF_INCREESE(curvar);
				} else if (curvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)curvar->Value;

					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = data->BromScript->GC.RegisterVariable();
					BS_REF_INCREESE(curvar);

					bool found = false;
					for (int i = 0; i < udi->TypeData->Members.Count; i++) {
						if (udi->TypeData->Members[i]->Name == key) {
							if (udi->TypeData->Members[i]->Getter != null) {
								curvar = udi->TypeData->Members[i]->Getter(data->BromScript, (byte*)udi->Ptr + udi->TypeData->Members[i]->Offset);

								if (curvar == null) curvar = data->BromScript->GetDefaultVarNull();
								else data->BromScript->GC.RegisterVariable(curvar);
							} else {
								UserdataInstance* udi2 = new UserdataInstance();
								udi2->TypeData = udi->TypeData->Members[i];
								udi2->Ptr = (byte*)udi->Ptr + udi2->TypeData->Offset;

								curvar->Value = udi2;
								curvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
								curvar->IsCpp = true;
							}
							found = true;
							break;
						}
					}

					if (!found) {
						for (int i = 0; i < udi->TypeData->Functions.Count(); i++) {
							if (udi->TypeData->Functions.GetKeyByIndex(i) == key) {
								Function* func = new Function(data->BromScript);
								func->CppFunc = udi->TypeData->Functions.GetValueByIndex(i);
								func->IsCpp = true;
								func->Name = key;
								func->Filename = "C++";
								func->SetReferences(data->Function, 0);

								curvar->Type = VariableType::Function;
								curvar->Value = func;

								found = true;
								break;
							}
						}

						if (!found) {
							BS_THROW_ERROR(data->BromScript, CString::Format("Unknown userdata index '%s[\"%s\"]'", fullvarname.str_szBuffer, key.str_szBuffer));
							if (prevvar != null) BS_REF_DECREESE(prevvar);
							BS_REF_DECREESE(curvar);
							return data->BromScript->GetDefaultVarNull();
						}
					}
				} else {
					BS_THROW_ERROR(data->BromScript, CString::Format("Cannot index a non table value, %s", fullvarname.str_szBuffer));
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);
					return data->BromScript->GetDefaultVarNull();
				}

				fullvarname += CString::Format("[%s]", key.str_szBuffer);
			} else
				fullvarname += "[]";


			bool exec = data->Reader->ReadBool();
			if (exec) {
				if (curvar->Type != VariableType::Function && (prevvar->Type != VariableType::Userdata || ((UserdataInstance*)prevvar->Value)->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100])) {
					BS_THROW_ERROR(data->BromScript, CString::Format("Cannot execute a non function value, varname: %s", fullvarname.str_szBuffer));
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					BS_REF_DECREESE(curvar);
					return data->BromScript->GetDefaultVarNull();
				}

				int params = data->Reader->ReadInt();
				List<Variable*> vars;

				for (int i = 0; i < params; i++) {
					bool oldcall = data->FuncCallback;
					data->FuncCallback = true;
					vars.Add(data->Function->InternalRun(data));
					data->FuncCallback = oldcall;

					if (data->HasReturnValue || data->Function->ForceReturn) {
						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return data->Returning;
					}
				}

				if (keyvar->Type > VariableType::Userdata) {
					UserdataInstance* udi = (UserdataInstance*)prevvar->Value;
					ArgumentData* args = new ArgumentData();
					args->BromScript = data->BromScript;
					args->Caller = data->Function;
					args->SetVariableData(&vars);
					args->ThisObject = prevvar;

					Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100](data->BromScript, args);
					delete args;

					if (ret == null) ret = data->BromScript->GetDefaultVarNull();
					else data->BromScript->GC.RegisterVariable(ret);

					Variable* tmp = prevvar;
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = ret;
					BS_REF_INCREESE(curvar);
				} else {
					Function* func = (Function*)curvar->Value;
					Variable* tmp = prevvar;
					if (prevvar != null) BS_REF_DECREESE(prevvar);
					prevvar = curvar;
					curvar = func->Run(&vars, tmp);
					BS_REF_INCREESE(curvar);

					if (data->HasReturnValue || data->Function->ForceReturn) {
						if (prevvar != null) BS_REF_DECREESE(prevvar);
						BS_REF_DECREESE(curvar);
						return data->BromScript->GetDefaultVarNull();
					}
				}
			}
		}

		if (prevvar != null) BS_REF_DECREESE(prevvar);
		BS_REF_DECREESE(curvar);
		return curvar;
	}

	void Executer::SetVar(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();
		int localindex = islocal ? data->Reader->ReadInt() : 0;

		CString key = islocal ? data->Function->FixedLocalKeys[localindex] : data->Reader->ReadString();

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (data->HasReturnValue || data->Function->ForceReturn)
			return;

		if (islocal) {
			data->Function->SetVar(key, var, localindex);
		} else {
			data->Function->SetVar(key, var, true);
		}
	}

	void Executer::LSetVar(ExecuteData* data) {
		int localindex = data->Reader->ReadInt();

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (data->HasReturnValue || data->Function->ForceReturn)
			return;

		data->Function->SetVar(data->Function->FixedLocalKeys[localindex], var, localindex);
	}

	Variable* Executer::GetVar(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();

		if (islocal) {
			Variable* var = data->Function->FixedLocalVars[data->Reader->ReadInt()];
			return var != null ? var : data->BromScript->GetDefaultVarNull();
		} else {
			CString key = data->Reader->ReadString();
			Variable* var = data->Function->GetVar(key);

			return var;
		}
	}

	void Executer::If(ExecuteData* data) {
		byte skipnextifs = data->Reader->ReadByte();
		int ifstatmentsize = data->Reader->ReadInt();

		int oldpos = data->Reader->Pos;

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (data->HasReturnValue || data->Breaking || data->Continueing || data->Function->ForceReturn)
			return;

		// jump to end of condition code, just to be sure
		data->Reader->Pos = oldpos + ifstatmentsize;
		int codelen = data->Reader->ReadInt();

		if (!CheckIfVarTrue(var)) {
			data->Reader->Pos += codelen;
			return;
		}

		oldpos = data->Reader->Pos;

		oldcall = data->FuncCallback;
		data->FuncCallback = false;
		data->Function->InternalRun(data);
		data->FuncCallback = oldcall;
		if (data->HasReturnValue || data->Breaking || data->Continueing || data->Function->ForceReturn)
			return;

		// incase it returns in the middle, would be bad if we enter at a random point in the code :V
		data->Reader->Pos = oldpos + codelen;
		while (skipnextifs == 1) {
			data->Reader->Pos++;
			skipnextifs = data->Reader->ReadByte();

			// skip code data for elseifs
			data->Reader->Pos += data->Reader->ReadInt();
			data->Reader->Pos += data->Reader->ReadInt();
		}

		// only one else..... well lets hope, cus it's only possibru once!
		if (skipnextifs == 2) {
			// skip code
			data->Reader->ReadByte();
			data->Reader->Pos += data->Reader->ReadInt();
		}
	}

	void Executer::While(ExecuteData* data) {
		int checklen = data->Reader->ReadInt();
		int checkpos = data->Reader->Pos;

		while (true) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			Variable* var = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Breaking || data->Continueing || data->Function->ForceReturn)
				return;

			data->Reader->Pos = checkpos + checklen;

			int codelen = data->Reader->ReadInt();
			if (!CheckIfVarTrue(var)) {
				data->Reader->Pos = checkpos + checklen + codelen + 5; // +5, End byte and the int len for the bodycode, we should move this up.
				return;
			}

			oldcall = data->FuncCallback;
			data->FuncCallback = false;
			data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return;

			if (data->Breaking) {
				data->Breaking = false;
				data->Reader->Pos = checkpos + checklen + codelen + 5; // +5, End byte and the int len for the bodycode, we should move this up.
				return;
			}

			data->Continueing = false;
			data->Reader->Pos = checkpos;// goto if statment
		}
	}

	void Executer::For(ExecuteData* data) {
		int initlen = data->Reader->ReadInt();
		int iflen = data->Reader->ReadInt();
		int blocksize = data->Reader->ReadInt();
		int afterlen = data->Reader->ReadInt();

		bool oldcall = data->FuncCallback;
		data->FuncCallback = false;
		data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		int startpos = data->Reader->Pos;
		while (true) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			Variable* var = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			data->Reader->Pos = startpos + iflen;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return;

			if (!CheckIfVarTrue(var)) {
				data->Reader->Pos = startpos + iflen + blocksize + afterlen;
				return;
			}

			oldcall = data->FuncCallback;
			data->FuncCallback = false;
			data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			data->Reader->Pos = startpos + iflen + blocksize;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return;

			if (data->Breaking) {
				data->Breaking = false;
				data->Reader->Pos = startpos + iflen + blocksize + afterlen;
				return;
			}

			oldcall = data->FuncCallback;
			data->FuncCallback = false;
			data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			data->Reader->Pos = startpos + iflen + blocksize + afterlen;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return;

			data->Continueing = false;
			data->Reader->Pos = startpos; // back to the depths we go!
		}
	}

	void Executer::ForEach(ExecuteData* data) {
		int localindexkey = data->Reader->ReadInt();
		int localindexvalue = data->Reader->ReadInt();

		CString key = data->Function->FixedLocalKeys[localindexkey];
		CString value = data->Function->FixedLocalKeys[localindexvalue];

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		if (data->HasReturnValue || data->Function->ForceReturn)
			return;

		if (var->Type == VariableType::Null) {
			BS_THROW_ERROR(data->BromScript, CString::Format("Cannot foreach a null value! foreach (%s, %s, NULL)", key.str_szBuffer, value.str_szBuffer));
			return;
		}

		if (var->Type != VariableType::Table && var->Type < VariableType::Userdata) {
			BS_THROW_ERROR(data->BromScript, CString::Format("Cannot foreach a non Table value! foreach (%s, %s, NULL)", key.str_szBuffer, value.str_szBuffer));
			return;
		}

		int codelen = data->Reader->ReadInt();
		int codepos = data->Reader->Pos;

		if (var->Type == VariableType::Table) {
			Table* tbl = (Table*)var->Value;
			int curpos = tbl->GetNextIndex(0);
			while (curpos != -1) {
				Variable* valvar = tbl->GetByIndex(curpos);

				Variable* keyvar = data->BromScript->GC.RegisterVariable();
				keyvar->Type = VariableType::String;
				keyvar->Value = Converter::StringToPointer(tbl->GetKeyByIndex(curpos));

				data->Function->SetVar(key, keyvar, localindexkey);
				data->Function->SetVar(value, valvar, localindexvalue);

				bool oldcall = data->FuncCallback;
				data->FuncCallback = false;
				data->Function->InternalRun(data);
				data->FuncCallback = oldcall;

				if (data->HasReturnValue || data->Function->ForceReturn)
					return;

				if (data->Breaking) {
					data->Breaking = false;
					data->Reader->Pos = codepos + codelen;
					return;
				}

				data->Continueing = false;
				data->Reader->Pos = codepos; // back to the depths we go!

				curpos = tbl->GetNextIndex(curpos + 1);
			}
		} else {
			UserdataInstance* udi = (UserdataInstance*)var->Value;

			for (int i = 0; i < udi->TypeData->Members.Count + udi->TypeData->Functions.Count(); i++) {
				if (i < udi->TypeData->Members.Count) {
					Variable* valvar = data->BromScript->GC.RegisterVariable();
					Variable* keyvar = Converter::ToVariable(udi->TypeData->Members[i]->Name);

					if (udi->TypeData->Members[i]->Getter != null) {
						valvar = udi->TypeData->Members[i]->Getter(data->BromScript, (byte*)udi->Ptr + udi->TypeData->Members[i]->Offset);

						if (valvar == null) valvar = data->BromScript->GetDefaultVarNull();
						else data->BromScript->GC.RegisterVariable(valvar);
					} else {
						UserdataInstance* udi2 = new UserdataInstance();
						udi2->TypeData = udi->TypeData->Members[i];
						udi2->Ptr = (byte*)udi->Ptr + udi2->TypeData->Offset;

						valvar->Value = udi2;
						valvar->Type = (VariableType::Enum)udi2->TypeData->TypeID;
						valvar->IsCpp = true;
					}

					data->Function->SetVar(key, keyvar, false);
					data->Function->SetVar(value, valvar, false);
				} else {
					int gi = i - udi->TypeData->Members.Count;
					Variable* keyvar = Converter::ToVariable(udi->TypeData->Functions.GetKeyByIndex(gi));
					Variable* valvar = Converter::ToVariable(data->BromScript, udi->TypeData->Functions.GetKeyByIndex(gi), udi->TypeData->Functions.GetValueByIndex(gi));

					data->Function->SetVar(key, keyvar, false);
					data->Function->SetVar(value, valvar, false);
				}

				bool oldcall = data->FuncCallback;
				data->FuncCallback = false;
				data->Function->InternalRun(data);
				data->FuncCallback = oldcall;

				if (data->HasReturnValue || data->Function->ForceReturn)
					return;

				if (data->Breaking) {
					data->Breaking = false;
					data->Reader->Pos = codepos + codelen;
					return;
				}

				data->Continueing = false;
				data->Reader->Pos = codepos; // back to the depths we go!
			}
		}

		data->Reader->Pos = codepos + codelen;
	}

	void Executer::Rewind(ExecuteData* data) {
		if (data->GotoReset > -1) {
			data->Reader->Pos = data->GotoReset;
			data->GotoReset = -1;
		}
	}

	void Executer::Goto(ExecuteData* data) {
		short deptdiff = data->Reader->ReadShort();
		int offset = data->Reader->ReadInt();

		data->GotoReset = data->Reader->Pos;
		data->GotoDeptDiff = deptdiff;
		data->Reader->Pos = offset;

		if (deptdiff != 0) {
			data->HasReturnValue = true;
		}
	}

	void Executer::Loop(ExecuteData* data) {
		int localindex = data->Reader->ReadInt();

		CString key = data->Function->FixedLocalKeys[localindex];

		Variable* ivar = Converter::ToVariable(0);
		data->Function->SetVar(key, ivar, localindex);

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		int codelen = data->Reader->ReadInt();
		int codepos = data->Reader->Pos;

		if (data->HasReturnValue || data->Function->ForceReturn)
			return;

		if (var->Type == VariableType::Number) {
			int goal = (int)var->GetNumber();
			for (int i = 0; i < goal; i++) {
				bool oldcall = data->FuncCallback;

				data->FuncCallback = false;
				data->Function->InternalRun(data);
				data->FuncCallback = oldcall;

				if (data->HasReturnValue || data->Function->ForceReturn)
					return;

				if (data->Breaking) {
					data->Breaking = false;
					data->Reader->Pos = codepos + codelen;
					return;
				}

				data->Continueing = false;
				data->Reader->Pos = codepos; // back to the depths we go!

				ivar = data->Function->FixedLocalVars[localindex];
				if (var->Type != VariableType::Number) {
					BS_THROW_ERROR(data->BromScript, CString::Format("Variable '%s' is used as looper, but isn't a number anymore, instead it's a '%s'", key.str_szBuffer, Converter::TypeToString(var).str_szBuffer));
					return;
				}

				(*(double*)ivar->Value)++;
			}

			data->Reader->Pos = codepos + codelen;
			return;
		}

		if (var->Type == VariableType::Bool) {
			if (!var->GetBool()) {
				data->Reader->Pos = codepos + codelen;
				return;
			}

			int i = 0;

			while (true) {
				bool oldcall = data->FuncCallback;
				data->FuncCallback = false;
				data->Function->InternalRun(data);
				data->FuncCallback = oldcall;

				if (data->HasReturnValue || data->Function->ForceReturn)
					return;

				if (data->Breaking) {
					data->Breaking = false;
					data->Reader->Pos = codepos + codelen;
					return;
				}

				data->Continueing = false;
				data->Reader->Pos = codepos; // back to the depths we go!

				ivar = data->Function->FixedLocalVars[localindex];
				if (ivar->Type != VariableType::Number) {
					BS_THROW_ERROR(data->BromScript, CString::Format("Variable '%s' is used as looper, but isn't a number anymore, instead it's a '%s'", key.str_szBuffer, Converter::TypeToString(var).str_szBuffer));
					return;
				}
				(*(double*)ivar->Value)++;
			}
		}

		BS_THROW_ERROR(data->BromScript, CString::Format("Cannot do %s loops, only true for always, or the amount of loops", Converter::VariableToString(var).str_szBuffer));
	}

	void Executer::Else(ExecuteData* data) {
		// not realy needed anymore i guess. could be removed.
		int codelen = data->Reader->ReadInt();
		int codepos = data->Reader->Pos;

		bool oldcall = data->FuncCallback;
		data->FuncCallback = false;
		data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		data->Reader->Pos = codepos + codelen; // back to the depths we go!
	}

	bool ShouldDoArithmatic(int nf) {
		return nf >= Misc::ArithmaticFuncs::Add && nf <= Misc::ArithmaticFuncs::Modulo;
	}

	Variable* Executer::Merge(ExecuteData* data) {
		int spos = data->Reader->Pos;
		int mergesize = data->Reader->ReadInt();

		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* var = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		BS_REF_INCREESE(var);

		if (data->HasReturnValue || data->Function->ForceReturn)
			return data->Returning;

		if (var->Type >= VariableType::Userdata) {
			Variable* oldvar = var;
			var = Converter::MemberToVariable(data->BromScript, var);
			BS_REF_INCREESE(var);
			BS_REF_DECREESE(oldvar);
		}

		Misc::ExecFuncs nf = (Misc::ExecFuncs)data->Function->Code[data->Reader->Pos];

		if (nf == Misc::ExecFuncs::MergeEnd) {
			data->Reader->Pos = spos + mergesize - 1;
			BS_REF_DECREESE(var);
			return var;
		}

		while (ShouldDoArithmatic((int)nf)) {
			data->Reader->Pos++;

			// dont resume when left side is false at AND op
			if (nf == Misc::ArithmaticFuncs::And && !CheckIfVarTrue(var)) {
				Variable* ret = data->BromScript->GC.RegisterVariable();
				ret->Type = VariableType::Bool;
				ret->Value = null;
				data->Reader->Pos = spos + mergesize - 1;
				return ret;
			}

			// dont resume when left side is true at OR op
			if (nf == Misc::ArithmaticFuncs::Or && CheckIfVarTrue(var)) {
				Variable* ret = data->BromScript->GC.RegisterVariable();
				ret->Type = VariableType::Bool;
				ret->Value = (void*)1;
				data->Reader->Pos = spos + mergesize - 1;
				return ret;
			}

			oldcall = data->FuncCallback;
			data->FuncCallback = true;
			Variable* var2 = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			BS_REF_INCREESE(var2);

			if (data->HasReturnValue || data->Breaking || data->Continueing || data->Function->ForceReturn)
				return data->Returning;

			if (var2->Type >= VariableType::Userdata)
				var2 = Converter::MemberToVariable(data->BromScript, var2);


			Variable* oldvar = var;

			var = Executer::Arithmatic(data, var, var2, (Misc::ArithmaticFuncs)nf);
			BS_REF_INCREESE(var);

			BS_REF_DECREESE(var2);
			BS_REF_DECREESE(oldvar);

			nf = (Misc::ExecFuncs)data->Function->Code[data->Reader->Pos];
		}

		if (nf == Misc::ExecFuncs::MergeEnd) {
			data->Reader->Pos = spos + mergesize - 1;
			BS_REF_DECREESE(var);
			return var;
		}

		BS_REF_DECREESE(var);
		return var;
	}

	Variable* Executer::ExecuteFunction(ExecuteData* data) {
		bool islocal = data->Reader->ReadBool();
		Variable* var = null;
		CString name;

		if (islocal) {
			int localindex = data->Reader->ReadInt();
			name = data->Function->FixedLocalKeys[localindex];
			var = data->Function->FixedLocalVars[localindex];
		} else {
			name = data->Reader->ReadString();
			var = data->Function->GetVar(name);
		}
		int params = data->Reader->ReadInt();

		List<Variable*> vars;
		for (int i = 0; i < params; i++) {
			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			vars.Add(data->Function->InternalRun(data));
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return data->Returning;
		}


		if (var == null) {
			BS_THROW_ERROR(data->BromScript, "Trying to execute a null value!");
			return data->BromScript->GetDefaultVarNull();
		}

		if (var->Type != VariableType::Function) {
			if (var->Type > VariableType::Userdata) {
				UserdataInstance* udi = (UserdataInstance*)var->Value;
				if (udi->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100] != null) {
					ArgumentData args;
					args.BromScript = data->BromScript;
					args.Caller = data->Function;
					args.SetVariableData(&vars);
					args.ThisObject = var;

					Variable* ret = udi->TypeData->Operators[Misc::ArithmaticFuncs::Call - 100](data->BromScript, &args);

					if (ret == null) ret = data->BromScript->GetDefaultVarNull();
					else data->BromScript->GC.RegisterVariable(ret);

					return ret;
				}
			}

			BS_THROW_ERROR(data->BromScript, CString::Format("Trying to execute a non function! Varname: %s", name.str_szBuffer));
			return data->BromScript->GetDefaultVarNull();
		}

		Function* func = (Function*)var->Value;
		Variable* ret = func->Run(&vars);
		return ret;
	}

	void Executer::Delete(ExecuteData* data) {
		bool oldcall = data->FuncCallback;

		data->FuncCallback = true;
		data->Function->InternalRun(data)->EmptyValue();
		data->FuncCallback = oldcall;
	}

	Variable* Executer::Return(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		data->Returning = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		data->HasReturnValue = true;
		return data->Returning;
	}

	Variable* Executer::GetCount(ExecuteData* data) {
		bool oldcall = data->FuncCallback;
		data->FuncCallback = true;
		Variable* ret = data->Function->InternalRun(data);
		data->FuncCallback = oldcall;

		int len = 0;
		if (ret->Type == VariableType::String) {
			len = ((CString*)ret->Value)->Size();
		} else if (ret->Type == VariableType::Table) {
			len = ret->GetTable()->Count;
		} else {
			BS_THROW_ERROR(data->BromScript, CString::Format("Trying to get length of type '%s'", Converter::TypeToString(ret).str_szBuffer));
			return data->BromScript->GetDefaultVarNull();
		}

		Variable* var = data->BromScript->GC.RegisterVariable();
		var->Value = Converter::NumberToPointer(len);
		var->Type = VariableType::Number;
		return var;
	}

	Variable* Executer::GetBool(ExecuteData* data) {
		byte b = data->Reader->ReadByte();

		switch (b) {
			case 0: return data->BromScript->GetDefaultVarFalse();
			case 1: return data->BromScript->GetDefaultVarTrue();
			case 2: return data->BromScript->GetDefaultVarNull();
			default:
				BS_THROW_ERROR(data->BromScript, "Fatal runtime error, corrupt bytecode");
				break;
		}

		return null;
	}

	Variable* Executer::GetString(ExecuteData* data) {
		return data->Function->StringTableVars[data->Reader->ReadShort()];
	}

	Variable* Executer::GetNumber(ExecuteData* data) {
		Variable* var = data->BromScript->GC.RegisterVariable();
		var->Value = Converter::NumberToPointer(data->Reader->ReadDouble());
		var->Type = VariableType::Number;
		return var;
	}

	Variable* Executer::GetTable(ExecuteData* data) {
		int varcount = data->Reader->ReadInt();

		Variable* var = data->BromScript->GC.RegisterVariable();
		Table* tbl = new Table(data->BromScript);
		var->Value = tbl;
		var->Type = VariableType::Table;

		int curindex = 0;
		for (int i = 0; i < varcount; i++) {
			CString key;
			if (data->Reader->ReadBool()) {
				key = data->Reader->ReadString();
			} else {
				key = CString((double)curindex++);
			}

			bool oldcall = data->FuncCallback;
			data->FuncCallback = true;
			Variable* ret = data->Function->InternalRun(data);
			data->FuncCallback = oldcall;

			if (data->HasReturnValue || data->Function->ForceReturn)
				return data->Returning;

			tbl->Set(key, ret);
		}

		return var;
	}

	Variable* Executer::GetEnum(ExecuteData* data) {
		int varcount = data->Reader->ReadInt();

		Variable* var = data->BromScript->GC.RegisterVariable();
		Table* tbl = new Table(data->BromScript);
		var->Value = tbl;
		var->Type = VariableType::Table;

		int curindex = -1;
		for (int i = 0; i < varcount; i++) {
			CString key = data->Reader->ReadString();

			if (data->Reader->ReadBool()) {
				bool oldcall = data->FuncCallback;
				data->FuncCallback = true;
				Variable* ret = data->Function->InternalRun(data);
				data->FuncCallback = oldcall;

				if (ret == null || ret->Type != VariableType::Number) {
					BS_THROW_ERROR(data->BromScript, CString::Format("An enum may only contain numbers, and %s is not a number! Key: %s", Converter::TypeToString(ret).str_szBuffer, key.str_szBuffer));
					return null;
				}

				curindex = (int)ret->GetNumber();
			} else {
				curindex++;
			}

			tbl->Set(key, Converter::ToVariable(curindex));
		}

		return var;
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
			tbl->Set(CString((double)i), Converter::ToVariable(str.Substring(i, strlen - i < partlen ? strlen - i : partlen)));
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

		ret = data->BromScript->GC.RegisterVariable();

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
}
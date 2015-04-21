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

#include "../Objects/Function.h"

using namespace Scratch;

namespace BromScript {
	Function::Function(Instance* bs) : StringTableCount(0), StringTableVars(null), CodeReferenceFunc(null), CurrentSourceFileLine(-1), CodeOffset(0), FixedLocalsCount(0), CodeLength(0), Code(null), ForceReturn(false), FixedLocalVars(null), FixedLocalKeys(null), IsCpp(false), Parent(null), BromScript(bs), CppFunc(null), CurrentThisObject(null), StringTable(null) {
	}

	Function::~Function() {
		// TODO: stringtable and stringtablevars cleanup

		if (this->CodeReferenceFunc != null) {
			for (int i = 0; i < this->CodeReferenceFunc->CodeReferenceChilds.Count; i++) {
				if (this->CodeReferenceFunc->CodeReferenceChilds[i] == this) {
					this->CodeReferenceFunc->CodeReferenceChilds.RemoveAt(i);
					break;
				}
			}

			this->Code = null;
			this->StringTable = null;
			this->StringTableVars = null;
		} else {
			Function* new_hiveovermind = null;
			for (int i = 0; i < this->CodeReferenceChilds.Count; i++) {
				Function* child = this->CodeReferenceChilds[i];

				if (new_hiveovermind == null) {
					new_hiveovermind = child;
					new_hiveovermind->CodeReferenceFunc = null; // you are the captain now

					this->Code = null;
					this->StringTable = null;
					this->StringTableVars = null;
				} else {
					child->CodeReferenceFunc = new_hiveovermind;
					new_hiveovermind->CodeReferenceChilds.Add(child);
				}
			}
		}

		if (this->FixedLocalVars != null) {
			for (int i = 0; i < this->FixedLocalsCount; i++) {
				if (this->FixedLocalVars[i] != null) {
					BS_REF_DECREESE(this->FixedLocalVars[i]);
				}
			}

			delete[] this->FixedLocalVars;
		}

		if (this->FixedLocalKeys != null) delete[] this->FixedLocalKeys;

		if (this->Code != null) delete[] this->Code;
	}

	void Function::SetVar(CString key, Variable* var, bool global) {
		if (global) {
			BromScript::Function* func = this;
			while (func != null) {
				for (int i = 0; i < func->FixedLocalsCount; i++) {
					if (func->FixedLocalKeys[i] == key) {
						func->SetVar(key, var, i);
						return;
					}
				}

				func = func->Parent;
			}

			if (var == null || var->Type == VariableType::Null) {
				this->BromScript->Globals->Remove(key);
			} else {
				this->BromScript->Globals->Set(key, var);
			}

			return;
		}

		for (int i = 0; i < this->FixedLocalsCount; i++) {
			if (this->FixedLocalKeys[i] == key) {
				this->SetVar(key, var, i);
				return;
			}
		}
	}

	void Function::SetVar(CString key, Variable* var, int localindex) {
		Variable* oldvar = this->FixedLocalVars[localindex];
		if (oldvar != null) {
			BS_REF_DECREESE(oldvar);
		}

		if (var == null || var->Type == VariableType::Null) {
			this->FixedLocalVars[localindex] = null;
		} else {
			this->FixedLocalVars[localindex] = var;
			BS_REF_INCREESE(var);
		}
	}

	void Function::SetReferences(Function* func, int startoffset) {
		this->CodeReferenceFunc = func;

		this->Code = func->Code;
		this->CodeOffset = startoffset;

		this->StringTableCount = func->StringTableCount;
		this->StringTable = func->StringTable;
		this->StringTableVars = func->StringTableVars;

		func->CodeReferenceChilds.Add(this);
	}


	Variable* Function::GetVar(CString key) {
		if (key == "this" && this->CurrentThisObject != null)
			return this->CurrentThisObject;

		BromScript::Function* func = this;
		while (func != null) {
			for (int i = 0; i < func->FixedLocalsCount; i++) {
				if (func->FixedLocalKeys[i] == key) {
					return func->FixedLocalVars[i] == null ? this->BromScript->GetDefaultVarNull() : func->FixedLocalVars[i];
				}
			}

			func = func->Parent;
		}

		return this->BromScript->Globals->Get(key);
	}

	Variable* Function::Run() {
		ArgumentData args;
		args.BromScript = this->BromScript;
		args.Caller = this;
		args.Count = 0;

		Variable* ret = this->Run(&args);

		return ret;
	}

	Variable* Function::Run(Variable* thisobject) {
		Variable* oldthis = this->CurrentThisObject;
		this->CurrentThisObject = thisobject;

		ArgumentData args;
		args.BromScript = this->BromScript;
		args.Caller = this;

		Variable* ret = this->Run(&args);

		this->CurrentThisObject = oldthis;
		return ret;
	}

	Variable* Function::Run(List<Variable*>* vars) {
		ArgumentData args;
		args.BromScript = this->BromScript;
		args.Caller = this;
		args.SetVariableData(vars);

		Variable* ret = this->Run(&args);

		return ret;
	}

	Variable* Function::Run(List<Variable*>* vars, Variable* thisobject) {
		Variable* oldthis = this->CurrentThisObject;
		this->CurrentThisObject = thisobject;

		Variable* ret = this->Run(vars);

		this->CurrentThisObject = oldthis;
		return ret;
	}

	Variable* Function::Run(ArgumentData* args) {
		if (this->BromScript->GetCurrentStackSize() >= 257) {
			BS_THROW_ERROR(args, CString::Format("Stack error, cannot call '%s'", this->Name.str_szBuffer));
			return this->BromScript->GetDefaultVarNull();
		}

		if (args->Count > 255) {
			BS_THROW_ERROR(args, CString::Format("Param limit hit (255), cannot call '%s'", this->Name.str_szBuffer));
			return this->BromScript->GetDefaultVarNull();
		}

		this->ForceReturn = false;

		if (this->CurrentThisObject != null) this->BromScript->GC.RegisterVariable(this->CurrentThisObject);
		for (int i = 0; i < args->Count; i++) {
			this->BromScript->GC.RegisterVariable(args->GetVariable(i));
		}

		if (this->IsCpp) {
			args->ThisObject = this->CurrentThisObject;

			for (int i = 0; i < args->Count; i++) {
				BS_REF_INCREESE(args->GetVariable(i));
			}

			this->BromScript->GC.RunFrame();

			this->BromScript->EnterStack(this);
			Variable* ret = this->CppFunc(this->BromScript, args);
			this->BromScript->LeaveStack();

			if (ret == null) ret = this->BromScript->GetDefaultVarNull();
			else this->BromScript->GC.RegisterVariable(ret);

			for (int i = 0; i < args->Count; i++) {
				BS_REF_DECREESE(args->GetVariable(i));
			}

			return ret;
		}

		for (int i = 0; i < args->Count && i < this->Parameters.Count; i++) {
			this->SetVar(this->Parameters[i], args->GetVariable(i), i);
		}

		this->BromScript->GC.RunFrame();

		ExecuteData* data = new ExecuteData();
		data->BromScript = this->BromScript;
		data->Reader = new ByteReader();
		data->Reader->Pos = this->CodeOffset;
		data->Reader->Func = this;
		data->Function = this;

		int oldline = this->CurrentSourceFileLine;
		this->CurrentSourceFileLine = -1;

		this->BromScript->EnterStack(this);
		Variable* ret = this->InternalRun(data);
		this->BromScript->LeaveStack();

		this->CurrentSourceFileLine = oldline;

		delete data;
		return ret;
	}

	Variable* Function::Run(ArgumentData* args, Variable* thisobject) {
		Variable* oldthis = this->CurrentThisObject;
		this->CurrentThisObject = thisobject;

		Variable* ret = this->Run(args);

		this->CurrentThisObject = oldthis;
		return ret;
	}

	Variable* Function::InternalRun(ExecuteData* data) {
		while (true) {
			if (data->GotoDeptDiff < 0) {
				data->GotoDeptDiff++;

				if (data->GotoDeptDiff == 0) {
					data->HasReturnValue = false;
				}

				return null;
			}

			if (data->HasReturnValue || data->Breaking || data->Continueing || data->Function->ForceReturn)
				return data->Returning;

			if (data->BromScript->KillScriptThreaded)
				return data->BromScript->GetDefaultVarNull();

			Misc::ExecFuncs b = (Misc::ExecFuncs)data->Reader->ReadByte();

			switch (b) {
				case Misc::ExecFuncs::GetVar: return Executer::GetVar(data); break;
				case Misc::ExecFuncs::GetTblIndex:
					if (data->FuncCallback) return Executer::GetTableIndex(data);
					else Executer::GetTableIndex(data);
					break;

				case Misc::ExecFuncs::SetTblIndex:Executer::SetTableIndex(data); break;

				case Misc::ExecFuncs::SetVar: Executer::SetVar(data); break;
				case Misc::ExecFuncs::LSetVar: Executer::LSetVar(data); break;

				case Misc::ExecFuncs::PostIncrement: if (data->FuncCallback) return Executer::PostIncrement(data); else Executer::PostIncrement(data); break;
				case Misc::ExecFuncs::PreIncrement: if (data->FuncCallback) return Executer::PreIncrement(data); else Executer::PreIncrement(data); break;
				case Misc::ExecFuncs::PostDecrement: if (data->FuncCallback) return Executer::PostDecrement(data); else Executer::PostDecrement(data); break;
				case Misc::ExecFuncs::PreDecrement: if (data->FuncCallback) return Executer::PreDecrement(data); else Executer::PreDecrement(data); break;

				case Misc::ExecFuncs::If: Executer::If(data); break;
				case Misc::ExecFuncs::ElseIf: Executer::If(data); break; // same as if
				case Misc::ExecFuncs::Else: Executer::Else(data); break;

				case Misc::ExecFuncs::For: Executer::For(data); break;
				case Misc::ExecFuncs::ForEach: Executer::ForEach(data); break;
				case Misc::ExecFuncs::While: Executer::While(data); break;
				case Misc::ExecFuncs::Loop: Executer::Loop(data); break;
				case Misc::ExecFuncs::Goto: Executer::Goto(data); break;
				case Misc::ExecFuncs::Rewind: Executer::Rewind(data); break;
				case Misc::ExecFuncs::Delete: Executer::Delete(data); break;

				case Misc::ExecFuncs::New: return Executer::New(data); break;
				case Misc::ExecFuncs::Class: Executer::CreateClass(data); break;
				case Misc::ExecFuncs::Function: Executer::CreateFunction(data); break;
				case Misc::ExecFuncs::AnonFunction: return Executer::CreateAnonFunction(data); break;

				case Misc::ExecFuncs::ExecuteFunction:
					if (data->FuncCallback) return Executer::ExecuteFunction(data);
					else Executer::ExecuteFunction(data);
					break;

				case Misc::ExecFuncs::End:
					return data->BromScript->GetDefaultVarNull(); // lets give back null value.

				case Misc::ExecFuncs::MergeStart: return Executer::Merge(data);
				case Misc::ExecFuncs::Return: return Executer::Return(data);
				case Misc::ExecFuncs::Break: data->Breaking = true; return data->FuncCallback ? data->BromScript->GetDefaultVarNull() : null;
				case Misc::ExecFuncs::Continue: data->Continueing = true; return data->FuncCallback ? data->BromScript->GetDefaultVarNull() : null;

				case Misc::ExecFuncs::GetCount: return Executer::GetCount(data);
				case Misc::ExecFuncs::Bool: return Executer::GetBool(data);
				case Misc::ExecFuncs::String: return Executer::GetString(data);
				case Misc::ExecFuncs::Number: return Executer::GetNumber(data);
				case Misc::ExecFuncs::Table: return Executer::GetTable(data);
				case Misc::ExecFuncs::Enum: return Executer::GetEnum(data);

				case Misc::ExecFuncs::Skip: break;
				case Misc::ExecFuncs::StringTable: Executer::StringTable(data); break;
				case Misc::ExecFuncs::GlobalLocals: Executer::GlobalLocals(data); break;
				case Misc::ExecFuncs::CurrentLine:
					this->CurrentSourceFileLine = data->Reader->ReadInt();
					this->BromScript->Debug->Update();
					break;

				default:
					BS_THROW_ERROR(data->BromScript, CString::Format("Invalid bytecode detected! (%d)", b));

					data->HasReturnValue = true;
					data->Returning = data->BromScript->GetDefaultVarNull();
					return data->Returning;
			}
		}
	}
}
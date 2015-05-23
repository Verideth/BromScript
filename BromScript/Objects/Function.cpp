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

#include "../Managers/Instance.h"
#include "Function.h"

using namespace Scratch;

namespace BromScript {
	Function::Function(Instance* bs) : StringTableCount(0), StringTableVars(null), FixedLocalTypes(null), FixedLocalIsRef(null), CodeReferenceFunc(null), CurrentSourceFileLine(-1), CodeOffset(0), FixedLocalsCount(0), CodeLength(0), Code(null), ForceReturn(false), FixedLocalVars(null), FixedLocalKeys(null), IsCpp(false), Parent(null), BromScript(bs), CppFunc(null), CurrentThisObject(null), StringTable(null) {
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
		if (this->FixedLocalIsRef != null) delete[] this->FixedLocalIsRef;
		if (this->FixedLocalTypes != null) delete[] this->FixedLocalTypes;
		if (this->Code != null) delete[] this->Code;
	}

	void Function::SetVar(CString key, Variable* var, bool global) {
		if (global) {
			BromScript::Function* func = this;
			while (func != null) {
				for (int i = 0; i < func->FixedLocalsCount; i++) {
					if (func->FixedLocalKeys[i] == key) {
						func->SetVar(var, i);
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
				this->SetVar(var, i);
				return;
			}
		}
	}

	void Function::SetVar(Variable* var, int localindex) {
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
		while (func->CodeReferenceFunc != null) {
			func = func->CodeReferenceFunc;
		}

		this->CodeReferenceFunc = func;

		this->Code = func->Code;
		this->CodeOffset = startoffset;

		this->StringTableCount = func->StringTableCount;
		this->StringTable = func->StringTable;
		this->StringTableVars = func->StringTableVars;

		func->CodeReferenceChilds.Add(this);
	}


	Variable* Function::GetVar(CString& key) {
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
		if (this->BromScript->GetCurrentStackSize() >= 80085) {
			BS_THROW_ERROR(args, CString::Format("Stack error, cannot call '%s'", this->Name.str_szBuffer));
			return this->BromScript->GetDefaultVarNull();
		}

		this->ForceReturn = false;

		if (this->CurrentThisObject != null) this->BromScript->GC.RegisterVariable(this->CurrentThisObject);
		for (int i = 0; i < args->Count; i++) {
			this->BromScript->GC.RegisterVariable(args->GetVariable(i));
		}

		if (this->IsCpp) {
			args->SetThisObject(this->CurrentThisObject);

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
			Variable* var = args->GetVariable(i);

			if (this->FixedLocalTypes[i] != -1) {
				if (var->Type != this->FixedLocalTypes[i]) {
					BS_THROW_ERROR(this->BromScript, CString::Format("Expected type %s at argument %d, but got %s", (const char*)Converter::TypeToString(this->BromScript, (VariableType::Enum)this->FixedLocalTypes[i]), i + 1, (const char*)Converter::TypeToString(this->BromScript, var->Type)).str_szBuffer);
					return this->BromScript->GetDefaultVarNull();
				}
			}

			if (this->FixedLocalIsRef != null && !this->FixedLocalIsRef[i] && (var->Type == VariableType::Number || var->Type == VariableType::String)) {
				Variable* nvar = this->BromScript->GC.GetPooledVariable();
				nvar->Type = var->Type;

				if (var->Type == VariableType::Number) {
					nvar->Value = this->BromScript->GC.NumberPool.GetNext();
					*(double*)nvar->Value = *(double*)var->Value;
				} else if (var->Type == VariableType::String){
					nvar->Value = new CString((CString*)var->Value);
				}

				var = nvar;
			}

			this->SetVar(var, i);
		}

		this->BromScript->GC.RunFrame();

		ExecuteData* data = new ExecuteData();
		data->BromScript = this->BromScript;
		data->Reader = new ByteReader();
		data->Reader->Code = this->Code;
		data->Reader->StringTable = this->StringTable;
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
			if (data->Function->ForceReturn || data->BromScript->KillScriptThreaded)
				return data->BromScript->GetDefaultVarNull();

			Operators b = (Operators)data->Reader->ReadByte();
			switch (b) {
				case Operators::Skip: break;
				case Operators::Pop: Executer::Pop(data); break;
				case Operators::Duplicate: Executer::Duplicate(data); break;
					
				case Operators::AddIndex: Executer::AddIndex(data); break;
				case Operators::GetIndex: Executer::GetIndex(data); break;
				case Operators::SetIndex: Executer::SetIndex(data); break;
				case Operators::Set: Executer::Set(data); break;
				case Operators::SetL: Executer::SetL(data); break;
				case Operators::PostIncrement: Executer::PostIncrement(data); break;
				case Operators::PreIncrement: Executer::PreIncrement(data); break;

				case Operators::StackNull: Executer::StackNull(data); break;
				case Operators::StackBool: Executer::StackBool(data); break;
				case Operators::StackNumber: Executer::StackNumber(data); break;
				case Operators::StackTable: Executer::StackTable(data); break;
				case Operators::StackString: Executer::StackString(data); break;
				case Operators::StackFunction: Executer::StackFunction(data); break;
				case Operators::Jump: Executer::Jump(data); break;
				case Operators::JumpNT: Executer::JumpNT(data); break;
				case Operators::Get: Executer::Get(data); break;
				case Operators::GetL: Executer::GetL(data); break;
				case Operators::StringTable: Executer::StringTable(data); break;
				case Operators::GlobalLocals: Executer::GlobalLocals(data); break;
				case Operators::CurrentLine: Executer::CurrentLine(data); break;

				case Operators::Call: Executer::Call(data); break;
				case Operators::CallThis: Executer::CallThis(data); break;
				case Operators::GetCount: Executer::GetCount(data); break;

				case Operators::New: Executer::New(data); break;
				case Operators::Delete: Executer::Delete(data); break;


				// these functions abort the execution, so put them here instead of in the Executer
				case Operators::Return:{
					if (data->Reader->ReadBool()) {
						return data->PopStack();
					} else {
						return data->BromScript->GetDefaultVarNull();
					}
				}

				case Operators::EndScope: {
					return data->BromScript->GetDefaultVarNull();
				}

				// our special case
				default:
					if (b > Operators::Arithmetic_START && b < Operators::Arithmetic_END) {
						Variable* right = data->PopStack();
						Variable* left = data->PopStack();

						int op = (int)b + (Misc::ArithmaticFuncs::Add - (int)Operators::Arithmetic_START - 1);

						BS_REF_INCREESE(left);
						BS_REF_INCREESE(right);
						data->PushStack(data->BromScript->GC.RegisterVariable(Executer::Arithmatic(data, left, right, (Misc::ArithmaticFuncs)op)));
						BS_REF_DECREESE(right);
						BS_REF_DECREESE(left);

						break;
					}

					BS_THROW_ERROR(data->BromScript, CString::Format("Invalid bytecode detected! (%d)", b));
					return data->BromScript->GetDefaultVarNull();
			}
		}
	}
}

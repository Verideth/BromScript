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

#include "BSUDArray.h"
#include "../Objects/ExecuteData.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"

#include "../SIF.h"

namespace BromScript {
	namespace Userdatas {
		namespace Array {
			CArray::CArray() {

			}

			CArray::~CArray() {

			}

			void CArray::Add(Variable* var) {
				var->IncreeseRefCount();
				this->Vars.Add(var);
			}

			void CArray::Insert(int index, Variable* var) {
				var->IncreeseRefCount();
				this->Vars.Insert(index, var);
			}

			void CArray::Remove(int index, int count) {
				this->Vars.RemoveAt(index)->DecreeseRefCount();
			}

			BS_FUNCTION(Add) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;

				CArray* arr = (CArray*)args->GetThisObjectData();

				for (int i = 0; i < args->Count; i++) {
					arr->Add(args->GetVariable(i));
				}

				return null;
			}

			BS_FUNCTION(Insert) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				int pos = (int)args->GetNumber(0);
				CArray* arr = (CArray*)args->GetThisObjectData();

				for (int i = 0; i < args->Count; i++) {
					arr->Insert(pos, args->GetVariable(i));
				}

				return null;
			}

			BS_FUNCTION(Remove) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				int pos = (int)args->GetNumber(0);
				int c = args->CheckType(1, VariableType::Number) ? (int)args->GetNumber(1) : 1;

				CArray* arr = (CArray*)args->GetThisObjectData();

				while (c > 0) {
					arr->Remove(pos);
					c--;
				}

				return null;
			}

			BS_FUNCTION(GET) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;

				if (!args->CheckType(0, VariableType::Number) && !args->CheckType(0, VariableType::String)) {
					args->CheckType(0, VariableType::Number, true);
					return null;
				}


				CArray* arr = (CArray*)args->GetThisObjectData();
				int pos = 0;

				if (args->CheckType(0, VariableType::Number)) {
					pos = (int)args->GetNumber(0);
				} else {
					pos = atoi(args->GetString(0));
				}

				if (pos >= arr->Vars.Count) { args->Error("Index out of bounds"); return nullptr; }
				return arr->Vars[pos];
			}

			BS_FUNCTION(SET) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				int c = args->CheckType(1, VariableType::Number) ? (int)args->GetNumber(1) : 1;
				int pos = (int)args->GetNumber(0);
				Variable* var = args->GetVariable(1);

				CArray* arr = (CArray*)args->GetThisObjectData();
				if (pos >= arr->Vars.Count) { args->Error("Index out of bounds"); return nullptr; }

				arr->Vars[pos]->DecreeseRefCount();
				arr->Vars[pos] = var;
				
				return null;
			}

			BS_FUNCTION(COUNT) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;
				CArray* arr = (CArray*)args->GetThisObjectData();

				return bsi->ToVariable(arr->Vars.Count);
			}

			BS_FUNCTION(GetNextIndex) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_ARRAY_TYPE, true)) return null;
				CArray* arr = (CArray*)args->GetThisObjectData();

				if (args->GetVariable(0)->Type == VariableType::Null) return bsi->ToVariable(0);

				int next = (int)args->GetVariable(0)->GetNumber() + 1;
				if (next >= arr->Vars.Count) return bsi->GetDefaultVarNull();

				return bsi->ToVariable(next);
			}

			BS_FUNCTION_CTOR(CTOR) {
				return new CArray();
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (CArray*)data;
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("Array", BROMSCRIPT_USERDATA_ARRAY_TYPE, sizeof(CArray), CTOR, DTOR);

				vd->RegisterFunction("GetNextIndex",GetNextIndex);
				vd->RegisterFunction("Add", Add);
				vd->RegisterFunction("Remove", Remove);
				vd->RegisterFunction("Insert", Insert);
				vd->RegisterOperator(Operators::ArithmeticGetIndex, GET);
				vd->RegisterOperator(Operators::ArithmeticSetIndex, SET);
				vd->RegisterOperator(Operators::ArithmeticGetCount, COUNT);
			}
		}
	}
}
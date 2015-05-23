#include "../Objects/UserdataInstance.h"
#include "../Objects/Variable.h"
#include "../Managers/Instance.h"

namespace BromScript{
	UserdataInstance::UserdataInstance(void) :CallDTor(false) {
	}

	UserdataInstance::~UserdataInstance(void) {
		if (!this->CallDTor || this->TypeData == null || this->TypeData->Dtor == null || this->Ptr == null)
			return;

		this->TypeData->Dtor(this->TypeData->BromScript, this->Ptr);
		this->Ptr = null;
	}

	void UserdataInstance::SetIndex(Variable* selfobj, Variable* keyvar, Variable* value) {
		Variable* member = this->GetIndex(selfobj, keyvar);
		if (this->TypeData->BromScript->GetCurrentFunction()->ForceReturn) return;

		if (member->Type != VariableType::Null) {
			if (Converter::SetMember(this->TypeData->BromScript, member, value, keyvar->ToString())) {
				return;
			}
		}

		int index = BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticSetIndex);
		if (this->TypeData->OperatorsOverrides[index] != null) {
			ArgumentData args;
			args.SetThisObject(selfobj);
			args.AddVariable(keyvar);
			args.AddVariable(value);

			Variable* ret = this->TypeData->OperatorsOverrides[index](this->TypeData->BromScript, &args);
			if (ret != null) {
				this->TypeData->BromScript->GC.RegisterVariable(ret);
			}

			return;
		}

		BS_THROW_ERROR(this->TypeData->BromScript, Scratch::CString::Format("No index called '%s' to set in %s type", keyvar->ToString(this->TypeData->BromScript).str_szBuffer, Converter::TypeToString(this->TypeData->BromScript, (VariableType::Enum)this->TypeData->TypeID).str_szBuffer));
	}

	Variable* UserdataInstance::GetMember(const Scratch::CString& key) {
		for (int i = 0; i < this->TypeData->Members.Count; i++) {
			Userdata* ud = this->TypeData->Members[i];
			if (ud->Name == key) {
				Variable* ret = null;

				if (ud->Getter != null) {
					ret = ud->Getter(this->TypeData->BromScript, (byte*)this->Ptr + ud->Offset);

					if (ret == null) ret = this->TypeData->BromScript->GetDefaultVarNull();
					else this->TypeData->BromScript->GC.RegisterVariable(ret);
				} else {
					UserdataInstance* udi2 = new UserdataInstance();
					udi2->TypeData = ud;
					udi2->Ptr = (byte*)this->Ptr + udi2->TypeData->Offset;

					ret = this->TypeData->BromScript->GC.GetPooledVariable();
					ret->Value = udi2;
					ret->Type = (VariableType::Enum)udi2->TypeData->TypeID;
					ret->IsCpp = true;
				}

				return ret;
			}
		}

		return null;
	}

	Variable* UserdataInstance::GetMethod(const Scratch::CString& key) {
		for (int i = 0; i < this->TypeData->Functions.Count(); i++) {
			if (this->TypeData->Functions.GetKeyByIndex(i) == key) {
				Function* func = new Function(this->TypeData->BromScript);
				func->CppFunc = this->TypeData->Functions.GetValueByIndex(i);
				func->IsCpp = true;
				func->Name = key;
				func->Filename = "C++";
				func->SetReferences(this->TypeData->BromScript->GetCurrentFunction(), 0);

				Variable* ret = this->TypeData->BromScript->GC.GetPooledVariable();
				ret->Type = VariableType::Function;
				ret->Value = func;

				return ret;
			}
		}

		return null;
	}

	Variable* UserdataInstance::GetIndex(Variable* selfobj, Variable* keyvar) {
		Variable* ret = null;
		Scratch::CString key = keyvar->ToString(this->TypeData->BromScript);

		Variable* member = this->GetMember(key);
		if (member != null) return member;

		Variable* method = this->GetMethod(key);
		if (method != null) return method;

		int index = BS_ARITHMATICOP_TOFUNCINDEX(Operators::ArithmeticGetIndex);
		if (this->TypeData->OperatorsOverrides[index] != null) {
			ArgumentData args;
			args.SetThisObject(selfobj);
			args.AddVariable(keyvar);

			ret = this->TypeData->OperatorsOverrides[index](this->TypeData->BromScript, &args);
			if (ret == null) ret = this->TypeData->BromScript->GetDefaultVarNull();
			else this->TypeData->BromScript->GC.RegisterVariable(ret);

			return ret;
		}

		BS_THROW_ERROR(this->TypeData->BromScript, Scratch::CString::Format("No index called '%s' to get in %s type", key.str_szBuffer, Converter::TypeToString(this->TypeData->BromScript, (VariableType::Enum)this->TypeData->TypeID).str_szBuffer));
		return this->TypeData->BromScript->GetDefaultVarNull();
	}
}
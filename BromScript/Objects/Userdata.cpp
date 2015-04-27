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

#ifndef BROMSCRIPT_USERDATA_CPP_INCLUDED
#define BROMSCRIPT_USERDATA_CPP_INCLUDED

#include "../Objects/Userdata.h"
#include "../Managers/Instance.h"

using namespace Scratch;

namespace BromScript{
	Userdata::Userdata() :CallDTor(false), Name(""), Offset(0), TypeID(0), TypeSize(0), Getter(null), Setter(null) {
		memset(this->OperatorsOverrides, 0, sizeof(this->OperatorsOverrides));
	}

	Userdata::~Userdata(void) {
		while (this->Members.Count > 0) {
			delete this->Members.RemoveAt(0);
		}
	}

	Userdata* Userdata::Copy() {
		Userdata* ud = new Userdata();

		ud->Name = this->Name;
		ud->Offset = this->Offset;
		ud->TypeID = this->TypeID;
		ud->BromScript = this->BromScript;
		ud->TypeSize = this->TypeSize;
		ud->Getter = this->Getter;
		ud->Setter = this->Setter;

		ud->Ctor = this->Ctor;
		ud->Dtor = this->Dtor;

		for (int i = 0; i < this->Members.Count; i++)
			ud->Members.Add(this->Members[i]->Copy());

		for (int i = 0; i < this->Functions.Count(); i++)
			ud->Functions.Add(this->Functions.GetKeyByIndex(i), this->Functions.GetValueByIndex(i));

		for (int i = 0; i < (int)Operators::Arithmetic_END - (int)Operators::Arithmetic_START - 1; i++)
			ud->OperatorsOverrides[i] = this->OperatorsOverrides[i];

		return ud;
	}

	void Userdata::RegisterOperator(Operators opcode, BSFunction func) {
		this->OperatorsOverrides[BS_ARITHMATICOP_TOFUNCINDEX(opcode)] = func;
	}

	void Userdata::RegisterMember(CString name, int offset, int type) {
		Userdata* md = null;
		for (int i = 0; i < this->BromScript->RegisteredUserdataTypes.Count; i++) {
			if (this->BromScript->RegisteredUserdataTypes[i]->TypeID == type) {
				md = this->BromScript->RegisteredUserdataTypes[i]->Copy();
				break;
			}
		}

		if (md == null) {
			if (type > MemberType::Bool)
				throw "Unknown type";

			md = new Userdata();
		}

		md->BromScript = this->BromScript;
		md->Offset = offset;
		md->TypeID = type;
		md->Name = name;

		this->Members.Add(md);
	}

	void Userdata::RegisterMember(CString name, int offset, int type, BSSetFunction setter, BSGetFunction getter) {
		Userdata* md = null;
		for (int i = 0; i < this->BromScript->RegisteredUserdataTypes.Count; i++) {
			if (this->BromScript->RegisteredUserdataTypes[i]->TypeID == type) {
				md = this->BromScript->RegisteredUserdataTypes[i]->Copy();
				break;
			}
		}

		if (md == null) {
			if (type > MemberType::Bool)
				throw "Unknown MemberType";

			md = new Userdata();
		}

		md->BromScript = this->BromScript;
		md->Offset = offset;
		md->TypeID = type;
		md->Name = name;
		md->Getter = getter;
		md->Setter = setter;

		this->Members.Add(md);
	}

	void Userdata::RegisterFunction(CString name, BSFunction function) {
		this->Functions.Add(name, function);
	}
}

#endif
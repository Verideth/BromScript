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

#include "../Objects/Table.h"

using namespace Scratch;

namespace BromScript{
	Table::Table(Instance* bs) :Count(0), BufferSize(0), BromScript(bs), Keys(null), Values(null) {
		this->AllocateMoreSpace();
	}

	Table::~Table(void) {
		this->Clear();
		free(this->Keys);
		free(this->Values);
	}

	void Table::Clear() {
		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null) {
				delete this->Keys[i];
			}
		}

		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Values[i] != null) {
				BS_REF_DECREESE(this->Values[i]);
			}
		}

		free(this->Keys);
		free(this->Values);

		this->Count = 0;
		this->Keys = null;
		this->Values = null;
		this->BufferSize = 0;
		//this->AllocateMoreSpace();
	}

	CString Table::GetKeyByIndex(int i) {
		if (i >= this->BufferSize || this->Keys[i] == null)
			return "";

		return *this->Keys[i];
	}

	int Table::GetNextIndex(int spos) {
		for (int i = spos; i < this->BufferSize; i++) {
			if (this->Keys[i] != null) {
				return i;
			}
		}

		return -1;
	}

	void Table::Set(const CString& key, Variable* var) {
		this->BromScript->GC.RegisterVariable(var);
		BS_REF_INCREESE(var);

		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null && *this->Keys[i] == key) {
				if (this->Values[i]->ReadOnly) {
					BS_THROW_ERROR(this->BromScript, CString::Format("Cannot override '%s', it's read only!", key.str_szBuffer));
					return;
				}

				BS_REF_DECREESE(this->Values[i]);
				this->Values[i] = var;
				return;
			}
		}

		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Values[i] == null) {
				this->Keys[i] = new CString(key);
				this->Values[i] = var;

				this->Count++;
				return;
			}
		}

		this->Count++;
		this->AllocateMoreSpace();

		this->Keys[this->BufferSize - 128] = new CString(key);
		this->Values[this->BufferSize - 128] = var;
	}

	void Table::SetIndex(int i, Variable* var) {
		this->BromScript->GC.RegisterVariable(var);
		BS_REF_INCREESE(var);

		Variable* oldvar = this->Values[i];
		if (oldvar != null) {
			if (oldvar->ReadOnly) {
				BS_THROW_ERROR(this->BromScript, CString::Format("Cannot override '%d', it's read only!", (*this->Keys[i]).str_szBuffer));
				return;
			}

			BS_REF_DECREESE(oldvar);
		}

		this->Values[i] = var;
	}

	void Table::Remove(const CString& key) {
		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null && *this->Keys[i] == key) {
				BS_REF_DECREESE(this->Values[i]);
				this->Values[i] = null;

				delete this->Keys[i];
				this->Keys[i] = null;

				this->Count--;

				if (this->BufferSize - this->Count > 128)
					this->Defrag();

				return;
			}
		}
	}

	bool Table::HasKey(const CString& key) {
		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null && *this->Keys[i] == key) {
				return true;
			}
		}

		return false;
	}

	int Table::GetHasKeyIndex(const CString& key) {
		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null && *this->Keys[i] == key) {
				return i;
			}
		}

		return -1;
	}

	Variable* Table::Get(const CString& key) {
		for (int i = 0; i < this->BufferSize; i++) {
			if (this->Keys[i] != null && *this->Keys[i] == key) {
				return this->Values[i];
			}
		}

		return this->BromScript->GetDefaultVarNull();
	}

	void Table::Defrag() {
		if (true) // it crashes at free below, no idea why, lets disable it for now.
			return;

		int nsize = (int)(this->Count > 0 ? ceil(this->Count / 128.0) * 128 : 128);

		Variable** newbuffv = (Variable**)malloc(sizeof(Variable*) * nsize);
		CString** newbuffk = (CString**)malloc(sizeof(CString*) * nsize);

		int curp = this->GetNextIndex(0);
		int i = 0;
		while (curp != -1) {
			newbuffv[i] = this->Values[curp];
			newbuffk[i] = this->Keys[curp];

			i++;
			curp = this->GetNextIndex(curp + 1);
		}

		while (i < this->BufferSize) {
			newbuffv[i] = null;
			newbuffk[i] = null;

			i++;
		}

		free(this->Values);
		free(this->Keys);

		this->Values = newbuffv;
		this->Keys = newbuffk;
	}

	void Table::AllocateMoreSpace() {
		this->BufferSize += 128;
		Variable** newbuffv = (Variable**)malloc(sizeof(Variable*) * this->BufferSize);
		CString** newbuffk = (CString**)malloc(sizeof(CString*) * this->BufferSize);

		for (int i = 0; i < this->BufferSize; i++) {
			newbuffv[i] = null;
			newbuffk[i] = null;
		}

		if (this->Values != null) {
			memcpy(newbuffv, this->Values, sizeof(Variable*) * (this->BufferSize - 128));
			free(this->Values);
		}

		if (this->Keys != null) {
			memcpy(newbuffk, this->Keys, sizeof(CString*) * (this->BufferSize - 128));
			free(this->Keys);
		}

		this->Values = newbuffv;
		this->Keys = newbuffk;
	}
}
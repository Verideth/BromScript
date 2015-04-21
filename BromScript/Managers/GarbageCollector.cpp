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

#include "../Managers/GarbageCollector.h"
#include "../Objects/ArgumentData.h"

using namespace Scratch;

namespace BromScript{
	GarbageCollector::GarbageCollector() {
		this->Buffer = null;
		this->BufferSize = 0;
		this->NullStart = 0;
		this->FrameSkip = 0;
		this->AllocateMoreSpace();
	}

	GarbageCollector::~GarbageCollector() {
		bool isdone = false;
		while (!isdone) {
			isdone = true;

			List<Variable*> vars;
			for (int i = 0; i < this->BufferSize; i++) {
				Variable* var = this->Buffer[i];

				if (var != null) {
					isdone = false;
					vars.Add(var);
					if (var->GetRefCount() == 0) {
						for (int i2 = 0; i2 < this->BufferSize; i2++) {
							if (var == this->Buffer[i2]) {
								this->Buffer[i2] = null;
							}
						}

						delete var;
					} else if (var->Type == VariableType::Table) {
						var->GetTable()->Clear();
					} else if (var->Type == VariableType::Function) {
						Function* func = var->GetFunction();

						for (int i = 0; i < func->FixedLocalsCount; i++) {
							if (func->FixedLocalVars[i] != null) {
								BS_REF_DECREESE(func->FixedLocalVars[i]);
								func->FixedLocalVars[i] = null;
							}
						}
					}
				}
			}
			
			continue;
		}

		free(this->Buffer);
	}

	BS_FUNCTION(GarbageCollector::RunWrapper) {
		return Converter::ToVariable(args->BromScript->GC.Run());
	}

	void GarbageCollector::RunFrame() {
		this->FrameSkip++;
		if (this->FrameSkip < 1024) return;

		this->FrameSkip = 0;

		for (int i = 0; i < this->BufferSize; i++) {
			Variable* var = this->Buffer[i];

			if (var != null && var->GetRefCount() == 0) {
				delete var;
				this->Buffer[i] = null;

				if (i < this->NullStart) this->NullStart = i;
			}
		}
	}

	int GarbageCollector::Run() {
		int cleaned = 0;
		this->FrameSkip = 0;

		int curc = 1337;
		while (curc > 0) {
			curc = 0;
			for (int i = 0; i < this->BufferSize; i++) {
				Variable* var = this->Buffer[i];
				if (var != null && var->GetRefCount() == 0) {
					curc++;

					delete var;
					this->Buffer[i] = null;

					if (i < this->NullStart) this->NullStart = i;
				}
			}

			cleaned += curc;
		}

		return cleaned * sizeof(Variable);
	}

	Variable* GarbageCollector::RegisterVariable() {
		Variable* var = new Variable();
		var->RegisteredInGC = true;

		this->Buffer[this->NullStart] = var;

		bool foundnext = false;
		for (int i = this->NullStart + 1; i < this->BufferSize; i++) {
			if (this->Buffer[i] == null) {
				this->NullStart = i;
				foundnext = true;
				break;
			}
		}

		if (!foundnext) {
			this->AllocateMoreSpace();
			this->NullStart++;
		}

		this->FrameSkip++;
		return var;
	}

	Variable* GarbageCollector::RegisterVariable(Variable* var) {
		if (var->RegisteredInGC)
			return var;

		var->RegisteredInGC = true;
		this->Buffer[this->NullStart] = var;

		bool foundnext = false;
		for (int i = this->NullStart + 1; i < this->BufferSize; i++) {
			if (this->Buffer[i] == null) {
				this->NullStart = i;
				foundnext = true;
				break;
			}
		}

		if (!foundnext) {
			this->AllocateMoreSpace();
			this->NullStart++;
		}

		this->FrameSkip++;
		return var;
	}

	void GarbageCollector::AllocateMoreSpace() {
		this->BufferSize += 1024;
		Variable** newbuff = (Variable**)malloc(sizeof(Variable*) * this->BufferSize);

		if (this->Buffer != null) {
			memcpy(newbuff, this->Buffer, sizeof(Variable*) * (this->BufferSize - 1024));
			free(this->Buffer);
		}

		memset(newbuff + (this->BufferSize - 1024), 0, sizeof(Variable*) * 1024);

		this->Buffer = newbuff;
	}
}
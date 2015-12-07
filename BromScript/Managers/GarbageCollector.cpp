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
		this->Pools = nullptr;
		this->Buffer = nullptr;

		this->BufferSize = 0;
		this->PoolsSize = 0;
		this->CurrentPool = -1;
		this->FrameSkip = 0;

		this->NullStart = 0;

		this->BuffHighest = 0;

		this->AllocateMoreSpace();
		this->AllocateMorePools();
	}

	GarbageCollector::~GarbageCollector() {
		if (this->Buffer == nullptr) return;
		this->SelfDestruct();
	}

	void GarbageCollector::SelfDestruct() {
		bool isdone = false;
		while (!isdone) {
			isdone = true;

			List<Variable*> vars;
			for (int i = 0; i < this->BufferSize; i++) {
				Variable* var = this->Buffer[i];

				if (var != nullptr) {
					isdone = false;
					vars.Add(var);
					if (var->GetRefCount() == 0) {
						this->Buffer[i] = nullptr;

						if (var->PoolRef.ID == -1) {
							delete var;
						} else {
							Pool* p = this->Pools[var->PoolRef.ID];
							var->EmptyValue();
							var->RegisteredInGC = false;
							p->Free(var);
						}
					} else if (var->Type == VariableType::Table) {
						var->GetTable()->Clear();
					} else if (var->Type == VariableType::Function) {
						Function* func = var->GetFunction();

						for (int i = 0; i < func->FixedLocalsCount; i++) {
							if (func->FixedLocalVars[i] != nullptr) {
								BS_REF_DECREESE(func->FixedLocalVars[i]);
								func->FixedLocalVars[i] = nullptr;
							}
						}
					}
				}
			}

			continue;
		}

		for (int i = 0; i < this->PoolsSize; i++) {
			delete this->Pools[i];
		}

		free(this->Buffer);
		free(this->Pools);

		this->Pools = nullptr;
		this->Buffer = nullptr;

		this->BufferSize = 0;
		this->PoolsSize = 0;
		this->CurrentPool = 0;
		this->FrameSkip = 0;

		this->NullStart = 0;
	}

	BS_FUNCTION(GarbageCollector::RunWrapper) {
		return Converter::ToVariable(args->BromScript, args->BromScript->GC.Run());
	}

	int GarbageCollector::Run() {
		int cleaned = 0;
		this->FrameSkip = 0;

		int curc = 1337;
		while (curc > 0) {
			curc = 0;
			for (int i = 0; i < this->BufferSize; i++) {
				Variable* var = this->Buffer[i];
				if (var != nullptr && var->GetRefCount() == 0) {
					curc++;

					// number pooling, whooo
					if (var->Type == VariableType::Number) {
						this->NumberPool.Free((double*)var->Value);
						var->Value = nullptr;
					}

					if (var->PoolRef.ID == -1) {
						delete var;
					} else {
						Pool* p = this->Pools[var->PoolRef.ID];
						var->EmptyValue();
						var->RegisteredInGC = false;
						p->Free(var);
					}

					this->Buffer[i] = nullptr;
					if (i < this->NullStart) this->NullStart = i;
				}
			}

			cleaned += curc;
		}

		return cleaned * sizeof(Variable);
	}

	void GarbageCollector::RunFrame() {
		if (this->FrameSkip < 512) return;

		this->FrameSkip = 0;

		for (int i = 0; i < this->BuffHighest + 1; i++) {
			Variable* var = this->Buffer[i];

			if (var != nullptr && var->GetRefCount() == 0) {
				// number pooling, whooo
				if (var->Type == VariableType::Number) {
					this->NumberPool.Free((double*)var->Value);
					var->Value = nullptr;
				}

				if (var->PoolRef.ID == -1) {
					delete var;
				} else {
					Pool* p = this->Pools[var->PoolRef.ID];
					var->EmptyValue();
					var->RegisteredInGC = false;
					p->Free(var);
				}

				this->Buffer[i] = nullptr;
				if (i < this->NullStart) this->NullStart = i;
				if (i == this->BuffHighest) while (this->Buffer[--this->BuffHighest] == nullptr) { };
			}
		}
	}

	// use pooling
	Variable* GarbageCollector::GetPooledVariable() {
		Variable* ret = this->Pools[this->CurrentPool]->GetNext();
		if (ret != nullptr) return this->RegisterVariable(ret);

		// fallback
		for (int i = 0; i < this->PoolsSize; i++) {
			ret = this->Pools[this->CurrentPool]->GetNext();
			if (ret != nullptr) return this->RegisterVariable(ret);
		}

		// fallback's fallback
		this->AllocateMorePools();
		return this->RegisterVariable(this->Pools[this->CurrentPool]->GetNext());
	}
	
	// don't use pooling
	Variable* GarbageCollector::RegisterVariable(Variable* var) {
		if (var->RegisteredInGC)
			return var;

		var->RegisteredInGC = true;
		this->Buffer[this->NullStart] = var;

		if (this->NullStart > this->BuffHighest) this->BuffHighest = this->NullStart;

		bool foundnext = false;
		for (int i = this->NullStart + 1; i < this->BufferSize; i++) {
			if (this->Buffer[i] == nullptr) {
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

		if (this->Buffer != nullptr) {
			memcpy(newbuff, this->Buffer, sizeof(Variable*) * (this->BufferSize - 1024));
			free(this->Buffer);
		}

		memset(newbuff + (this->BufferSize - 1024), 0, sizeof(Variable*) * 1024);

		this->Buffer = newbuff;
	}

	void GarbageCollector::AllocateMorePools() {
		this->PoolsSize++;
		Pool** newbuff = (Pool**)malloc(sizeof(Pool*) * this->PoolsSize);

		if (this->Pools != nullptr) {
			memcpy(newbuff, this->Pools, sizeof(Pool*) * (this->PoolsSize - 1));
			free(this->Pools);
		}

		this->Pools = newbuff;
		this->CurrentPool++;
		this->Pools[this->CurrentPool] = new Pool(this->CurrentPool);
	}
}
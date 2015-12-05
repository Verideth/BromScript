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

#include "../Objects/ExecuteData.h"

using namespace Scratch;

namespace BromScript {
	ExecuteData::ExecuteData(void) : Reader(nullptr), StackPos(0), StackSize(0), Stack(nullptr) {
		this->AllocateMoreSpace(32);
	}

	ExecuteData::~ExecuteData(void) {
		for (int i = 0; i < this->StackPos; i++) {
			if (this->Stack[i] != null) {
				BS_REF_DECREESE(this->Stack[i]);
				this->Stack[i] = nullptr;
			}
		}

		if (this->Reader != null) delete this->Reader;
		if (this->Stack != nullptr) free(this->Stack);
	}

	void ExecuteData::AllocateMoreSpace(int num) {
		this->StackSize += num;
		Variable** newbuff = (Variable**)malloc(sizeof(Variable*) * this->StackSize);

		if (this->Stack != nullptr) {
			memcpy(newbuff, this->Stack, sizeof(Variable*) * (this->StackSize - num));
			free(this->Stack);
		}

		this->Stack = newbuff;
		memset(this->Stack + this->StackSize - num, 0, sizeof(Variable*) * num);
	}
}
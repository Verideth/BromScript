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

#include "Pool.h"
#include "PoolReference.h"

using namespace Scratch;

namespace BromScript{
	Pool::Pool(int id) {
		for (int i = 0; i < BS_POOL_SIZE; i++) {
			this->Buffer[i].PoolRef.ID = id;
			this->Buffer[i].PoolRef.Index = i;
		}

		// links
		for (int i = 0; i < BS_POOL_SIZE; i++) {
			this->BufferLinks[i].Data = &this->Buffer[i];
			//this->BufferLinks[i].PrevLink = i > 0 ? &this->BufferLinks[i - 1] : null;
			this->BufferLinks[i].NextLink = i + 1 < BS_POOL_SIZE ? &this->BufferLinks[i + 1] : null;
		}

		this->NextUnusedLink = &this->BufferLinks[0];
		this->NextUsedLink = null;
	}

	Variable* Pool::GetNext() {
		if (this->NextUnusedLink == null) return null;

		PoolLink* curlink = this->NextUnusedLink;
		Variable* ret = (Variable*)curlink->Data;
		if (ret == null) return null;

		this->NextUnusedLink = curlink->NextLink;

		curlink->NextLink = this->NextUsedLink;
		this->NextUsedLink = curlink;

		curlink->Data = null;
		return ret;
	}

	void Pool::Free(Variable* pooledvalue) {
		if (this->NextUsedLink == null) {
			throw "At some point, a object has been freed which was NOT from this pool, and this is just one of the sideeffects";
		}

		PoolLink* tmp = this->NextUsedLink->NextLink;
		this->NextUsedLink->NextLink = this->NextUnusedLink;
		this->NextUnusedLink = this->NextUsedLink;
		this->NextUsedLink = tmp;

		this->NextUnusedLink->Data = pooledvalue;
	}
}
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


#ifndef BROMSCRIPT_POOL_TYPED_CPP_INCLUDED
#define BROMSCRIPT_POOL_TYPED_CPP_INCLUDED

#include "PoolTyped.h"
#include "PoolReference.h"


namespace BromScript{
	template<class Type>
	PoolTyped<Type>::PoolTyped() {
		// links
		for (int i = 0; i < BS_POOL_SIZE; i++) {
			this->BufferLinks[i].Data = new Type();
			this->BufferLinks[i].NextLink = i + 1 < BS_POOL_SIZE ? &this->BufferLinks[i + 1] : null;
		}

		this->NextUnusedLink = &this->BufferLinks[0];
		this->NextUsedLink = null;
	}

	template<class Type>
	PoolTyped<Type>::~PoolTyped() {
		// links
		for (int i = 0; i < BS_POOL_SIZE; i++) {
			delete this->BufferLinks[i].Data;
		}
	}

	template<class Type>
	Type* PoolTyped<Type>::GetNext() {
		if (this->NextUnusedLink == null) return null;
		PoolLink* curlink = this->NextUnusedLink;
		Type* ret = (Type*)curlink->Data;

		this->NextUnusedLink = curlink->NextLink;

		curlink->NextLink = this->NextUsedLink;
		this->NextUsedLink = curlink;

		curlink->Data = null;
		return ret;
	}

	template<class Type>
	void PoolTyped<Type>::Free(Type* val) {
		if (this->NextUsedLink == null) {
			// we cached enough objects, delete it, and on in with our lives
			delete val;
			return;
		}

		PoolLink* tmp = this->NextUsedLink->NextLink;
		this->NextUsedLink->NextLink = this->NextUnusedLink;
		this->NextUnusedLink = this->NextUsedLink;
		this->NextUsedLink = tmp;

		this->NextUnusedLink->Data = val;
	}
}

#endif
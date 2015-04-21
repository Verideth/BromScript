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

#ifndef BROMSCRIPT_LIST_CPP_INCLUDED
#define BROMSCRIPT_LIST_CPP_INCLUDED

#include "List.h"

#include <cstdlib>
#include <cstring>

namespace BromScript{
	template<class Type>
	List<Type>::List(void) :Count(0), Buffer(null), BufferSize(0) {
		this->AllocateMoreSpace();
	}

	template<class Type>
	List<Type>::List(const List<Type>& copy) {
		this->Count = copy.Count;
		this->Buffer = null;
		this->BufferSize = 0;

		this->AllocateMoreSpace(copy.BufferSize);

		for (int i = 0; i < copy.Count; i++) {
			this->Buffer[i] = new Type;
			*this->Buffer[i] = *copy.Buffer[i];
		}
	}

	template<class Type>
	List<Type>::~List(void) {
		this->Clear();
		free(this->Buffer);
	}

	template<class Type>
	void List<Type>::Add(Type object) {
		if (this->Count == this->BufferSize)
			this->AllocateMoreSpace();

		Type* obj = new Type;
		*obj = object;

		this->Buffer[this->Count++] = obj;
	}

	template<class Type>
	void List<Type>::Insert(int index, Type object) {
		if (this->Count == this->BufferSize)
			this->AllocateMoreSpace();

		this->Count++;

		Type* obj = new Type;
		*obj = object;

		for (int i = this->Count - 1; i > index; i--)
			this->Buffer[i] = this->Buffer[i - 1];

		this->Buffer[index] = obj;
	}

	template<class Type>
	void List<Type>::Set(int i, Type object) {
		*this->Buffer[i] = object;
	}

	template<class Type>
	Type List<Type>::RemoveAt(int i) {
		if (i >= this->BufferSize) throw;

		Type object = *this->Buffer[i];
		delete this->Buffer[i];

		this->Count--;
		for (; i < this->Count; i++)
			this->Buffer[i] = this->Buffer[i + 1];

		this->Buffer[this->Count] = null;
		return object;
	}

	template<class Type>
	Type List<Type>::Get(int i) {
		return *this->Buffer[i];
	}

	template<class Type>
	Type List<Type>::operator[](int i) {
		return *this->Buffer[i];
	}

	template<class Type>
	void List<Type>::Clear() {
		for (int i = 0; i < this->Count; i++)
			delete this->Buffer[i];

		free(this->Buffer);

		this->Count = 0;
		this->Buffer = null;
		this->BufferSize = 0;
		this->AllocateMoreSpace();
	}

	template<class Type>
	void List<Type>::AllocateMoreSpace() {
		this->AllocateMoreSpace(128);
	}

	template<class Type>
	void List<Type>::AllocateMoreSpace(int num) {
		this->BufferSize += num;
		Type** newbuff = (Type**)malloc(sizeof(Type*) * this->BufferSize);

		if (this->Buffer != null) {
			memcpy(newbuff, this->Buffer, sizeof(Type*) * (this->BufferSize - num));
			free(this->Buffer);
		}

		this->Buffer = newbuff;
	}
}

#endif
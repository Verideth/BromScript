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

#include "../SIF.h"
#include "../Userdatas/BSUDIterator.h"
#include "../Managers/Instance.h"
#include "../Objects/Userdata.h"

namespace BromScript {
	namespace Userdatas {
		namespace Iterator {
			class IteratorClass {
			public:
				int CurrentIndex = 0;
				Variable* Object;

				IteratorClass(Variable* obj) :Object(obj) { }
			};

			BS_FUNCTION_CTOR(CTOR) {
				return new IteratorClass(args->GetVariable(0));
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (IteratorClass*)data;
			}

			BS_FUNCTION(NextKey) {
				IteratorClass* iter = (IteratorClass*)args->GetThisObjectData();

				iter->CurrentIndex = iter->Object->GetTable()->GetNextIndex(iter->CurrentIndex);

				if (iter->CurrentIndex == -1) return null;
				return Converter::ToVariable(bsi, iter->Object->GetTable()->GetKeyByIndex(iter->CurrentIndex++));
			}

			BS_FUNCTION(GetIndex) {
				IteratorClass* iter = (IteratorClass*)args->GetThisObjectData();
				return iter->Object->GetTable()->Get(args->GetVariable(0)->ToString());
			}

			BS_FUNCTION(Reset) {
				((IteratorClass*)args->GetThisObjectData())->CurrentIndex = 0;
				return null;
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("Iterator", BROMSCRIPT_USERDATA_ITERATOR_TYPE, sizeof(IteratorClass), CTOR, DTOR);

				vd->RegisterFunction("NextKey", NextKey);
				vd->RegisterFunction("GetIndex", GetIndex);
				vd->RegisterFunction("Reset", Reset);

				vd->RegisterOperator(Operators::ArithmeticGetIndex, GetIndex);
			}
		}
	}
}
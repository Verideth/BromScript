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

#ifndef BROMSCRIPT_EXECUTEDATA_INCLUDED
#define BROMSCRIPT_EXECUTEDATA_INCLUDED

#include "../SIF.h"

#include "../Managers/Instance.h"
#include "ByteReader.h"
#include "../Objects/Variable.h"
#include "../Objects/Function.h"

namespace BromScript {
	class ExecuteData {
	public:
		ExecuteData(void);
		~ExecuteData(void);

		Instance* BromScript;
		ByteReader* Reader;

		bool Breaking;
		bool Continueing;
		bool FuncCallback;
		Function* Function;

		bool HasReturnValue;
		Variable* Returning;

		int GotoReset;
		int GotoDeptDiff;
	};
}

#endif
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

#ifndef BROMSCRIPT_CONVERTER_INCLUDED
#define BROMSCRIPT_CONVERTER_INCLUDED

#include "../SIF.h"
#include "../Objects/Table.h"
#include "../Misc.h"
#include "../Objects/MemberData.h"

namespace BromScript{
	class Variable;

	class Converter {
	public:
		static Variable* ToVariable(Instance* bromscript, bool value);
		static Variable* ToVariable(Instance* bromscript, double value);
		static Variable* ToVariable(Instance* bromscript, int value);
		static Variable* ToVariable(Instance* bromscript, long long value);
		static Variable* ToVariable(Instance* bromscript, float value);
		static Variable* ToVariable(Instance* bromscript, const Scratch::CString &value);
		static Variable* ToVariable(Instance* bromscript, Table* value);
		static Variable* ToVariable(Instance* bromscript, const Scratch::CString &key, BSFunction value);

		static bool SetMember(Instance* bromscript, Variable* member, Variable* value, const Scratch::CString &key);
		static Variable* MemberToVariable(Instance* bromscript, Variable* member);

		static void* NumberToPointer(Instance* bromscript, double val);
		inline static void* NumberToPointer(double val) { return new double(val); }
		inline static void* BoolToPointer(bool val) { return (void*)(val ? 1 : 0); }
		inline static void* StringToPointer(const Scratch::CString &val) { return new Scratch::CString(val); }

		static Scratch::CString TypeToString(Variable* var);
		static Scratch::CString TypeToString(VariableType::Enum type);
		static Scratch::CString TypeToString(Instance* bromscript, VariableType::Enum type);
		static Scratch::CString ArithmaticToString(Operators type);

		static Scratch::CString VariableToString(Variable* var);
		static Scratch::CString VariableToString(Instance* bromscript, Variable* var);
	};
}

#endif
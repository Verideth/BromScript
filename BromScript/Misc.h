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

#ifndef BROMSCRIPT_MISC_INCLUDED
#define BROMSCRIPT_MISC_INCLUDED

namespace BromScript {
	class Misc {
	public:
		enum ExecFuncs {
			End = 0, SetVar, GetVar, ExecuteFunction, If, MergeStart, MergeEnd, String, Number, Table,
			Return = 14, LSetVar,
			ElseIf = 24, Else, While, Bool, GetTblIndex, SetTblIndex,
			Break = 30,
			Function = 33, ForEach, Loop,
			Class = 36, New, CSNew, For, PostIncrement, PostDecrement, PreIncrement, PreDecrement,
			Continue = 150, AnonFunction, Enum, GetCount, Goto, Rewind, Delete,
			GlobalLocals, StringTable = 253, CurrentLine = 254, Skip = 255
		};
	};

	enum class Operators {
		EndScope,

		Set, SetL,
		Get, GetL,
		Call, CallThis,
		GetIndex, SetIndex,
		AddIndex, New,
		GetCount,

		Jump, JumpNT,
		Return, Delete,

		StackNumber,
		StackString,
		StackTable,
		StackBool,
		StackNull,
		StackFunction,
		Pop, Duplicate,

		PreIncrement, PostIncrement,
		PreDecrement, PostDecrement,

		Arithmetic_START, ArithmeticAdd, ArithmeticSubstract, ArithmeticDivide, ArithmeticMultiply, ArithmeticAnd, ArithmeticOr,
		ArithmeticGreaterThan, ArithmeticLessThan, ArithmeticGreaterOrEqual, ArithmeticLessOrEqual, ArithmeticEqual,
		ArithmeticNotEqual, ArithmeticBitwiseLeft, ArithmeticBitwiseRight, ArithmeticBitwiseOr, ArithmeticBitwiseAnd,
		ArithmeticModulo, ArithmeticCall, ArithmeticGetIndex, ArithmeticSetIndex, ArithmeticGetCount, ArithmeticToString,
		Arithmetic_END,

		GlobalLocals = 157, StringTable = 253, CurrentLine = 254, Skip = 255
	};
}

#endif
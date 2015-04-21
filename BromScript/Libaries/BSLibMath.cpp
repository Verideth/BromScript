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
#include "BSLibMath.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"

using namespace Scratch;

namespace BromScript{
	namespace Libaries{
		namespace Math{
			bool CheckInput(ArgumentData* args, int min, int max) {
				if (args->Count < min || args->Count > max) {
					BS_THROW_ERROR(args, CString::Format("Invalid number of arguments (min %d, max %d)", min, max));
					return true;
				}

				for (int i = 0; i < args->Count; i++)
					if (!args->CheckType(i, VariableType::Number, true)) return true;

				return false;
			}

			BS_FUNCTION(Random) {
				if (CheckInput(args, 0, 2)) return null;

				if (args->Count == 0) return Converter::ToVariable((double)rand() / (double)RAND_MAX);
				if (args->Count == 1) return Converter::ToVariable((double)rand() / (double)RAND_MAX * args->GetNumber(0));
				else return Converter::ToVariable((double)rand() / (double)RAND_MAX * (args->GetNumber(1) - args->GetNumber(0)) + args->GetNumber(0));
			}

			BS_FUNCTION(Floor) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(floor(args->GetNumber(0)));
			}

			BS_FUNCTION(Min) {
				if (CheckInput(args, 2, 2)) return null;
				return Converter::ToVariable(args->GetNumber(0) < args->GetNumber(1) ? args->GetNumber(0) : args->GetNumber(1));
			}

			BS_FUNCTION(Max) {
				if (CheckInput(args, 2, 2)) return null;
				return Converter::ToVariable(args->GetNumber(0) > args->GetNumber(1) ? args->GetNumber(0) : args->GetNumber(1));
			}

			BS_FUNCTION(Ceiling) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(ceil(args->GetNumber(0)));
			}

			BS_FUNCTION(Round) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(floor(args->GetNumber(0) + 0.5));
			}

			BS_FUNCTION(Atan) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(atan(args->GetNumber(0)));
			}

			BS_FUNCTION(Atan2) {
				if (CheckInput(args, 2, 2)) return null;
				return Converter::ToVariable(atan2(args->GetNumber(0), args->GetNumber(1)));
			}

			BS_FUNCTION(Cos) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(cos(args->GetNumber(0)));
			}

			BS_FUNCTION(Sin) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(sin(args->GetNumber(0)));
			}

			BS_FUNCTION(Tan) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(tan(args->GetNumber(0)));
			}

			BS_FUNCTION(Pow) {
				if (CheckInput(args, 2, 2)) return null;
				return Converter::ToVariable(pow(args->GetNumber(0), args->GetNumber(1)));
			}

			BS_FUNCTION(Sign) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable((double)(args->GetNumber(0) < 0 ? -1 : args->GetNumber(0) > 0 ? 1 : 0));
			}

			BS_FUNCTION(Sqrt) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(sqrt(args->GetNumber(0)));
			}

			BS_FUNCTION(Abs) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(abs(args->GetNumber(0)));
			}

			BS_FUNCTION(Neg) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(-abs(args->GetNumber(0)));
			}

			BS_FUNCTION(ToRadians) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(BS_PI * args->GetNumber(0) / 180.0);
			}

			BS_FUNCTION(ToDegrees) {
				if (CheckInput(args, 1, 1)) return null;
				return Converter::ToVariable(args->GetNumber(0) * (180.0 / BS_PI));
			}
		}
	}
}
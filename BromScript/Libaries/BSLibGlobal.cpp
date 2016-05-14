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

#include "BSLibGlobal.h"
#include "../SIF.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"

#ifdef _MSC_VER
#include <Windows.h>
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

using namespace Scratch;

namespace BromScript{
	namespace Libaries{
		namespace Global{
			BS_FUNCTION(Environment) {
#ifdef _MSC_VER
				return Converter::ToVariable(bsi, "Windows");
#else
				return Converter::ToVariable(bsi, "Linux");
#endif
			}

			BS_FUNCTION(Sleep) {
				if (args->Count != 1) { BS_THROW_ERROR(args, "Expected number, got nothing!"); return null; }
				if (!args->CheckType(0, VariableType::Number, true)) return null;

#ifdef _MSC_VER
				::Sleep((int)args->GetNumber(0));
#else
				sleep((int)args->GetNumber(0));
#endif

				return null;
			}

			BS_FUNCTION(Call) {
				if (!args->CheckType(0, VariableType::Function, true)) return null;

				Function* tryfunc = args->GetFunction(0);
				List<Variable*> vars;

				for (int i = 1; i < args->Count; i++)
					vars.Add(args->GetVariable(i));

				tryfunc->Run(&vars);

				return Converter::ToVariable(bsi, !args->ErrorOccured());
			}

			BS_FUNCTION(Print) {
				if (args->Count == 0) { BS_THROW_ERROR(args, "Expected one or more variable, got nothing!"); return null; }
				CString out;
				for (int i = 0; i < args->Count; i++) {
					if (i > 0)
						out += '\t';

					out += Converter::VariableToString(bsi, args->GetVariable(i));
				}

				out += "\n";
				printf("%s", out.str_szBuffer);
				return null;
			}

			BS_FUNCTION(ToString) {
				if (args->Count != 1) { BS_THROW_ERROR(args, "Expected variable, got nothing!"); return null; }
				return Converter::ToVariable(bsi, Converter::VariableToString(bsi, args->GetVariable(0)));
			}

			BS_FUNCTION(ToNumber) {
				if (args->Count != 1) { BS_THROW_ERROR(args, "Expected variable, got nothing!"); return null; }
				return bsi->ToVariable(atof(args->GetVariable(0)->ToString(bsi)));
			}

			BS_FUNCTION(TypeOf) {
				if (args->Count != 1) { BS_THROW_ERROR(args, "Expected variable, got nothing!"); return null; }
				return Converter::ToVariable(bsi, Converter::TypeToString(args->GetVariable(0)));
			}

			BS_FUNCTION(Time) {
				time_t time = clock() / (CLOCKS_PER_SEC / 1000);
				double ms = (double)(time - time % 1000) / 1000.0;
				time /= 1000;

				double ret = (double)time + ms;
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(Include) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				return bsi->DoFile(args->GetString(0), args->Caller->Env, args->CheckType(1, VariableType::Bool, false) ? args->GetBool(1) : true);
			}

#ifdef _MSC_VER
			BS_FUNCTION(IncludeDll) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				bsi->DoDLL(args->GetString(0));
				return null;
			}
#else
			BS_FUNCTION(IncludeSO) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				bsi->DoSO(args->GetString(0));
				return null;
			}
#endif

			BS_FUNCTION(SetReadOnly) {
				if (args->Count != 1) {
					BS_THROW_ERROR(args, "Expected variable, got nothing!");
					return null;
				}

				args->GetVariable(0)->ReadOnly = true;
				return null;
			}

			BS_FUNCTION(Error) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				BS_THROW_ERROR(args, args->GetString(0));
				return null;
			}
		}
	}
}
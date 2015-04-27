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
#include "BSLibString.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"

using namespace Scratch;

namespace BromScript{
	namespace Libaries{
		namespace String{
			BS_FUNCTION(FromChar) {
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				return Converter::ToVariable(bsi, CString((char)args->GetNumber(0)));
			}

			BS_FUNCTION(ToChar) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				CString str = args->GetString(0);
				int len = str.Size();
				if (len == 0) {
					BS_THROW_ERROR(args, "Cannot convert 0 sized string to char");
					return null;
				}

				if (len > 1) {
					BS_THROW_ERROR(args, "Cannot convert a string larger than 1 to char");
					return null;
				}

				return Converter::ToVariable(bsi, (int)str[0]);
			}

			BS_FUNCTION(Split) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::String, true)) return null;

				CStackArray<CString> arr;
				CString(args->GetString(0)).Split(args->GetString(1), arr);

				Table* tbl = new Table(args->BromScript);
				for (int i = 0; i < arr.Count(); i++)
					tbl->Set(CString((double)i), Converter::ToVariable(bsi, arr[i]));

				return Converter::ToVariable(bsi, tbl);
			}

			BS_FUNCTION(Sub) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;

				if (args->Count == 3 && !args->CheckType(2, VariableType::Number, true)) return null;

				if (args->Count == 3) return Converter::ToVariable(bsi, CString(args->GetString(0)).Substring((int)args->GetNumber(1), (int)args->GetNumber(2)));
				else return Converter::ToVariable(bsi, CString(args->GetString(0)).Substring((int)args->GetNumber(1)));
			}

			BS_FUNCTION(IndexOf) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (args->Count < 2) {
					BS_THROW_ERROR(args, "Expected 2(or 3) arguments (String, Char/String, [Number])!");
					return null;
				}

				if (args->CheckType(1, VariableType::String)) {
					return Converter::ToVariable(bsi, CString(args->GetString(0)).IndexOf(args->GetString(1), (int)args->GetNumber(2)));
				} else if (args->CheckType(1, VariableType::Number)) {
					return Converter::ToVariable(bsi, CString(args->GetString(0)).IndexOf((char)args->GetNumber(1), (int)args->GetNumber(2)));
				} else {
					BS_THROW_ERROR(args, "Expected 2(or 3) arguments (String, Char/String, [Number])!");
					return null;
				}
			}

			BS_FUNCTION(LastIndexOf) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (args->Count < 2) {
					BS_THROW_ERROR(args, "Expected 2(or 3) arguments (String, Char/String, [Number])!");
					return null;
				}

				CString str = args->GetString(0);
				int spos = str.Size() - 1;
				if (args->Count == 3) {
					if (!args->CheckType(2, VariableType::Number, true)) return null;
					spos = (int)args->GetNumber(2);
				}

				if (args->CheckType(1, VariableType::String)) {
					return Converter::ToVariable(bsi, str.IndexOf(args->GetString(1), spos));
				} else if (args->CheckType(1, VariableType::Number)) {
					return Converter::ToVariable(bsi, str.IndexOf((char)args->GetNumber(1), spos));
				} else {
					BS_THROW_ERROR(args, "Expected 2(or 3) arguments (String, Char/String, [Number])!");
					return null;
				}
			}

			BS_FUNCTION(Left) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;
				return Converter::ToVariable(bsi, CString(args->GetString(0)).Substring((int)args->GetNumber(1)));
			}

			BS_FUNCTION(Right) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;
				return Converter::ToVariable(bsi, CString(args->GetString(0)).Substring(0, (int)args->GetNumber(1) * -1));
			}

			BS_FUNCTION(ToLower) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				return Converter::ToVariable(bsi, CString(args->GetString(0)).ToLower());
			}

			BS_FUNCTION(ToUpper) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				return Converter::ToVariable(bsi, CString(args->GetString(0)).ToUpper());
			}

			BS_FUNCTION(Trim) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				if (args->Count == 1) {
					return Converter::ToVariable(bsi, CString(args->GetString(0)).Trim());
				} else {
					if (!args->CheckType(1, VariableType::String, true)) return null;

					CString str = args->GetString(0);
					CString chars = args->GetString(1);
					int len = chars.Size();
					int strlen = -1;
					while (strlen != str.Size()) {
						strlen = str.Size();

						for (int i = 0; i < len; i++) {
							str = str.Trim(chars[i]);
						}
					}

					return Converter::ToVariable(bsi, str);
				}
			}

			BS_FUNCTION(TrimLeft) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				if (args->Count == 1) {
					return Converter::ToVariable(bsi, CString(args->GetString(0)).TrimLeft());
				} else {
					if (!args->CheckType(1, VariableType::String, true)) return null;

					CString str = args->GetString(0);
					CString chars = args->GetString(1);
					int len = chars.Size();
					int strlen = -1;
					while (strlen != str.Size()) {
						strlen = str.Size();

						for (int i = 0; i < len; i++) {
							str = str.TrimLeft(chars[i]);
						}
					}

					return Converter::ToVariable(bsi, str);
				}
			}

			BS_FUNCTION(TrimRight) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				if (args->Count == 1) {
					return Converter::ToVariable(bsi, CString(args->GetString(0)).TrimRight());
				} else {
					if (!args->CheckType(1, VariableType::String, true)) return null;

					CString str = args->GetString(0);
					CString chars = args->GetString(1);
					int len = chars.Size();
					int strlen = -1;
					while (strlen != str.Size()) {
						strlen = str.Size();

						for (int i = 0; i < len; i++) {
							str = str.TrimRight(chars[i]);
						}
					}

					return Converter::ToVariable(bsi, str);
				}
			}

			BS_FUNCTION(Replace) {
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::String, true)) return null;
				if (!args->CheckType(2, VariableType::String, true)) return null;

				return Converter::ToVariable(bsi, CString(args->GetString(0)).Replace(args->GetString(1), args->GetString(2)));
			}
		}
	}
}
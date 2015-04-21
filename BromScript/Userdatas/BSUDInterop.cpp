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

#include "../Userdatas/BSUDInterop.h"
#include "../Userdatas/BSUDInteropMethod.h"

#include "../SIF.h"
#include "../Managers/Instance.h"

namespace BromScript{
	namespace Userdatas{
		namespace Interop{
			CInterop::CInterop(Scratch::CString filename) :Filename(filename) {
#ifdef _MSC_VER
				this->LibHandle = LoadLibrary(this->Filename.str_szBuffer);
#else
				this->LibHandle = dlopen(this->Filename.str_szBuffer, RTLD_LAZY);
#endif
			}

			CInterop::~CInterop() {
				this->Close();
			}

			void CInterop::Close() {
				if (this->LibHandle != null) {
#ifdef _MSC_VER
					FreeLibrary(this->LibHandle);
#else
					dlclose(this->LibHandle);
#endif
					this->LibHandle = null;
				}
			}

			void* CInterop::GetMethod(Scratch::CString filename) {
#ifdef _MSC_VER
				return GetProcAddress(this->LibHandle, filename.str_szBuffer);
#else
				return dlsym(this->LibHandle, filename.str_szBuffer);
#endif
			}

			BS_FUNCTION_CTOR(CTOR) {
				if (!args->CheckType(0, VariableType::String, true)) return null;

				CInterop* ret = new CInterop(args->GetString(0));
				if (ret->LibHandle == null) {
#ifdef _MSC_VER
					LPTSTR errorText = NULL;
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorText, 0, NULL);

					BS_THROW_ERROR(args, errorText);
					LocalFree(errorText);
#else
					BS_THROW_ERROR(args, Scratch::CString::Format("Error opening '%s'", args->GetString(0)));
#endif

					delete ret;
					return null;
				}

				return ret;
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (CInterop*)data;
			}

			BS_FUNCTION(GetMethod) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_INTEROP_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::String, true)) return null;
				if (!args->CheckType(1, VariableType::String, true)) return null;
				if (!args->CheckType(2, VariableType::Table, true)) return null;

				CInterop* iop = (CInterop*)args->GetThisObjectData();

				void* funcptr = iop->GetMethod(args->GetString(0));
				if (funcptr == null) {
#ifdef _MSC_VER
					LPTSTR errorText = NULL;
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorText, 0, NULL);

					BS_THROW_ERROR(args, errorText);
					LocalFree(errorText);
#else
					BS_THROW_ERROR(args, Scratch::CString::Format("Could not find function '%s'", args->GetString(0)));
#endif
					return null;
				}

				Table* tbl = args->GetTable(2);
				int* types = new int[tbl->Count];

				int ti = 0;
				int i = -1;
				while ((i = tbl->GetNextIndex(i + 1)) != -1) {
					Variable* v = tbl->GetByIndex(i);

					Scratch::CString tn = v->GetString().ToLower();

					if (tn == "bool") types[ti++] = MemberType::Bool;
					else if (tn == "byte") types[ti++] = MemberType::Byte;
					else if (tn == "short") types[ti++] = MemberType::Short;
					else if (tn == "int") types[ti++] = MemberType::Int;
					else if (tn == "float") types[ti++] = MemberType::Float;
					else if (tn == "long") types[ti++] = MemberType::Long;
					else if (tn == "double") types[ti++] = MemberType::Double;
					else if (tn == "string") types[ti++] = MemberType::String;
					else if (tn == "userdata") types[ti++] = MemberType::Userdata;
					else if (tn == "out") types[ti++] = MemberType::Long; // WARNING, we use long here, as we'll never do that. so this is reserved for out
					else {
						BS_THROW_ERROR(args, Scratch::CString::Format("Unknown type: '%s'", tn.str_szBuffer));
						delete[] types;
						return null;
					}
				}

				int rettype = 0;
				Scratch::CString tn(args->GetString(1));
				tn = tn.ToLower();

				if (tn == "bool") rettype = MemberType::Bool;
				else if (tn == "byte") rettype = MemberType::Byte;
				else if (tn == "short") rettype = MemberType::Short;
				else if (tn == "int") rettype = MemberType::Int;
				else if (tn == "float") rettype = MemberType::Float;
				else if (tn == "string") rettype = MemberType::String;
				else if (tn == "void") rettype = -1;
				else {
					BS_THROW_ERROR(args, Scratch::CString::Format("Unknown type: '%s'", tn.str_szBuffer));
					delete[] types;
					return null;
				}


				return bsi->CreateUserdata(BROMSCRIPT_USERDATA_INTEROPMETHOD_TYPE, new InteropMethod::CInteropMethod(args->GetString(0), funcptr, rettype, types, tbl->Count), true);
			}

			BS_FUNCTION(Close) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_INTEROP_TYPE, true)) return null;

				CInterop* iop = (CInterop*)args->GetThisObjectData();
				iop->Close();

				return null;
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("Interop", BROMSCRIPT_USERDATA_INTEROP_TYPE, sizeof(CInterop), CTOR, DTOR);

				vd->RegisterFunction("GetMethod", GetMethod);
				vd->RegisterFunction("Close", Close);
			}
		}
	}
}
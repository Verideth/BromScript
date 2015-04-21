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

#include "../Userdatas/BSUDInteropMethod.h"

#include "../SIF.h"
#include "../Managers/Instance.h"

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <cstdlib>
#include <dlfcn.h>
#endif

namespace BromScript{
	namespace Userdatas{
		namespace InteropMethod{
			CInteropMethod::CInteropMethod(Scratch::CString fname, void* funcptr, int rettype, int* args, int argcount) :FunctionName(fname), FunctionPtr(funcptr), ReturnType(rettype), Args(args), ArgCount(argcount) {}
			CInteropMethod::~CInteropMethod() {
				delete[] this->Args;
			}

			void* CInteropMethod::Call() {
				return null;
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (CInteropMethod*)data;
			}

			BS_FUNCTION(Call) {
#ifdef _MSC_VER
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_INTEROPMETHOD_TYPE, true)) return null;

				CInteropMethod* iopm = (CInteropMethod*)args->GetThisObjectData();


				if (args->Count != iopm->ArgCount) {
					BS_THROW_ERROR(args, "Amount of arguments given did not match argument count of given interlop method.");
					return null;
				}

				for (int i = args->Count - 1; i > -1; i--) {
					Variable* v = args->GetVariable(i);

					switch (iopm->Args[i]) {
						case MemberType::Bool: {
							int vardata = v->GetBool();
							_asm{ push vardata };
						} break;

						case MemberType::Byte: { // stack is 32 bit, so don't push a char but make it a int instead.
							int vardata = (int)v->GetNumber();
							_asm{ push vardata };
						} break;

						case MemberType::Int: {
							int vardata = (int)v->GetNumber();
							_asm{ push vardata };
						} break;

						case MemberType::Short: {
							short vardata = (short)v->GetNumber();
							_asm{ push vardata };
						} break;

						case MemberType::String: {
							const char* vardata = ((Scratch::CString*)v->Value)->str_szBuffer;
							_asm{ push vardata };
						} break;

						case MemberType::Long: { // this is reserved for the out op
							int vardata = (int)v->Value;
							_asm{ push vardata };
						} break;
					}
				}

				void* fptr = iopm->FunctionPtr;
				int retdata = -1;

				_asm{
					call fptr
						mov retdata, eax
				}

				for (int i = 0; i < args->Count; i++) {
					_asm{
						pop ebx
					};
				}

				switch (iopm->ReturnType) {
					case -1: return null;
					case MemberType::Bool: return Converter::ToVariable(retdata == 1 ? true : false);
					case MemberType::Byte: return Converter::ToVariable(retdata);
					case MemberType::Short: return Converter::ToVariable((short)retdata);
					case MemberType::Int: return Converter::ToVariable(retdata);
					case MemberType::Float: return Converter::ToVariable(*(float*)&retdata);
					case MemberType::String: return Converter::ToVariable((char*)retdata);
				}

				return null;
#else
				BS_THROW_ERROR(args, "Interop not supported on linux yet, sorry!");
				return null;
#endif
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("InteropMethod", BROMSCRIPT_USERDATA_INTEROPMETHOD_TYPE, sizeof(CInteropMethod), null, DTOR);

				vd->RegisterFunction("Call", Call);
				vd->RegisterOperator(Misc::ArithmaticFuncs::Call, Call);
			}
		}
	}
}
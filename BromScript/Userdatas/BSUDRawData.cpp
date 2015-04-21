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

#include "../Userdatas/BSUDRawData.h"

#include "../SIF.h"
#include "../Managers/Instance.h"

namespace BromScript{
	namespace Userdatas{
		namespace RawData{
			CRawData::CRawData(int size) :DataSize(size) {
				this->Data = new unsigned char[size];
			}

			CRawData::~CRawData() {
				delete[] this->Data;
			}

			BS_FUNCTION_CTOR(CTOR) {
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				return new CRawData((int)args->GetNumber(0));
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (CRawData*)data;
			}

			BS_FUNCTION(GetByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset + 1 > rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				return Converter::ToVariable(*(unsigned char*)(rd->Data + offset));
			}

			BS_FUNCTION(GetShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset + 2 > rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				return Converter::ToVariable(*(short*)(rd->Data + offset));
			}

			BS_FUNCTION(GetInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset + 4 > rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				return Converter::ToVariable(*(int*)(rd->Data + offset));
			}

			BS_FUNCTION(GetFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset + 4 > rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				return Converter::ToVariable(*(float*)(rd->Data + offset));
			}

			BS_FUNCTION(SetByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset >= rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				unsigned char newvalue = (unsigned char)args->GetNumber(1);
				memcpy(rd->Data + offset, &newvalue, 1);
				return null;
			}

			BS_FUNCTION(SetShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset >= rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				short newvalue = (short)args->GetNumber(1);
				memcpy(rd->Data + offset, &newvalue, 2);
				return null;
			}

			BS_FUNCTION(SetInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset >= rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				int newvalue = (int)args->GetNumber(1);
				memcpy(rd->Data + offset, &newvalue, 4);
				return null;
			}

			BS_FUNCTION(SetFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Number, true)) return null;
				if (!args->CheckType(1, VariableType::Number, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				int offset = (int)args->GetNumber(0);

				if (offset < 0 || offset >= rd->DataSize) {
					BS_THROW_ERROR(args, "index is out of range");
					return null;
				}

				float newvalue = (float)args->GetNumber(1);
				memcpy(rd->Data + offset, &newvalue, 4);
				return null;
			}

			BS_FUNCTION(GetReference) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_RAWDATA_TYPE, true)) return null;

				CRawData* rd = (CRawData*)args->GetThisObjectData();
				return Converter::ToVariable((int)rd->Data);
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("RawData", BROMSCRIPT_USERDATA_RAWDATA_TYPE, sizeof(CRawData), CTOR, DTOR);

				vd->RegisterFunction("SetByte", SetByte);
				vd->RegisterFunction("SetShort", SetShort);
				vd->RegisterFunction("SetInt", SetInt);
				vd->RegisterFunction("SetFloat", SetFloat);

				vd->RegisterFunction("GetByte", GetByte);
				vd->RegisterFunction("GetShort", GetShort);
				vd->RegisterFunction("GetInt", GetInt);
				vd->RegisterFunction("GetFloat", GetFloat);

				vd->RegisterFunction("GetReference", GetReference);
			}
		}
	}
}
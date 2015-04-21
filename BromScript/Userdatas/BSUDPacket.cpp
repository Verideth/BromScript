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

#include "../Userdatas/BSUDPacket.h"
#include "../Userdatas/BSUDSocket.h"

#include "../SIF.h"
#include "../Managers/Instance.h"
#include "../Objects/BSEzSock.h"
#include "../Objects/BSPacket.h"
#include "../Scratch/CString.h"

namespace BromScript{
	namespace Userdatas{
		namespace Packet{
			BS_FUNCTION_CTOR(CTOR) {
				return new BromScript::Packet();
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (BromScript::Packet*)data;
			}

			BS_FUNCTION(ReadBool) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable(((BromScript::Packet*)args->GetThisObjectData())->ReadBool());
			}

			BS_FUNCTION(ReadByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable((double)((BromScript::Packet*)args->GetThisObjectData())->ReadByte());
			}

			BS_FUNCTION(ReadShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable((double)((BromScript::Packet*)args->GetThisObjectData())->ReadShort());
			}

			BS_FUNCTION(ReadInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable((double)((BromScript::Packet*)args->GetThisObjectData())->ReadInt());
			}

			BS_FUNCTION(ReadFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable((double)((BromScript::Packet*)args->GetThisObjectData())->ReadFloat());
			}

			BS_FUNCTION(ReadLong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable((double)((BromScript::Packet*)args->GetThisObjectData())->ReadLong());
			}

			BS_FUNCTION(ReadDouble) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable(((BromScript::Packet*)args->GetThisObjectData())->ReadDouble());
			}

			BS_FUNCTION(ReadString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				return Converter::ToVariable(((BromScript::Packet*)args->GetThisObjectData())->ReadString());
			}

			BS_FUNCTION(ReadLine) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				BromScript::Packet* p = (BromScript::Packet*)args->GetThisObjectData();

				int spos = p->InPos;
				while (p->CanRead(1)) {
					if (p->InBuffer[p->InPos++] == '\n') {
						char* buff = new char[p->InPos - spos];
						memcpy(buff, p->InBuffer + spos, p->InPos - spos);

						Scratch::CString str;
						str.str_szBuffer = buff;
						return Converter::ToVariable(str);
					}
				}

				if (p->InPos != spos) {
					char* buff = new char[p->InPos - spos];
					memcpy(buff, p->InBuffer + spos, p->InPos - spos);

					Scratch::CString str;
					str.str_szBuffer = buff;
					return Converter::ToVariable(str);
				}

				return null;
			}

			BS_FUNCTION(ToString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				BromScript::Packet* p = (BromScript::Packet*)args->GetThisObjectData();

				Scratch::CString str;
				str.str_szBuffer = new char[p->InSize + 1];
				memcpy(str.str_szBuffer, p->InBuffer, p->InSize);
				str.str_szBuffer[p->InSize] = 0;

				return Converter::ToVariable(str);
			}

			BS_FUNCTION(WriteBool) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true)) return null;
				if (!args->CheckType(0, VariableType::Bool, true)) return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteBool(args->GetBool(0));
				return null;
			}

			BS_FUNCTION(WriteByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteByte((unsigned char)args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteShort((short)args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteInt((int)args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteFloat((float)args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteLong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteLong((long long)args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteDouble) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				((BromScript::Packet*)args->GetThisObjectData())->WriteDouble(args->GetNumber(0));
				return null;
			}

			BS_FUNCTION(WriteString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::String, true))return null;

				BromScript::Packet* p = (BromScript::Packet*)args->GetThisObjectData();
				if (args->CheckType(1, VariableType::Bool) && args->GetBool(1)) {
					Scratch::CString str = args->GetString(0);
					p->WriteBytes((unsigned char*)str.str_szBuffer, str.Size() + 1, false);
				} else
					p->WriteString(args->GetString(0));

				return null;
			}

			BS_FUNCTION(WriteLine) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::String, true))return null;

				BromScript::Packet* p = (BromScript::Packet*)args->GetThisObjectData();
				p->WriteBytes((unsigned char*)args->GetString(0), strlen(args->GetString(0)), false);
				p->WriteBytes((unsigned char*)"\r\n", 2, false);
				return null;
			}

			BS_FUNCTION(WriteBytes) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;
				if (!args->CheckType(0, BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;

				BromScript::Packet* p = (BromScript::Packet*)args->GetThisObjectData();
				BromScript::Packet* buffer = (BromScript::Packet*)args->GetUserdata(0, BROMSCRIPT_USERDATA_PACKET_TYPE);
				p->WriteBytes(buffer->InBuffer, buffer->InSize, false);
				return null;
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("Packet", BROMSCRIPT_USERDATA_PACKET_TYPE, sizeof(EzSock), CTOR, DTOR);

				vd->RegisterFunction("ReadBool", ReadBool);
				vd->RegisterFunction("ReadByte", ReadByte);
				vd->RegisterFunction("ReadShort", ReadShort);
				vd->RegisterFunction("ReadInt", ReadInt);
				vd->RegisterFunction("ReadFloat", ReadFloat);
				vd->RegisterFunction("ReadLong", ReadLong);
				vd->RegisterFunction("ReadDouble", ReadDouble);
				vd->RegisterFunction("ReadString", ReadString);
				vd->RegisterFunction("ReadLine", ReadString);
				vd->RegisterFunction("ToString", ToString);

				vd->RegisterFunction("WriteBool", WriteBool);
				vd->RegisterFunction("WriteByte", WriteByte);
				vd->RegisterFunction("WriteBytes", WriteBytes);
				vd->RegisterFunction("WriteShort", WriteShort);
				vd->RegisterFunction("WriteInt", WriteInt);
				vd->RegisterFunction("WriteFloat", WriteFloat);
				vd->RegisterFunction("WriteLong", WriteLong);
				vd->RegisterFunction("WriteDouble", WriteDouble);
				vd->RegisterFunction("WriteString", WriteString);
				vd->RegisterFunction("WriteLine", WriteLine);
			}
		}
	}
}
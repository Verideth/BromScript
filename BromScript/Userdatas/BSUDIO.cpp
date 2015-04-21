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

#include "../Userdatas/BSUDIO.h"
#include "../Userdatas/BSUDPacket.h"

#include "../SIF.h"
#include "../Managers/Instance.h"
#include "../Objects/BSEzSock.h"
#include "../Objects/BSPacket.h"
#include "../Scratch/CString.h"

namespace BromScript {
	namespace Userdatas {
		namespace IO {
			BS_FUNCTION_CTOR(CTOR) {
				return new Scratch::CFileStream();
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (Scratch::CFileStream*)data;
			}

			BS_FUNCTION(Open) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::String, true))return null;
				if (!args->CheckType(1, VariableType::String, true))return null;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh != null) {
					BS_THROW_ERROR(args, "IO object already busy!");
					return null;
				}

				return Converter::ToVariable(fs->Open(args->GetString(0), args->GetString(1)));
			}

			BS_FUNCTION(Close) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true))return null;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh == null) {
					BS_THROW_ERROR(args, "No file opened!");
					return null;
				}

				fs->Close();
				return null;
			}

			BS_FUNCTION(ReadToEnd) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true))return null;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh == null) {
					BS_THROW_ERROR(args, "No file opened!");
					return null;
				}

				BromScript::Packet* p = new BromScript::Packet();
				p->InBuffer = (unsigned char*)fs->ReadToEnd();
				if (p->InBuffer == null) {
					delete p;
					BS_THROW_ERROR(args, "Error while reading stream");
					return null;
				}

				p->InSize = fs->Size();

				return bsi->CreateUserdata(BROMSCRIPT_USERDATA_PACKET_TYPE, p, true);
			}

			BS_FUNCTION(GetLength) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true))return null;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				return Converter::ToVariable((double)fs->Size());
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("IO", BROMSCRIPT_USERDATA_IO_TYPE, sizeof(Scratch::CFileStream), CTOR, DTOR);

				vd->RegisterFunction("Open", Open);
				vd->RegisterFunction("Close", Close);
				vd->RegisterFunction("ReadToEnd", ReadToEnd);
				vd->RegisterFunction("GetLength", GetLength);
			}
		}
	}
}
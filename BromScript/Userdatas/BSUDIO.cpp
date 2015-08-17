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

#include <sys/stat.h>
#include <direct.h>
#include "../Libaries/dirent.h"

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
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;
				if (!args->CheckType(1, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh != null) {
					BS_THROW_ERROR(args, "IO object already busy!");
					return nullptr;
				}

				return Converter::ToVariable(bsi, fs->Open(args->GetString(0), args->GetString(1)));
			}

			BS_FUNCTION(Close) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh == null) {
					BS_THROW_ERROR(args, "No file opened!");
					return nullptr;
				}

				fs->Close();
				return nullptr;
			}

			BS_FUNCTION(ReadToEnd) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				if (fs->fs_pfh == null) {
					BS_THROW_ERROR(args, "No file opened!");
					return nullptr;
				}

				BromScript::Packet* p = new BromScript::Packet();
				p->InBuffer = (unsigned char*)fs->ReadToEnd();
				if (p->InBuffer == null) {
					delete p;
					BS_THROW_ERROR(args, "Error while reading stream");
					return nullptr;
				}

				p->InSize = fs->Size();

				return bsi->CreateUserdata(BROMSCRIPT_USERDATA_PACKET_TYPE, p, true);
			}

			BS_FUNCTION(GetLength) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				return Converter::ToVariable(bsi, (double)fs->Size());
			}

			BS_FUNCTION(SetPos) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				fseek(((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh, (long)args->GetNumber(0), SEEK_SET);
				return nullptr;
			}

			BS_FUNCTION(GetPos) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				return Converter::ToVariable(bsi, (double)ftell(((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh));
			}

			BS_FUNCTION(ReadBool) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned char ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret == 1);
			}

			BS_FUNCTION(ReadByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned char ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				short ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadUShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned short ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				int ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadUInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned int ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, (double)ret);
			}

			BS_FUNCTION(ReadFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				float ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadLong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				long ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, (double)ret);
			}

			BS_FUNCTION(ReadULong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned long ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, (double)ret);
			}

			BS_FUNCTION(ReadDouble) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				double ret;
				fread(&ret, 1, sizeof(ret), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);
				return Converter::ToVariable(bsi, ret);
			}

			BS_FUNCTION(ReadString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(VariableType::Number, true)) return nullptr;

				int len = (int)args->GetNumber(0);
				char* buff = new char[len + 1];
				buff[len] = 0;

				fread(buff, 1, len, ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				Variable* ret = Converter::ToVariable(bsi, buff);

				delete[] buff;
				return ret;
			}

			BS_FUNCTION(ReadByteString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				unsigned char len;
				fread(&len, 1, sizeof(len), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				char* buff = new char[len + 1];
				buff[len] = 0;

				fread(buff, 1, len, ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				Variable* ret = Converter::ToVariable(bsi, buff);

				delete[] buff;
				return ret;
			}

			BS_FUNCTION(ReadShortString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				short len;
				fread(&len, 1, sizeof(len), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				char* buff = new char[len + 1];
				buff[len] = 0;

				fread(buff, 1, len, ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				Variable* ret = Converter::ToVariable(bsi, buff);

				delete[] buff;
				return ret;
			}

			BS_FUNCTION(ReadIntString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				int len;
				fread(&len, 1, sizeof(len), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				char* buff = new char[len + 1];
				buff[len] = 0;

				fread(buff, 1, len, ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				Variable* ret = Converter::ToVariable(bsi, buff);

				delete[] buff;
				return ret;
			}

			BS_FUNCTION(ReadNullString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				std::string str;
				while (true) {
					char c;
					fread(&c, 1, sizeof(c), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

					if (c == 0) break;
					str += c;
				}

				return Converter::ToVariable(bsi, str.c_str());
			}

			BS_FUNCTION(ReadLine) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;

				std::string str;
				while (true) {
					char c;
					fread(&c, 1, sizeof(c), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

					if (c == '\n') break;
					str += c;
				}

				if (str[str.size() - 1] == '\r') str[str.size() - 1] = 0;
				return Converter::ToVariable(bsi, str.c_str());
			}

			BS_FUNCTION(WriteBool) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Bool, true)) return nullptr;

				bool obj = (bool)args->GetBool(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteByte) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				unsigned char obj = (unsigned char)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				short obj = (short)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteUShort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				unsigned short obj = (unsigned short)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				int obj = (int)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteUInt) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				unsigned int obj = (unsigned int)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteLong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				long long obj = (long long)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteULong) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				unsigned long long obj = (unsigned long long)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteFloat) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				float obj = (float)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteDouble) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::Number, true)) return nullptr;

				double obj = (double)args->GetNumber(0);
				fwrite(&obj, 1, sizeof(obj), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteLine) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				const char* str = args->GetString(0);
				char obj[2]{'\r', '\n'};

				fwrite(str, 1, strlen(str), fs->fs_pfh);
				fwrite(&obj, 1, sizeof(obj), fs->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				const char* str = args->GetString(0);
				fwrite(str, 1, strlen(str), ((Scratch::CFileStream*)args->GetThisObjectData())->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteNullString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				const char* str = args->GetString(0);
				unsigned char obj = 0;

				fwrite(str, 1, strlen(str), fs->fs_pfh);
				fwrite(&obj, 1, sizeof(obj), fs->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteByteString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				const char* str = args->GetString(0);
				unsigned char obj = (unsigned char)strlen(str);

				fwrite(&obj, 1, sizeof(obj), fs->fs_pfh);
				fwrite(str, 1, strlen(str), fs->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteShortString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				const char* str = args->GetString(0);
				short obj = (short)strlen(str);

				fwrite(&obj, 1, sizeof(obj), fs->fs_pfh);
				fwrite(str, 1, strlen(str), fs->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(WriteIntString) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_IO_TYPE, true)) return nullptr;
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				Scratch::CFileStream* fs = (Scratch::CFileStream*)args->GetThisObjectData();

				const char* str = args->GetString(0);
				short obj = (short)strlen(str);

				fwrite(&obj, 1, sizeof(obj), fs->fs_pfh);
				fwrite(str, 1, strlen(str), fs->fs_pfh);

				return nullptr;
			}

			BS_FUNCTION(S_FileExists) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				struct stat s;
				int err = stat(args->GetString(0), &s);
				if (err == -1) {
					return Converter::ToVariable(bsi, false);
				}

				if (s.st_mode == S_IFDIR) {
					return Converter::ToVariable(bsi, false);
				} else {
					return Converter::ToVariable(bsi, true);
				}
			}

			BS_FUNCTION(S_DirExists) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				struct stat s;
				int err = stat(args->GetString(0), &s);
				if (err == -1) {
					return Converter::ToVariable(bsi, false);
				}

				if (s.st_mode == S_IFDIR) {
					return Converter::ToVariable(bsi, true);
				} else {
					return Converter::ToVariable(bsi, false);
				}
			}

			BS_FUNCTION(S_CreateDir) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;
				return Converter::ToVariable(bsi, mkdir(args->GetString(0)) == 0);
			}

			BS_FUNCTION(S_CreateFile) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;
				FILE* f = fopen(args->GetString(0), "wb");
				if (f != nullptr) fclose(f);

				return Converter::ToVariable(bsi, f != nullptr);
			}

			BS_FUNCTION(S_DeleteDir) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;
				return Converter::ToVariable(bsi, rmdir(args->GetString(0)) == 0);
			}

			BS_FUNCTION(S_DeleteFile) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;
				return Converter::ToVariable(bsi, remove(args->GetString(0)) == 0);
			}

			BS_FUNCTION(S_GetFilesInDir) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				DIR* d = opendir(args->GetString(0));
				if (d == nullptr) return nullptr;

				Table* t = new Table(bsi);
				int i = 0;

				struct dirent *ent;
				char kbuff[32]; // I'm pretty sure we're not going over 32 letters with dir listing
				while ((ent = readdir(d)) != NULL) {
					if (ent->d_type != DT_DIR) {
						sprintf(kbuff, "%d", i++);
						t->Set(kbuff, Converter::ToVariable(bsi, ent->d_name));
					}
				}

				closedir(d);

				return Converter::ToVariable(bsi, t);
			}

			BS_FUNCTION(S_GetDirsInDir) {
				if (!args->CheckType(0, VariableType::String, true)) return nullptr;

				DIR* d = opendir(args->GetString(0));
				if (d == nullptr) return nullptr;

				Table* t = new Table(bsi);
				int i = 0;

				struct dirent *ent;
				char kbuff[32]; // I'm pretty sure we're not going over 32 letters with dir listing
				while ((ent = readdir(d)) != NULL) {
					if (ent->d_type == DT_DIR) {
						sprintf(kbuff, "%d", i++);
						t->Set(kbuff, Converter::ToVariable(bsi, ent->d_name));
					}
				}

				closedir(d);

				return Converter::ToVariable(bsi, t);
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("IO", BROMSCRIPT_USERDATA_IO_TYPE, sizeof(Scratch::CFileStream), CTOR, DTOR);

				vd->RegisterFunction("Open", Open);
				vd->RegisterFunction("Close", Close);
				vd->RegisterFunction("ReadToEnd", ReadToEnd);
				vd->RegisterFunction("GetLength", GetLength);

				vd->RegisterFunction("GetPos", GetPos);
				vd->RegisterFunction("SetPos", SetPos);

				vd->RegisterFunction("ReadBool", ReadBool);
				vd->RegisterFunction("ReadByte", ReadByte);
				vd->RegisterFunction("ReadShort", ReadShort);
				vd->RegisterFunction("ReadUShort", ReadUShort);
				vd->RegisterFunction("ReadInt", ReadInt);
				vd->RegisterFunction("ReadUInt", ReadUInt);
				vd->RegisterFunction("ReadFloat", ReadFloat);
				vd->RegisterFunction("ReadLong", ReadLong);
				vd->RegisterFunction("ReadULong", ReadULong);
				vd->RegisterFunction("ReadDouble", ReadDouble);
				vd->RegisterFunction("ReadString", ReadString);
				vd->RegisterFunction("ReadNullString", ReadNullString);
				vd->RegisterFunction("ReadByteString", ReadByteString);
				vd->RegisterFunction("ReadShortString", ReadShortString);
				vd->RegisterFunction("ReadIntString", ReadIntString);
				vd->RegisterFunction("ReadLine", ReadLine);

				vd->RegisterFunction("WriteBool", WriteBool);
				vd->RegisterFunction("WriteByte", WriteByte);
				vd->RegisterFunction("WriteShort", WriteShort);
				vd->RegisterFunction("WriteShort", WriteUShort);
				vd->RegisterFunction("WriteInt", WriteInt);
				vd->RegisterFunction("WriteInt", WriteUInt);
				vd->RegisterFunction("WriteFloat", WriteFloat);
				vd->RegisterFunction("WriteLong", WriteLong);
				vd->RegisterFunction("WriteLong", WriteULong);
				vd->RegisterFunction("WriteDouble", WriteDouble);
				vd->RegisterFunction("WriteString", WriteString);
				vd->RegisterFunction("WriteNullString", WriteNullString);
				vd->RegisterFunction("WriteByteString", WriteByteString);
				vd->RegisterFunction("WriteShortString", WriteShortString);
				vd->RegisterFunction("WriteIntString", WriteIntString);
				vd->RegisterFunction("WriteLine", WriteLine);

				vd->RegisterFunctionStatic("FileExists", S_FileExists);
				vd->RegisterFunctionStatic("DirExists", S_DirExists);
				vd->RegisterFunctionStatic("CreateDir", S_CreateDir);
				vd->RegisterFunctionStatic("CreateFile", S_CreateFile);
				vd->RegisterFunctionStatic("DeleteDir", S_DeleteDir);
				vd->RegisterFunctionStatic("DeleteFile", S_DeleteFile);

				vd->RegisterFunctionStatic("GetDirsInDir", S_GetDirsInDir);
				vd->RegisterFunctionStatic("GetFilesInDir", S_GetFilesInDir);
			}
		}
	}
}
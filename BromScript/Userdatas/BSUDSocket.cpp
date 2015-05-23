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

#include "../Userdatas/BSUDSocket.h"
#include "../Userdatas/BSUDPacket.h"

#include "../SIF.h"
#include "../Managers/Instance.h"
#include "../Objects/BSEzSock.h"
#include "../Objects/BSPacket.h"

#ifndef _MSC_VER
#include <arpa/inet.h>
#endif

namespace BromScript{
	namespace Userdatas{
		namespace Socket{
			BS_FUNCTION_CTOR(CTOR) {
				return new EzSock();
			}

			BS_FUNCTION_DTOR(DTOR) {
				delete (EzSock*)data;
			}

			BS_FUNCTION(Connect) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::String, true))return null;
				if (!args->CheckType(1, VariableType::Number, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();

				if (!sock->create()) return Converter::ToVariable(bsi, false);
				if (!sock->connect(args->GetString(0), (unsigned short)args->GetNumber(1))) return Converter::ToVariable(bsi, false);

				return Converter::ToVariable(bsi, true);
			}

			BS_FUNCTION(Close) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				sock->close();

				return null;
			}

			BS_FUNCTION(Host) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Number, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();

				if (!sock->create()) return Converter::ToVariable(bsi, false);
				if (!sock->bind((unsigned short)args->GetNumber(0))) return Converter::ToVariable(bsi, false);
				if (!sock->listen()) return Converter::ToVariable(bsi, false);

				return Converter::ToVariable(bsi, true);
			}

			BS_FUNCTION(Send) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;
				if (!args->CheckType(0, BROMSCRIPT_USERDATA_PACKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				BromScript::Packet* p = (BromScript::Packet*)args->GetUserdata(0, BROMSCRIPT_USERDATA_PACKET_TYPE);

				if (args->CheckType(1, VariableType::Bool) && args->GetBool(1)) { // send int header
					p->Sock = sock;
					p->Send();
				} else {
					sock->SendRaw(p->OutBuffer, p->OutSize);
				}

				return null;
			}

			BS_FUNCTION(Receive) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();

				BromScript::Packet* p = new BromScript::Packet(sock);
				if (args->CheckType(0, VariableType::Bool) && args->GetBool(0)) { // receive int header
					int len = p->ReadInt();

					p->ReadBytes(len);
					p->InPos = 4;
				} else if (args->CheckType(0, VariableType::Number)) {
					p->ReadBytes((int)args->GetNumber(0));
					p->InPos = 0;
				} else {
					delete p;
					BS_THROW_ERROR(args, "Expected number or false");
					return null;
				}

				return bsi->CreateUserdata(BROMSCRIPT_USERDATA_PACKET_TYPE, p, true);
			}

			BS_FUNCTION(Accept) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				EzSock* client = new EzSock();

				if (!sock->accept(client)) {
					delete client;
					return null;
				}

				return bsi->CreateUserdata(BROMSCRIPT_USERDATA_SOCKET_TYPE, client, true);
			}

			BS_FUNCTION(HasPendingConnections) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				return Converter::ToVariable(bsi, sock->CanRead());
			}

			BS_FUNCTION(GetIP) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				return Converter::ToVariable(bsi, Scratch::CString(inet_ntoa(sock->addr.sin_addr)));
			}

			BS_FUNCTION(SetBlocking) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;
				if (!args->CheckType(0, VariableType::Bool, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				sock->blocking = args->GetBool(0);
				return null;
			}

			BS_FUNCTION(GetPort) {
				if (!args->CheckThisObject(BROMSCRIPT_USERDATA_SOCKET_TYPE, true))return null;

				EzSock* sock = (EzSock*)args->GetThisObjectData();
				return Converter::ToVariable(bsi, sock->addr.sin_port);
			}

			void RegisterUD(BromScript::Instance* bsi) {
				Userdata* vd = bsi->RegisterUserdata("Socket", BROMSCRIPT_USERDATA_SOCKET_TYPE, sizeof(EzSock), CTOR, DTOR);

				vd->RegisterFunction("Close", Close);
				vd->RegisterFunction("Connect", Connect);
				vd->RegisterFunction("Host", Host);
				vd->RegisterFunction("Send", Send);
				vd->RegisterFunction("Receive", Receive);
				vd->RegisterFunction("Accept", Accept);
				vd->RegisterFunction("GetIP", GetIP);
				vd->RegisterFunction("GetPort", GetPort);
				vd->RegisterFunction("SetBlocking", SetBlocking);
				vd->RegisterFunction("HasPendingConnections", HasPendingConnections);
			}
		}
	}
}
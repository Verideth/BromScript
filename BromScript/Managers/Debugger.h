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

#ifndef BROMSCRIPT_DEBUGGER_INCLUDED
#define BROMSCRIPT_DEBUGGER_INCLUDED

#include "../Managers/Instance.h"
#include "../Objects/BSPacket.h"
#include "../Objects/BSEzSock.h"
#include "../Scratch/Scratch.h"

namespace BromScript {
	struct Breakpoint {
	public:
		Scratch::CString File;
		int Linenumber;
	};

	class Debugger {
	public:
		Instance* BSI;
		bool Connected;
		bool Debugging;

		int BreakpointsCount;
		Breakpoint** Breakpoints;

		Debugger(Instance* bromscript);
		~Debugger();

		bool Connect();
		void Disconnect();
		void Break();
		void Print(const char* msg);
		void Error(CallStack* stack, int stacksize, const char* msg);
		void Update();
	private:
		EzSock Sock;
		bool AlwaysBreak;
		bool ReceivedBreakpoints;
		bool DoStep;
		bool DoStepOver;
		int StepOverLine;

		void HandlePackets();
		void PACKET_Breakpoints(Packet& p);
		void PACKET_Break(Packet& p);
		void PACKET_Resume(Packet& p);
		void PACKET_Exec(Packet& p);
		void PACKET_Step(Packet& p);
		void PACKET_StepOver(Packet& p);
	};
}

#endif
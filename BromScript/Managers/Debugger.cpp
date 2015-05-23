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

#include "Debugger.h"
#include "../Objects/CallStack.h"
#include "../Managers/Instance.h"
#include "../Objects/BSPacket.h"
#include "../Objects/Function.h"
#include "../Objects/UserdataInstance.h"
#include "../Objects/CompilerException.h"
#include <stddef.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace Scratch;

namespace BromScript {
	// 1: hello, BSI pointer
	// 2: breakpoints & files
	// 3: dobreak
	// 4: Error
	// 5: exec
	// 6: step
	// 7: stepover

	// 252: member offsets;
	// 253: Print
	// 254: Resume
	// 255: disconnect

	Debugger::Debugger(Instance* bsi) :BSI(bsi), Connected(false), Debugging(false), Breakpoints(null), AlwaysBreak(false), BreakpointsCount(0), ReceivedBreakpoints(false), DoStep(false), DoStepOver(false), StepOverLine(-1) {

	}

	Debugger::~Debugger(void) {
		this->Disconnect();
	}

	bool Debugger::Connect() {
		if (this->Connected) return true;

		this->Sock.create();

#ifdef _MSC_VER
		unsigned short pid = (unsigned short)GetCurrentProcessId();
#else
		unsigned short pid = (unsigned short)getpid();
#endif

		if (this->Sock.connect("127.0.0.1", pid) != 0) {
			this->Sock.close();
			return false;
		}

		Packet p(&this->Sock);

		p.WriteByte(252);
		p.WriteString("Function_Name"); p.WriteShort(offsetof(Function, Name));
		p.WriteString("Function_Filename"); p.WriteShort(offsetof(Function, Filename));
		p.WriteString("Function_Parent"); p.WriteShort(offsetof(Function, Parent));
		p.WriteString("Function_LocalKeys"); p.WriteShort(offsetof(Function, FixedLocalKeys));
		p.WriteString("Function_LocalVars"); p.WriteShort(offsetof(Function, FixedLocalVars));
		p.WriteString("Function_StringTable"); p.WriteShort(offsetof(Function, StringTable));
		p.WriteString("Function_Code"); p.WriteShort(offsetof(Function, Code));
		p.WriteString("Function_CurrentThisObject"); p.WriteShort(offsetof(Function, CurrentThisObject));
		p.WriteString("Function_CurrentSourceFileLine"); p.WriteShort(offsetof(Function, CurrentSourceFileLine));
		p.WriteString("Function_IsCpp"); p.WriteShort(offsetof(Function, IsCpp));
		p.WriteString("Function_CppFunc"); p.WriteShort(offsetof(Function, CppFunc));

		p.WriteString("Instance_CurrentIncludePath"); p.WriteShort(offsetof(Instance, CurrentIncludePath));
		p.WriteString("Instance_Globals"); p.WriteShort(offsetof(Instance, Globals));
		p.WriteString("Instance_CurrentStack"); p.WriteShort(offsetof(Instance, CurrentStack));
		p.WriteString("Instance_RegisteredUserdataTypes"); p.WriteShort(offsetof(Instance, RegisteredUserdataTypes));
		p.WriteString("Instance_KillScriptThreaded"); p.WriteShort(offsetof(Instance, KillScriptThreaded));
		p.WriteString("Instance_GC"); p.WriteShort(offsetof(Instance, GC));
		p.WriteString("Instance_Debug"); p.WriteShort((short)offsetof(Instance, Debug));

		p.WriteString("Userdata_CallDTor"); p.WriteShort(offsetof(Userdata, CallDTor));
		p.WriteString("Userdata_TypeSize"); p.WriteShort(offsetof(Userdata, TypeSize));
		p.WriteString("Userdata_TypeID"); p.WriteShort(offsetof(Userdata, TypeID));
		p.WriteString("Userdata_Offset"); p.WriteShort(offsetof(Userdata, Offset));
		p.WriteString("Userdata_Members"); p.WriteShort(offsetof(Userdata, Members));
		p.WriteString("Userdata_Name"); p.WriteShort(offsetof(Userdata, Name));

		p.WriteString("UserdataInstance_CallDTor"); p.WriteShort(offsetof(UserdataInstance, CallDTor));
		p.WriteString("UserdataInstance_TypeData"); p.WriteShort(offsetof(UserdataInstance, TypeData));

		p.WriteString("Variable_Value"); p.WriteShort(offsetof(Variable, Value));
		p.WriteString("Variable_Type"); p.WriteShort(offsetof(Variable, Type));

		p.WriteString("Table_Count"); p.WriteShort(offsetof(Table, Count));
		p.WriteString("Table_Keys"); p.WriteShort(8); // private, thus not accessble D:
		p.WriteString("Table_Values"); p.WriteShort(12); // private, thus not accessble D:
		p.WriteString("Table_Buffersize"); p.WriteShort(16); // private, thus not accessble D:

		p.WriteString("Callstack_Filename"); p.WriteShort(offsetof(CallStack, Filename));
		p.WriteString("Callstack_FunctionName"); p.WriteShort(offsetof(CallStack, Name));
		p.WriteString("Callstack_CurrentLine"); p.WriteShort(offsetof(CallStack, LineNumber));
		p.WriteString("Callstack_Function"); p.WriteShort(offsetof(CallStack, Func));

		p.Send();

		p.WriteByte(1);
		p.WriteLong((long long)this->BSI);
		p.Send();

		this->Connected = true;

		while (this->Connected && !this->ReceivedBreakpoints) {
			this->HandlePackets();
		}

		return true;
	}

	void Debugger::Disconnect() {
		if (!this->Connected) return;

		this->AlwaysBreak = false;
		this->Debugging = false;
		this->Connected = false;
		this->ReceivedBreakpoints = false;
		this->Sock.close();
	}

	void Debugger::Update() {
		if (!this->Connected) return;

		this->HandlePackets();

		if (this->AlwaysBreak)
			this->Break();

		if (this->DoStep) {
			this->DoStep = false;
			this->Break();
		}

		Function* curfunc = this->BSI->GetCurrentFunction();
		if (this->DoStepOver && curfunc->CurrentSourceFileLine > this->StepOverLine) {
			this->DoStepOver = false;
			this->StepOverLine = -1;

			this->Break();
		}

		if (!this->Debugging && this->BreakpointsCount > 0) {
			for (int i = 0; i < this->BreakpointsCount; i++) {
				Breakpoint* point = this->Breakpoints[i];
				if (point->Linenumber == curfunc->CurrentSourceFileLine && curfunc->Filename.EndsWith(point->File)) {
					this->Break();
				}
			}

		}

		while (this->Debugging || !this->ReceivedBreakpoints) {
#ifdef _MSC_VER
			Sleep(1);
#else
			sleep(1);
#endif

			this->HandlePackets();
		}
	}

	void Debugger::Print(const char* msg) {
		if (!this->Connected) return;

		Packet p(&this->Sock);
		p.WriteByte(253);
		p.WriteString(msg);
		p.Send();
	}

	void Debugger::Break() {
		if (!this->Connected) {
#ifdef _MSC_VER
			__asm{int 3};
#else
			__asm("int $3");
#endif

			return;
		}

		this->Debugging = true;

		Packet p(&this->Sock);
		p.WriteByte(3);
		p.Send();

		this->Update();
	}

	void Debugger::Error(CallStack* stack, int stacksize, const char* msg) {
		if (!this->Connected) return;

		this->Debugging = true;

		Packet p(&this->Sock);
		p.WriteByte(4);
		p.WriteString(msg);
		p.Send();

		this->Update();
	}

	void Debugger::HandlePackets() {
		while (this->Sock.CanRead()) {
			byte psizebuff[4];

			if (this->Sock.Receive(psizebuff, 4, 0) <= 0) {
				this->Disconnect();
				return;
			}

			int psize = *(int*)psizebuff;
			if (psize <= 0)
				continue;

			Packet p(&this->Sock);
			p.InBuffer = new byte[psize];
			p.InSize = psize;

			int cpos = 0;
			while (cpos != psize) {
				int cur = this->Sock.Receive(p.InBuffer, psize - cpos, cpos);
				if (cur == -1) {
					this->Disconnect();
					break;
				}

				cpos += cur;
			}

			byte pid = p.ReadByte();
			switch (pid) {
				case 2: this->PACKET_Breakpoints(p); break;
				case 3: this->PACKET_Break(p); break;
				case 5: this->PACKET_Exec(p); break;
				case 6: this->PACKET_Step(p); break;
				case 7: this->PACKET_StepOver(p); break;
				case 254: this->PACKET_Resume(p); break;
				case 255: this->Disconnect(); break;
			}
		}
	}

	CString errCatcher;
	BS_ERRORFUNCTION(errOverride) {
		errCatcher.AppendF("BromScript error!\n");

		for (int i = 0; i < stacksize; i++) {
			errCatcher.AppendF("SID: %i, File: %s, Name: %s, Line: %d\n", i, stack[i].Filename.str_szBuffer, stack[i].Name.str_szBuffer, stack[i].LineNumber);
		}

		errCatcher.AppendF("Message: %s\n", msg);
	}

	void Debugger::PACKET_Exec(Packet& p) {
		CString code = p.ReadString().Trim();
		Function* func = null;
		if (p.ReadBool()) // run inside of that function's locals.
			func = (Function*)p.ReadInt();

		bool isasign = false;
		if (!code.StartsWith("return")) {
			int bracketA = 0;
			int bracketB = 0;
			int bracketC = 0;
			bool quotes = false;

			int len = code.Size();
			for (int i = 0; i < len; i++) {
				switch (code[i]) {
					case '[': bracketA++; break;
					case '(': bracketB++; break;
					case '{': bracketC++; break;
					case ']': bracketA--; break;
					case ')': bracketB--; break;
					case '}': bracketC--; break;
					case '"':
						if (i > 0 && code[i - 1] == '\\')
							break;

						quotes = !quotes;
						break;

					case '=':
						if (bracketA == 0 && bracketB == 0 && bracketC == 0 && !quotes) {
							isasign = true;
						}
						break;
				}
			}

			if (!isasign)
				code = CString::Format("return %s", code.str_szBuffer);
		}

		CString ret;

		byte* btcode = null;
		int size = code.Size();
		try {
			btcode = Compiler::Run("D_EXEC", code, size, &size);
		} catch (CompilerException err) {
			ret = err.Message;
		}

		if (ret.Size() > 0) {
			p.WriteByte(5);
			p.WriteString(ret);
			p.Send();
			return;
		}

		bool olddebug = this->Debugging;
		this->Debugging = false;
		this->Connected = false;
		ErrorFunction olderrfunc = this->BSI->GetErrorCallback();
		this->BSI->SetErrorCallback(errOverride);

		int stacksize = this->BSI->GetCurrentStackSize();
		bool* oldreturns = new bool[stacksize];
		for (int i = 0; i < stacksize; i++) {
			oldreturns[i] = this->BSI->CurrentStack[i].Func->ForceReturn;
		}

		if (func == null) {
			Variable* var = this->BSI->DoCode("D_EXEC", btcode, size);
			ret = var == null ? "NULL" : var->ToString();
			delete[] btcode;
		} else {
			Function tmp(this->BSI);
			tmp.Code = btcode;
			tmp.Parent = func;
			tmp.Filename = "D_EXEC";

			Variable* var = tmp.Run();
			ret = var == null ? "NULL" : var->ToString(this->BSI);

			delete[] tmp.StringTable;
			tmp.StringTable = null;
		}

		for (int i = 0; i < stacksize; i++) {
			this->BSI->CurrentStack[i].Func->ForceReturn = oldreturns[i];
		}

		this->BSI->SetErrorCallback(olderrfunc);
		this->Debugging = olddebug;
		this->Connected = true;

		if (errCatcher.Size() > 0) {
			p.WriteByte(5);
			p.WriteString(errCatcher);
			p.Send();
		} else {
			p.WriteByte(5);
			p.WriteString(ret);
			p.Send();
		}

		errCatcher = "";
	}

	void Debugger::PACKET_Break(Packet& p) {
		this->Debugging = true;

		p.WriteByte(3);
		p.Send();
	}

	void Debugger::PACKET_Breakpoints(Packet& p) {
		this->AlwaysBreak = p.ReadBool();

		if (this->Breakpoints != null) {
			for (int i = 0; i < this->BreakpointsCount; i++)
				delete this->Breakpoints[i];

			free(this->Breakpoints);
			this->Breakpoints = null;
		}

		this->BreakpointsCount = p.ReadInt();
		if (this->BreakpointsCount > 0) {
			this->Breakpoints = (Breakpoint**)malloc(sizeof(Breakpoint*) * this->BreakpointsCount);

			for (int i = 0; i < this->BreakpointsCount; i++) {
				this->Breakpoints[i] = new Breakpoint();
				this->Breakpoints[i]->File = p.ReadString();
				this->Breakpoints[i]->Linenumber = p.ReadInt();
			}
		}

		this->ReceivedBreakpoints = true;
	}

	void Debugger::PACKET_Resume(Packet& p) {
		this->Debugging = false;
	}

	void Debugger::PACKET_StepOver(Packet& p) {
		Function* curfunc = this->BSI->GetCurrentFunction();

		this->Debugging = false;
		this->DoStepOver = true;
		this->StepOverLine = curfunc->CurrentSourceFileLine;
	}

	void Debugger::PACKET_Step(Packet& p) {
		this->Debugging = false;
		this->DoStep = true;
	}
}

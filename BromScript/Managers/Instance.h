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

#ifndef BROMSCRIPT_INSTANCE_INCLUDED
#define BROMSCRIPT_INSTANCE_INCLUDED

#include "../SIF.h"
#include "../Objects/Variable.h"
#include "../Objects/CallStack.h"
#include "../Objects/Function.h"
#include "../Objects/ByteReader.h"
#include "../Objects/Table.h"
#include "../Objects/List.h"
#include "../Objects/Userdata.h"
#include "../Objects/UserdataInstance.h"
#include "../Managers/Compiler.h"
#include "../Managers/GarbageCollector.h"
#include "../Managers/Debugger.h"

namespace BromScript {
	class Instance {
	public:
		Instance();
		~Instance();

		Scratch::CString CurrentIncludePath;
		Table* Globals;
		List<CallStack*> CurrentStack;
		List<Userdata*> RegisteredUserdataTypes;
		bool KillScriptThreaded;

		GarbageCollector GC;
		Debugger* Debug;

		void LoadDefaultLibaries();
		Variable* DoString(const char* chunkname, const char* code);
		Variable* DoString(const char* chunkname, const char* code, int size);
		Variable* DoFile(const char* filename);
		Variable* DoCode(const char* filename, byte* code, int codelen);


#ifdef _MSC_VER
		void DoDLL(const char* filename);
#else
		void DoSO(const char* filename);
#endif

		void EnterStack(Function* func);
		void LeaveStack();

		void Error(Scratch::CString msg);
		void Error(Scratch::CString msg, int linenumber, const char* file);

		void SetErrorCallback(ErrorFunction func);
		ErrorFunction GetErrorCallback();
		void RegisterFunction(Scratch::CString key, BSFunction func, int linenumber = -1, const char* file = "c++");
		void RegisterScope(Function* scope);

		Userdata* GetRegisteredUserdata(int typeID);
		Userdata* RegisterUserdata(Scratch::CString name, int typeID, int typesize, BSFunctionCtor ctor, BSFunctionDtor dtor);
		Variable* CreateUserdata(int typeID, void* ptr, bool calldtor);

		void RemoveVar(Scratch::CString key);
		void SetVar(Scratch::CString key, Variable* var);
		Variable* GetVar(Scratch::CString key);

		Function* GetCurrentFunction();
		CallStack* GetCurrentStack();
		int GetCurrentStackSize();

		Variable* GetDefaultVarTrue();
		Variable* GetDefaultVarFalse();
		Variable* GetDefaultVarNull();

	private:
		List<Function*> ChunkScopes;
		List<BSModuleFunction> ModuleExits;

		bool SuppressErrors;
		bool IncludingInternalUserdata;

		int CurrentStackIndex;

		ErrorFunction ErrorCallback;

		Variable* _Default_Var_True;
		Variable* _Default_Var_False;
		Variable* _Default_Var_Null;
	};
}

#endif
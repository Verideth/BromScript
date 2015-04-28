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

#ifndef BROMSCRIPT_SIF_INCLUDED
#define BROMSCRIPT_SIF_INCLUDED

/////////////////////////////////////////////////
// config

// uncomment if you want to enable reference debugging, use the BS_REF_INCREESE and BS_REF_DECREESE macros for that.
// this is VERY slow.
// #define BS_DEBUG_REFCOUNT

// bytecode version, should only be changed when binary format changes
#define BROMSCRIPT_CURRENTVERSION 4
#define BS_POOL_SIZE 4098
#define BS_CALLSTACK_SIZE 1024

//////////////////////////////////////////////////

#ifndef SCRATCH_NO_GLOBFUNC
#define SCRATCH_NO_GLOBFUNC
#endif

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "Scratch/Scratch.h"
#include "Objects/List.h"

#define BS_FUNCTION(fn) BromScript::Variable* fn(BromScript::Instance* bsi, BromScript::ArgumentData* args)
#define BS_FUNCTION_CTOR(fn) void* fn(BromScript::Instance* bsi, BromScript::ArgumentData* args)
#define BS_FUNCTION_DTOR(fn) void fn(BromScript::Instance* bsi, void* data)

#define BS_MODULE(fn) void fn(BromScript::Instance* bsi)
#define BS_ERRORFUNCTION(fn) void fn(BromScript::Instance* bsi, BromScript::CallStack* stack, int stacksize, const char* msg)

#define BS_USERDATA_GET(fn) BromScript::Variable* fn(BromScript::Instance* bsi, void* ptr)
#define BS_USERDATA_SET(fn) void fn(BromScript::Instance* bsi, void* ptr, BromScript::Variable* var)

#define BS_THROW_ERROR(bsi_or_args, msg) (bsi_or_args)->Error((msg), __LINE__, __FILE__)
#define BS_ARITHMATICOP_TOFUNCINDEX(op) (int)op - (int)Operators::Arithmetic_START - 1

#ifndef null
#define null 0
#endif

#define BS_PI 3.14159265359

#ifdef BS_DEBUG_REFCOUNT
#define BS_REF_INCREESE(var) (var)->IncreeseRefCount(__LINE__, __FILE__)
#define BS_REF_DECREESE(var) (var)->DecreeseRefCount(__LINE__, __FILE__)
#else
#define BS_REF_INCREESE(var) (var)->IncreeseRefCount()
#define BS_REF_DECREESE(var) (var)->DecreeseRefCount()
#endif

namespace BromScript {
	class ByteReader;
	class ByteWriter;
	class CallStack;
	class Function;
	class Instance;
	class Debugger;
	class ExecuteData;
	class Executer;
	class Variable;
	class ArgumentData;
	class GarbageCollector;
	class Table;
	class MemberData;
	class Userdata;
	class UserdataInstance;
	class CompilerException;
	class RuntimeException;

	typedef unsigned char byte;
	typedef Variable* (*BSFunction)(Instance* bsi, ArgumentData* args);
	typedef void* (*BSFunctionCtor)(Instance* bsi, ArgumentData* args);
	typedef void(*BSFunctionDtor)(Instance* bsi, void* data);
	typedef void(*ErrorFunction)(Instance* bsi, CallStack* stack, int stacksize, const char* name);
	typedef void(*BSModuleFunction)(Instance*);
	typedef Variable* (*BSExecFuncion)(ExecuteData* data);

	typedef Variable* (*BSGetFunction)(BromScript::Instance* bsi, void* ptr);
	typedef void(*BSSetFunction)(BromScript::Instance* bsi, void* ptr, Variable* var);
}

#endif
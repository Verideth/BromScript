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

#include "../Managers/Instance.h"
#include "../Objects/Environment.h"
#include "../Objects/CompilerException.h"
#include "../Objects/RuntimeException.h"
#include "../Libaries.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace Scratch;

namespace BromScript {
	Instance::Instance() :CurrentIncludePath(""), CurrentStackIndex(0), Globals(new Table(this)), KillScriptThreaded(false), Debug(new Debugger(this)), ErrorCallback(nullptr), IncludingInternalUserdata(false) {
		this->_Default_Var_Null = this->GC.GetPooledVariable();
		this->_Default_Var_True = this->GC.RegisterVariable(Converter::ToVariable(this, true));
		this->_Default_Var_False = this->GC.RegisterVariable(Converter::ToVariable(this, false));

		BS_REF_INCREESE(this->_Default_Var_Null);
		BS_REF_INCREESE(this->_Default_Var_True);
		BS_REF_INCREESE(this->_Default_Var_False);
	}

	Instance::~Instance() {
		while (this->ModuleExits.Count > 0)
			this->ModuleExits.RemoveAt(0)(this);

		while (this->ChunkScopes.Count > 0) {
			Function* rootfunc = this->ChunkScopes.RemoveAt(0);

			if (rootfunc->StringTable != nullptr) delete[] rootfunc->StringTable;

			if (rootfunc->StringTableVars != nullptr) {
				for (int i = 0; i < rootfunc->StringTableCount; i++) {
					BS_REF_DECREESE(rootfunc->StringTableVars[i]);
				}

				delete[] rootfunc->StringTableVars;
			}
			
			delete rootfunc;
		}

		delete this->Globals;


		BS_REF_DECREESE(this->_Default_Var_Null);
		BS_REF_DECREESE(this->_Default_Var_True);
		BS_REF_DECREESE(this->_Default_Var_False);

		for (int i = 0; i < this->RegisteredUserdataTypes.Count; i++) {
			Userdata* ud = this->RegisteredUserdataTypes[i];

			delete ud->Statics;
			ud->Statics = nullptr;
		}

		this->GC.SelfDestruct();

		while (this->RegisteredUserdataTypes.Count > 0) {
			delete this->RegisteredUserdataTypes.RemoveAt(0);
		}

		delete this->Debug;
	}

	void Instance::SetVar(const Scratch::CString& key, Variable* var) {
		if (var == nullptr || var->Type == VariableType::Null) {
			this->Globals->Remove(key);
			return;
		}

		this->Globals->Set(key, var);
	}

	void Instance::RemoveVar(const Scratch::CString& key) {
		this->Globals->Remove(key);
	}

	Variable* Instance::GetVar(const Scratch::CString& key) {
		return this->Globals->Get(key);
	}

	Variable* Instance::DoString(const char* codename, const char* code) {
		int i = 0;
		while (true) {
			if (code[i] == 0) {
				return this->DoString(codename, code, i);
			}

			i++;
		}
	}

	Variable* Instance::DoString(const char* codename, const char* code, int size) {
		byte* bytecode = Compiler::Run(codename, code, size, &size);
		Variable* ret = this->DoCode(codename, bytecode, size);
		delete[] bytecode;

		return ret;
	}

	void Instance::EnterStack(Function* func) {
		CallStack* cs = &this->CurrentStack[this->CurrentStackIndex];
		cs->Func = func;
		cs->Name = func->Name;
		cs->LineNumber = func->CurrentSourceFileLine;
		cs->Filename = func->Filename;

		this->CurrentStackIndex++;
	}

	void Instance::LeaveStack() {
		this->CurrentStackIndex--;
		
		if (this->CurrentStackIndex == 0) {
			this->KillScriptThreaded = false;
		}
	}

	int Instance::GetCurrentStackSize() {
		return this->CurrentStackIndex;
	}

	Variable* Instance::GetDefaultVarNull() { return this->_Default_Var_Null; }
	Variable* Instance::GetDefaultVarTrue() { return this->_Default_Var_True; }
	Variable* Instance::GetDefaultVarFalse() { return this->_Default_Var_False; }

	Variable* Instance::ToVariable(bool value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(double value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(int value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(long long value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(float value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(const Scratch::CString &value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(const char* value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(Table* value) { return Converter::ToVariable(this, value); }
	Variable* Instance::ToVariable(const Scratch::CString &key, BSFunction value) { return Converter::ToVariable(this, key, value); }
	Variable* Instance::ToVariable(const char* key, BSFunction value) { return Converter::ToVariable(this, key, value); }

#ifdef _MSC_VER
	void Instance::DoDLL(const char* filename) {
		CString file(filename);
		if (file.Size() < 2 || file[1] != ':')
			file = this->CurrentIncludePath + filename;

		HINSTANCE dll = LoadLibrary(file);
		if (dll == nullptr) {
			BS_THROW_ERROR(this, CString::Format("Could not find module '%s'", file.str_szBuffer));
			return;
		}

		BSModuleFunction init = (BSModuleFunction)GetProcAddress(dll, "BromScriptStart");
		BSModuleFunction deinit = (BSModuleFunction)GetProcAddress(dll, "BromScriptShutdown");
		if (init == nullptr) {
			BS_THROW_ERROR(this, CString::Format("Could not find entry point ('BromScriptStart') in module '%s'", file.str_szBuffer));
			return;
		}

		if (deinit == nullptr) {
			BS_THROW_ERROR(this, CString::Format("Could not find exit point ('BromScriptShutdown') in module '%s'", file.str_szBuffer));
			return;
		}

		init(this);
		this->ModuleExits.Add(deinit);
	}
#else
	void Instance::DoSO(const char* filename){
		CString file(filename);
		if (file.Size() < 1 || file[0] != '/')
			file = this->CurrentIncludePath + filename;

		void* sohandle = dlopen(file, RTLD_LAZY);
		if (sohandle == nullptr){
			BS_THROW_ERROR(this, CString::Format("Could not find module '%s'", file.str_szBuffer));
			return;
		}

		BSModuleFunction init = (BSModuleFunction)dlsym(sohandle, "BromScriptStart");
		BSModuleFunction deinit = (BSModuleFunction)dlsym(sohandle, "BromScriptShutdown");

		if (init == nullptr){
			BS_THROW_ERROR(this, CString::Format("Could not find entry point ('BromScriptStart') in module '%s'", file.str_szBuffer));
			return;
		}

		if (deinit == nullptr){
			BS_THROW_ERROR(this, CString::Format("Could not find exit point ('BromScriptShutdown') in module '%s'", file.str_szBuffer));
			return;
		}

		//dlclose(sohandle); // not sure if this is okay...... we don't want to completly unload the entire dll at the end, however, unlike windows, we should unload libaries.......
		// TODO: make loaded libary object so that we can properly unload in UNIX and not load libaires more than once.

		init(this);
		this->ModuleExits.Add(deinit);
	}
#endif

	Variable* Instance::DoFile(const char* filename, Environment* env, bool useCurrentIncludePath) {
		CString oldincp = this->CurrentIncludePath;
		CString file(filename);
		file = file.Replace('\\', '/');

		if (useCurrentIncludePath) {
			if (file.Size() > 2 && file[1] != ':' && file[0] != ':')
				file = this->CurrentIncludePath + filename;

			if (file.Contains('/'))
				this->CurrentIncludePath += file.Substring(0, file.LastIndexOf('/') + 1);

		}
		CFileStream fs;
		if (!fs.Open(file, "rb")) {
			CString str = CString::Format("Cannot open file '%s'", filename);
			this->CurrentIncludePath = oldincp;

			throw CompilerException(str, filename, -1);
		}

		int filesize = fs.Size();
		byte* buff = (byte*)fs.Read(filesize);
		fs.Close();

		if (filesize == 0) {
			return this->GetDefaultVarNull();
		}

		if (filesize > 13) {
			// \0 BROMSCRIPT \0 VERSION
			bool iscompiled = true;
			byte bytecheck[] = {0, 66, 82, 79, 77, 83, 67, 82, 73, 80, 84, 0};
			for (int i = 0; i < 12; i++) { // tag is 13, but the last one is version so go to 12, we'll handle the version later.
				if (bytecheck[i] != buff[i]) {
					iscompiled = false;
					break;
				}
			}

			if (iscompiled) {
				if (buff[12] != BROMSCRIPT_CURRENTVERSION) {
					this->CurrentIncludePath = oldincp;

					delete buff;
					throw RuntimeException(CString::Format("Cannot execute '%s', invalid CBS version.", filename));
				}

				for (int i = 0; i < 13; i++) {
					buff[i] = (byte)Misc::ExecFuncs::Skip;
				}

				Variable* ret = nullptr;
				try {
					ret = this->DoCode(filename, buff, filesize);
				} catch (RuntimeException err) {
					this->CurrentIncludePath = oldincp;
					delete[] buff;

					throw err;
				}

				delete[] buff;
				this->CurrentIncludePath = oldincp;

				return ret;
			}
		}

		byte* bytecode = nullptr;

		try {
			bytecode = Compiler::Run(filename, (char*)buff, filesize, &filesize);
		} catch (CompilerException err) {
			delete[] buff;
			this->CurrentIncludePath = oldincp;
			throw err;
		}

		delete[] buff;

		Variable* ret = nullptr;
		try {
			ret = this->DoCode(filename, bytecode, filesize, env);
		} catch (RuntimeException err) {
			delete[] bytecode;
			this->CurrentIncludePath = oldincp;
			throw err;
		}

		delete[] bytecode;

		this->CurrentIncludePath = oldincp;
		return ret;
	}

	Variable* Instance::DoCode(const char* filename, byte* code, int codelen, Environment* env) {
		byte* codecopy;

		if (codelen > 13) {
			// \0 BROMSCRIPT \0 VERSION
			bool iscompiled = true;
			byte bytecheck[] = {0, 66, 82, 79, 77, 83, 67, 82, 73, 80, 84, 0};
			for (int i = 0; i < 12; i++) { // tag is 13, but the last one is version so go to 12, we'll handle the version later.
				if (bytecheck[i] != code[i]) {
					iscompiled = false;
					break;
				}

				if (i == 11) { // last loop
					if (code[12] != BROMSCRIPT_CURRENTVERSION) {
						delete code;
						throw RuntimeException(CString::Format("Cannot execute '%s', invalid CBS version.", filename));
					}
				}
			}

			codecopy = new byte[codelen];
			memcpy(codecopy, code, codelen);

			if (iscompiled) {
				for (int i = 0; i < 13; i++) {
					codecopy[i] = (byte)Misc::ExecFuncs::Skip;
				}
			}
		} else {
			codecopy = new byte[codelen];
			memcpy(codecopy, code, codelen);
		}

		if (env == nullptr) {
			env = new Environment(this);
		}

		Function* f = new Function(this);
		f->Code = codecopy;
		f->CodeLength = codelen;
		f->Name = "BS::Entrypoint";
		f->Filename = filename;
		f->SetEnv(env);
		this->ChunkScopes.Add(f);

		Variable* ret = f->Run();

		return ret;
	}

	Function* Instance::GetCurrentFunction() {
		if (this->CurrentStackIndex == 0) return nullptr;
		return this->CurrentStack[this->CurrentStackIndex - 1].Func;
	}

	CallStack* Instance::GetCurrentStack() {
		CallStack* stack = new CallStack[this->CurrentStackIndex];
		for (int i = 0; i < this->CurrentStackIndex; i++)
			stack[i] = this->CurrentStack[i];

		return stack;
	}

	void Instance::Error(const Scratch::CString& msg) {
		this->Error(msg, -1, nullptr);
	}

	void Instance::Error(const Scratch::CString& msg, int linenumber, const char* file) {
		int lastiserror = false;
		for (int i = 0; i < this->CurrentStackIndex; i++) {
			CallStack* s = &this->CurrentStack[i];

			s->Func->ForceReturn = true;
			s->LineNumber = s->Func->CurrentSourceFileLine;

			if (i + 1 == this->CurrentStackIndex && s->Func->IsCpp && linenumber > -1) {
				s->LineNumber = linenumber;

				int spos = 0;
				for (int i2 = strlen(file); i2 > 0; i2--) {
					if (file[i2] == '/' || file[i2] == '\\') {
						spos = i2 + 1;
						break;
					}
				}

				s->Filename = file + spos;
			}

			if (i + 1 == this->CurrentStackIndex && s->Name == "error") {
				lastiserror = true;
			}
		}

		if (this->Debug->Connected) {
			this->Debug->Error(this->CurrentStack, this->CurrentStackIndex, msg);
		}

		if (this->ErrorCallback != nullptr) {
			this->ErrorCallback(this, this->CurrentStack, this->CurrentStackIndex - (lastiserror ? 1 : 0), msg);
		}

		//throw RuntimeException(msg);
	}

	void Instance::RegisterScope(Function* func) {
		this->ChunkScopes.Add(func);
	}

	void Instance::SetErrorCallback(ErrorFunction func) {
		this->ErrorCallback = func;
	}

	ErrorFunction Instance::GetErrorCallback() {
		return this->ErrorCallback;
	}

	void Instance::RegisterFunction(const Scratch::CString& key, BSFunction func, int linenumber, const char* file) {
		Function* varfunc = new Function(this);
		varfunc->CppFunc = func;
		varfunc->Filename = file;
		varfunc->IsCpp = true;
		varfunc->Name = key;
		varfunc->CurrentSourceFileLine = linenumber;

		Variable* var = this->GC.GetPooledVariable();
		var->Type = VariableType::Function;
		var->Value = varfunc;

		this->Globals->Set(key, var);
	}

	Variable* Instance::CreateUserdata(int typeID, void* ptr, bool calldtor) {
		Userdata* ud = this->GetRegisteredUserdata(typeID);
		if (ud == nullptr)
			throw "Unknown Userdata type!";

		UserdataInstance* udi = new UserdataInstance();
		udi->TypeData = ud;
		udi->Ptr = ptr;

		Variable* var = this->GC.GetPooledVariable();
		var->Value = udi;
		var->Type = (VariableType::Enum)typeID;

		return var;
	}

	Variable* Instance::CreateUserdata(const Scratch::CString& name, void* ptr, bool calldtor) {
		Userdata* ud = this->GetRegisteredUserdata(name);
		if (ud == nullptr)
			throw "Unknown Userdata type!";

		UserdataInstance* udi = new UserdataInstance();
		udi->TypeData = ud;
		udi->Ptr = ptr;

		Variable* var = this->GC.GetPooledVariable();
		var->Value = udi;
		var->Type = (VariableType::Enum)ud->TypeID;

		return var;
	}

	Userdata* Instance::GetRegisteredUserdata(int typeID) {
		for (int i = 0; i < this->RegisteredUserdataTypes.Count; i++) {
			if (this->RegisteredUserdataTypes[i]->TypeID == typeID) {
				return this->RegisteredUserdataTypes[i];
			}
		}

		return nullptr;
	}

	Userdata* Instance::GetRegisteredUserdata(const Scratch::CString& name) {
		for (int i = 0; i < this->RegisteredUserdataTypes.Count; i++) {
			if (this->RegisteredUserdataTypes[i]->Name == name) {
				return this->RegisteredUserdataTypes[i];
			}
		}

		return nullptr;
	}

	// use typeID 0 to make it automaticly get a typeID asigned to it
	Userdata* Instance::RegisterUserdata(const Scratch::CString& name, int typeID, int typesize, BSFunctionCtor ctor, BSFunctionDtor dtor) {
		static int s_HighestID = 70000; // random high number to prevent collision with other types

		// just use the next ID
		if (typeID == 0) {
			typeID = s_HighestID + 1;
		}

		if (typeID < 100 && !this->IncludingInternalUserdata)
			throw "Cannot register Userdata with types below 100! (reserved for internal types)";

		for (int i = 0; i < this->RegisteredUserdataTypes.Count; i++) {
			if (this->RegisteredUserdataTypes[i]->TypeID == typeID) {
				CString str = CString::Format("Userdata typeID '%d' already registered under the name '%s'", typeID, this->RegisteredUserdataTypes[i]->Name.str_szBuffer);
				throw str.str_szBuffer;
			}
		}

		if (s_HighestID < typeID) {
			s_HighestID = typeID;
		}

		Userdata* ud = new Userdata(this);
		ud->Name = name;
		ud->Ctor = ctor;
		ud->Dtor = dtor;
		ud->TypeID = typeID;
		ud->TypeSize = typesize;

		this->RegisteredUserdataTypes.Add(ud);

		Function* func = new Function(this);
		func->Name = name;
		func->IsCpp = true;

		Variable* classvar = this->GC.GetPooledVariable();
		classvar->Type = VariableType::Class;
		classvar->Value = func;

		this->SetVar(name, classvar);

		return ud;
	}

	void Instance::LoadDefaultLibaries() {
		this->RegisterFunction("setReadOnly", BromScript::Libaries::Global::SetReadOnly);
		this->RegisterFunction("tostring", BromScript::Libaries::Global::ToString);
		this->RegisterFunction("tonumber", BromScript::Libaries::Global::ToNumber);
		this->RegisterFunction("print", BromScript::Libaries::Global::Print);
		this->RegisterFunction("call", BromScript::Libaries::Global::Call);
		this->RegisterFunction("typeof", BromScript::Libaries::Global::TypeOf);
		this->RegisterFunction("time", BromScript::Libaries::Global::Time);
		this->RegisterFunction("include", BromScript::Libaries::Global::Include);
		this->RegisterFunction("error", BromScript::Libaries::Global::Error);
		this->RegisterFunction("sleep", BromScript::Libaries::Global::Sleep);
		this->RegisterFunction("environment", BromScript::Libaries::Global::Environment);
#ifdef _MSC_VER
		this->RegisterFunction("includeDll", BromScript::Libaries::Global::IncludeDll);
#else
		this->RegisterFunction("includeDLL", BromScript::Libaries::Global::IncludeSO);
#endif

		Table* gc = new Table(this);
		gc->Set("Run", Converter::ToVariable(this, "Run", BromScript::GarbageCollector::RunWrapper));
		this->SetVar("GC", Converter::ToVariable(this, gc));

		Table* math = new Table(this);
		math->Set("Random", Converter::ToVariable(this, "Random", BromScript::Libaries::Math::Random));
		math->Set("Floor", Converter::ToVariable(this, "Floor", BromScript::Libaries::Math::Floor));
		math->Set("Min", Converter::ToVariable(this, "Min", BromScript::Libaries::Math::Min));
		math->Set("Max", Converter::ToVariable(this, "Max", BromScript::Libaries::Math::Max));
		math->Set("Ceiling", Converter::ToVariable(this, "Ceiling", BromScript::Libaries::Math::Ceiling));
		math->Set("Round", Converter::ToVariable(this, "Round", BromScript::Libaries::Math::Round));
		math->Set("Atan", Converter::ToVariable(this, "Atan", BromScript::Libaries::Math::Atan));
		math->Set("Atan2", Converter::ToVariable(this, "Atan2", BromScript::Libaries::Math::Atan2));
		math->Set("Cos", Converter::ToVariable(this, "Cos", BromScript::Libaries::Math::Cos));
		math->Set("Sin", Converter::ToVariable(this, "Sin", BromScript::Libaries::Math::Sin));
		math->Set("Tan", Converter::ToVariable(this, "Tan", BromScript::Libaries::Math::Tan));
		math->Set("Pow", Converter::ToVariable(this, "Pow", BromScript::Libaries::Math::Pow));
		math->Set("Sign", Converter::ToVariable(this, "Sign", BromScript::Libaries::Math::Sign));
		math->Set("Sqrt", Converter::ToVariable(this, "Sqrt", BromScript::Libaries::Math::Sqrt));
		math->Set("Abs", Converter::ToVariable(this, "Abs", BromScript::Libaries::Math::Abs));
		math->Set("Neg", Converter::ToVariable(this, "Neg", BromScript::Libaries::Math::Neg));
		math->Set("ToRadians", Converter::ToVariable(this, "ToRadians", BromScript::Libaries::Math::ToRadians));
		math->Set("ToDegrees", Converter::ToVariable(this, "ToDegrees", BromScript::Libaries::Math::ToDegrees));
		math->Set("Pi", Converter::ToVariable(this, BS_PI));
		this->SetVar("Math", Converter::ToVariable(this, math));

		Table* console = new Table(this);
		Table* consolecolors = new Table(this);
		console->Set("SetForeColor", Converter::ToVariable(this, "SetForeColor", BromScript::Libaries::Console::SetForeColor));
		console->Set("SetBackColor", Converter::ToVariable(this, "SetBackColor", BromScript::Libaries::Console::SetBackColor));
		console->Set("SetDefaultColors", Converter::ToVariable(this, "SetDefaultColors", BromScript::Libaries::Console::SetDefaultColors));
		console->Set("GetLine", Converter::ToVariable(this, "GetLine", BromScript::Libaries::Console::GetLine));
		console->Set("Clear", Converter::ToVariable(this, "Clear", BromScript::Libaries::Console::Clear));

		consolecolors->Set("Red", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorRed));
		consolecolors->Set("Black", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorBlack));
		consolecolors->Set("Blue", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorBlue));
		consolecolors->Set("Cyan", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorCyan));
		consolecolors->Set("Green", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorGreen));
		consolecolors->Set("Yellow", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorYellow));
		consolecolors->Set("Magenta", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorMagenta));
		consolecolors->Set("Gray", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorGray));
		consolecolors->Set("White", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorWhite));
#ifdef _MSC_VER
		consolecolors->Set("DarkBlue", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkBlue));
		consolecolors->Set("DarkCyan", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkCyan));
		consolecolors->Set("DarkGreen", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkGreen));
		consolecolors->Set("DarkMagenta", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkMagenta));
		consolecolors->Set("DarkRed", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkRed));
		consolecolors->Set("DarkWhite", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkWhite));
		consolecolors->Set("DarkYellow", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkYellow));
#else
		consolecolors->Set("DarkGray", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorDarkGray));
		consolecolors->Set("LightRed", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightRed));
		consolecolors->Set("LightGreen", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightGreen));
		consolecolors->Set("LightYellow", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightYellow));
		consolecolors->Set("LightBlue", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightBlue));
		consolecolors->Set("LightMgenta", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightMgenta));
		consolecolors->Set("LightCyan", Converter::ToVariable(this, (double)BromScript::Libaries::Console::ColorLightCyan));
#endif

		console->Set("Colors", Converter::ToVariable(this, consolecolors));
		this->SetVar("Console", Converter::ToVariable(this, console));

		Table* string = new Table(this);
		string->Set("ToChar", Converter::ToVariable(this, "ToChar", BromScript::Libaries::String::ToChar));
		string->Set("FromChar", Converter::ToVariable(this, "FromChar", BromScript::Libaries::String::FromChar));
		string->Set("Merge", Converter::ToVariable(this, "Merge", BromScript::Libaries::String::Merge));
		string->Set("Split", Converter::ToVariable(this, "Split", BromScript::Libaries::String::Split));
		string->Set("Sub", Converter::ToVariable(this, "Sub", BromScript::Libaries::String::Sub));
		string->Set("IndexOf", Converter::ToVariable(this, "IndexOf", BromScript::Libaries::String::IndexOf));
		string->Set("LastIndexOf", Converter::ToVariable(this, "LastIndexOf", BromScript::Libaries::String::LastIndexOf));
		string->Set("Left", Converter::ToVariable(this, "Left", BromScript::Libaries::String::Left));
		string->Set("Right", Converter::ToVariable(this, "Right", BromScript::Libaries::String::Right));
		string->Set("Trim", Converter::ToVariable(this, "Trim", BromScript::Libaries::String::Trim));
		string->Set("TrimLeft", Converter::ToVariable(this, "TrimLeft", BromScript::Libaries::String::TrimLeft));
		string->Set("TrimRight", Converter::ToVariable(this, "TrimRight", BromScript::Libaries::String::TrimRight));
		string->Set("Replace", Converter::ToVariable(this, "Replace", BromScript::Libaries::String::Replace));
		string->Set("ToLower", Converter::ToVariable(this, "ToLower", BromScript::Libaries::String::ToLower));
		string->Set("ToUpper", Converter::ToVariable(this, "ToUpper", BromScript::Libaries::String::ToUpper));
		this->SetVar("String", Converter::ToVariable(this, string));

		Table* debug = new Table(this);
		debug->Set("Print", Converter::ToVariable(this, "Print", BromScript::Libaries::Debug::Print));
		debug->Set("Connect", Converter::ToVariable(this, "Connect", BromScript::Libaries::Debug::Connect));
		debug->Set("Disconnect", Converter::ToVariable(this, "Disconnect", BromScript::Libaries::Debug::Disconnect));
		debug->Set("Break", Converter::ToVariable(this, "Break", BromScript::Libaries::Debug::Break));
		this->SetVar("Debug", Converter::ToVariable(this, debug));

		this->IncludingInternalUserdata = true;
		BromScript::Userdatas::Iterator::RegisterUD(this);
		BromScript::Userdatas::Socket::RegisterUD(this);
		BromScript::Userdatas::Packet::RegisterUD(this);
		BromScript::Userdatas::IO::RegisterUD(this);
		BromScript::Userdatas::Interop::RegisterUD(this);
		BromScript::Userdatas::InteropMethod::RegisterUD(this);
		BromScript::Userdatas::Array::RegisterUD(this);
		BromScript::Userdatas::RawData::RegisterUD(this);
		this->IncludingInternalUserdata = false;
	}
}
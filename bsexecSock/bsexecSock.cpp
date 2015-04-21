#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#include <iostream>
#include "SIF.h"
#include "EzSock.h"
#include "Packet.h"
#include <thread>
#include <BromScript.h>
#include <string>
#include <mutex>

using namespace Scratch;

CString CurrentOutput;
CString CurrentCode;
BromScript::Instance* CurrentBSI;
bool DoneWithScript = false;
mutex outputlock;

BS_ERRORFUNCTION(errfunc){
	outputlock.lock();
	CurrentOutput += "\n[red]BromScript error!\n";

	for (int i = 0; i < stacksize; i++) {
		CurrentOutput += Scratch::CString::Format("SID: %d, File: %s:%d, Function: %s\n", i, stack[i].Filename.str_szBuffer, stack[i].LineNumber, stack[i].Func->Name.str_szBuffer);
	}

	CurrentOutput += Scratch::CString::Format("Message: %s", msg);
	CurrentOutput += "[/red]";
	outputlock.unlock();
}

BS_FUNCTION(printoverride) {
	outputlock.lock();
	for (int i = 0; i < args->Count; i++) {
		if (i > 0)
			CurrentOutput += '\t';

		CurrentOutput += BromScript::Converter::VariableToString(bsi, args->GetVariable(i));
	}

	CurrentOutput += "\n";
	outputlock.unlock();

	return null;
}

void ThreadedCodeExec(){
	try{
		BromScript::Variable* var = CurrentBSI->DoString("inputbox", CurrentCode);
		outputlock.lock();
		if (var != null && var->Type != BromScript::VariableType::Null) {
			CurrentOutput += var->ToString();
		}
	} catch (BromScript::RuntimeException err) {
		outputlock.lock();
		BromScript::Function* func = CurrentBSI->GetCurrentFunction();
		if (func != null) {
			CurrentOutput += CString::Format("[red]Runtime error '%s:%d': %s[/red]\n", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
		} else {
			CurrentOutput += CString::Format("[red]Runtime error %s[/red]\n", err.Message.str_szBuffer);
		}
	} catch (BromScript::CompilerException err) {
		outputlock.lock();
		CurrentOutput += CString::Format("[red]Error while compiling '%s:%d': %s[/red]\n", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
	}
	outputlock.unlock();

	DoneWithScript = true;
}

void HandleC(EzSock* c){
	printf("Got new client from \"%d.%d.%d.%d\"\n", c->addr.sin_addr.S_un.S_un_b.s_b1, c->addr.sin_addr.S_un.S_un_b.s_b2, c->addr.sin_addr.S_un.S_un_b.s_b3, c->addr.sin_addr.S_un.S_un_b.s_b4);

	int len = 0;
	recv(c->sock, (char*)&len, 4, 0);

	if (len > 10000) {
		printf("Rejected, code too long (%d)", len);
		return;
	}

	if (len < 0) {
		printf("Rejected, invalid code length");
		return;
	}

	char* buff = new char[len + 1];
	buff[len] = 0;
	recv(c->sock, buff, len, 0);

	CurrentOutput = "";
	CurrentCode.str_szBuffer = buff;

	printf("Got code:\n%s\n", CurrentCode.str_szBuffer);

	CurrentBSI = new BromScript::Instance();
	CurrentBSI->LoadDefaultLibaries();

	CurrentBSI->SetErrorCallback(errfunc);
	CurrentBSI->RegisterFunction("print", printoverride);
	
	CurrentBSI->SetVar("includeDll", null);
	CurrentBSI->SetVar("include", null);
	CurrentBSI->SetVar("Console", null);

	CurrentBSI->SetVar("Debug", null);
	CurrentBSI->SetVar("Interop", null);
	CurrentBSI->SetVar("Socket", null);
	CurrentBSI->SetVar("Packet", null);
	CurrentBSI->SetVar("IO", null);
	CurrentBSI->SetVar("RawData", null);

	DoneWithScript = false;

	new std::thread(ThreadedCodeExec);

	int i = 0;
	while (i++ < 1000 && !DoneWithScript) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (!DoneWithScript) {
		CurrentBSI->KillScriptThreaded = true;
		CurrentBSI->Error("Excecution limit exceeded.");
	}

	outputlock.lock();
	// be sure we're doing printing stuff
	outputlock.unlock();

	len = CurrentOutput.Size() + 4;
	send(c->sock, (const char*)&len, 4, 0);
	len -= 4;
	send(c->sock, (const char*)&len, 4, 0);

	send(c->sock, CurrentOutput.str_szBuffer, len, 0);

	printf("Output:\n%s\n", CurrentOutput.str_szBuffer);

	delete[] CurrentCode.str_szBuffer;
	CurrentOutput = "";

	delete CurrentBSI;
	CurrentBSI = null;
}

LONG WINAPI OurCrashHandler(EXCEPTION_POINTERS* errinfo) {
	printf("Emergecny dump to lastcode.bs\n");

	remove("lastcode.bs");
	FILE* f = fopen("lastcode.bs", "wb");
	fwrite(CurrentCode.str_szBuffer, 1, CurrentCode.Size(), f);
	fclose(f);

	return 0;
}

int main(int argc, char* argv[])
{
	SetUnhandledExceptionFilter(OurCrashHandler);

	EzSock s;
	s.create();
	s.bind(12412);
	s.listen();

	EzSock c;

	while(true){
		Sleep(1);

		if (!s.accept(&c)){
			continue;
		}

		HandleC(&c);

		c.close();
	}

	return 0;
}


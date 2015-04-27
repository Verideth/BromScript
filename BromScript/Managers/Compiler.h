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

#ifndef BROMSCRIPT_COMPILER_INCLUDED
#define BROMSCRIPT_COMPILER_INCLUDED

#include "../SIF.h"
#include "../Misc.h"
#include "../Objects/List.h"
#include "../Objects/ByteWriter.h"
#include "../Scratch/CString.h"
#include "../Scratch/CDictionary.h"

namespace BromScript {
	class CompilerLocalData {
	public:
		Scratch::CString Name;
		int Type;

		CompilerLocalData() :Type(-1) {}
		CompilerLocalData(Scratch::CString name, int type) :Name(name), Type(type) {}
	};

	class CompilerLabelData {
	public:
		Scratch::CString Name;
		int Offset;
		int Line;

		int ScopeStart;
		int ScopeEnd;

		CompilerLabelData() {}
		CompilerLabelData(Scratch::CString name, int offset, int line) : ScopeStart(-1), ScopeEnd(-1), Line(line), Name(name), Offset(offset) {}
	};

	class Compiler {
	public:
		static byte* Run(const char* filename, const char* chunk, int chunklen, int* outbytecodelength);
		static byte* Run(const char* filename, const char* chunk, int chunklen, int* outbytecodelength, bool writelinenumbers, bool writeheader, List<CompilerException>& warnings);

	private:
		Compiler* Parent;
		Scratch::CString Filename;
		int CurrentStatmentLine;
		int CurrentLineParent;
		int CurrentPos;
		bool WriteLineNumbers;

		ByteWriter Writer;
		List<CompilerLocalData>& Locals;
		List<CompilerLabelData>& Gotos;
		List<CompilerException>& Warnings;
		List<Scratch::CString>& LoopLabels;
		Scratch::CDictionary<Scratch::CString, CompilerLabelData>& Labels;

		Scratch::CDictionary<Scratch::CString, Scratch::CString> Defines;
		Scratch::CDictionary<Scratch::CString, Scratch::CString> Sets;

		int ChunkSize;
		char* CurrentChunk;

		Scratch::CString* Peekline();
		Scratch::CString* ReadLine();
		Scratch::CString Unescape(Scratch::CString str);

		Scratch::CString GetUniqueLabelName();
		int SkipIfBlock();
		int DoProcess();
		void DoDefines();
		bool EndOfLine(int i);
		bool PastEndOfLine(int i);
		int GetCurrentLine(char* haystack, int cpos);
		int FindCharInSyntaxString(Scratch::CString& str, char tofind);

		int GetLocalIndex(Scratch::CString& key);

		byte* GetArgumentData(Scratch::CString line, int* size);
		void GetFunctionData(Scratch::CString line, char splitter, List<Scratch::CString>* ret);
		void GetIndexes(Scratch::CString line, List<Scratch::CString>* ret);
		Scratch::CString FindString(Scratch::CString line, Scratch::CString start, Scratch::CString end, int startpos);
		Scratch::CString* FindStringSpecial(Scratch::CString line, Scratch::CString start, Scratch::CString end, int startpos);
		int FindChar(Scratch::CString line, int startpos, char* tofind, int tofindlen);

		void ThrowError(Scratch::CString reason);
		void ThrowWarning(Scratch::CString reason);

		void _WriteArgumentData(List<Scratch::CString>& args, int& i);
		void WriteArgumentData(Scratch::CString line);
		void WriteCall(Scratch::CString line);
		void WriteCallThis(Scratch::CString line);
		void WriteGet(Scratch::CString line);
		void WriteSet(Scratch::CString line);

		void WriteFunction(Scratch::CString line);
		void WriteClass(Scratch::CString line);
		void WriteIf(Scratch::CString line);
		void WriteFor(Scratch::CString line);
		void WriteForeach(Scratch::CString line);
		void WriteWhile(Scratch::CString line);
		void WriteSetVar(Scratch::CString line);
		void WriteReturn(Scratch::CString line);
		void WriteDelete(Scratch::CString line);
		void WriteLabel(Scratch::CString line);
		void WriteJump(Scratch::CString line);
		void WriteJumpNT(Scratch::CString line);
		void WriteBreak(Scratch::CString line);
		void WriteContinue(Scratch::CString line);

		Compiler(Scratch::CString filename, const char* chunk, int chunklen, bool addcurline, List<CompilerLocalData>& locals, Scratch::CDictionary<Scratch::CString, CompilerLabelData>& labels, List<CompilerLabelData>& gotos, List<CompilerException>& warnings, Compiler* parent, List<Scratch::CString>& looplabels);
		~Compiler(void);

		byte* Run(int* outbytecodelength, List<Scratch::CString*>* strtbl);
		byte* Run(int* outbytecodelength, int currentlineparent, List<Scratch::CString*>* strtbl);
		void AddDefine(Scratch::CString key, Scratch::CString value);
		void AddSet(Scratch::CString key, Scratch::CString value);
	};
}

#endif
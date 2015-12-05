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

#include "Compiler.h"
#include <stdlib.h>

#include "../Objects/CompilerException.h"
#include "../Objects/Variable.h"

using namespace Scratch;

namespace BromScript {
	Compiler::Compiler(CString filename, const char* chunk, int chunklen, bool addcurline, List<CompilerLocalData>& locals, CDictionary<CString, CompilerLabelData>& labels, List<CompilerLabelData>& gotos, List<CompilerException>& warnings, Compiler* parent, List<CString>& looplabels) : Parent(parent), Gotos(gotos), Locals(locals), LoopLabels(looplabels), Warnings(warnings), Labels(labels), CurrentChunk(null), Filename(filename), WriteLineNumbers(addcurline), CurrentLineParent(0), CurrentPos(0), ChunkSize(chunklen) {
		if (this->ChunkSize == 0)
			return;

		this->CurrentChunk = new char[chunklen + 1];
		this->CurrentChunk[chunklen] = 0;

		memcpy(this->CurrentChunk, chunk, chunklen);
	}

	Compiler::~Compiler(void) {
		if (this->CurrentChunk != null) {
			delete[] this->CurrentChunk;
		}
	}

	byte* Compiler::Run(const char* filename, const char* chunk, int chunklen, int* outbytecodelength) {
		List<CompilerException> warnings;
		return Compiler::Run(filename, chunk, chunklen, outbytecodelength, true, true, warnings);
	}

	byte* Compiler::Run(const char* filename, const char* chunk, int chunklen, int* outbytecodelength, bool writecodelines, bool writeheader, List<CompilerException>& warnings) {
		List<CString*> strtbl;
		List<CompilerLocalData> locals;
		CDictionary<CString, CompilerLabelData> labels;
		List<CompilerLabelData> gotos;
		List<CString> looplabels;
		Compiler c(filename, chunk, chunklen, writecodelines, locals, labels, gotos, warnings, null, looplabels);
		c.CurrentLineParent = 1; // file start at 0 line number, so add one here, it fixes a shitload of headage.

		if (writeheader) {
			byte headerbuff[] = {0, 66, 82, 79, 77, 83, 67, 82, 73, 80, 84, 0, BROMSCRIPT_CURRENTVERSION};
			c.Writer.WriteBytes(headerbuff, 13, false);
		}

		c.Writer.WriteByte(Misc::ExecFuncs::StringTable);
		int stringtbljumppos = c.Writer.Count;
		c.Writer.WriteInt(0);

		c.Writer.WriteByte(Misc::ExecFuncs::GlobalLocals);
		int localsjumppos = c.Writer.Count;
		c.Writer.WriteInt(0);

		int codesize = 0;
		int scopestart = c.Writer.Count;
		byte* code = c.Run(&codesize, &strtbl);

		*(int*)(code + localsjumppos) = codesize + 1;
		c.Writer.WriteInt(locals.Count);
		for (int i = 0; i < locals.Count; i++) {
			c.Writer.WriteString(locals[i].Name);
			c.Writer.WriteInt(locals[i].Type);
		}

		*(int*)(code + stringtbljumppos) = codesize + c.Writer.Count + 1;
		c.Writer.WriteShort(strtbl.Count);
		for (int i = 0; i < strtbl.Count; i++) {
			CString str(strtbl[i]);

			c.Writer.WriteShort(str.Size());
			c.Writer.WriteBytes((byte*)str.str_szBuffer, str.Size(), false);
		}

		for (int i = 0; i < labels.Count(); i++) {
			CompilerLabelData& ld = labels.GetValueByIndex(i);

			if (ld.ScopeStart == -1) {
				ld.ScopeStart = scopestart;
				ld.ScopeEnd = scopestart + codesize + 1;
			}
		}

		for (int i = 0; i < gotos.Count; i++) {
			CompilerLabelData gd = gotos[i];

			if (!labels.HasKey(gd.Name)) {
				c.CurrentStatmentLine = gd.Line;
				c.ThrowError(CString::Format("Cannot find label '%s'", gd.Name.str_szBuffer));
			}

			CompilerLabelData ld = labels[gd.Name];

			if (gd.Offset < ld.ScopeStart || gd.Offset > ld.ScopeEnd) {
				c.CurrentStatmentLine = gd.Line;
				c.ThrowWarning("Jumping inside of another function, unsafe!");
			}

			*(int*)(code + gd.Offset) = ld.Offset;
		}

		byte* buff = new byte[codesize + c.Writer.Count + 1];
		buff[codesize] = null;

		memcpy(buff, code, codesize);
		memcpy(buff + codesize + 1, c.Writer.Buffer, c.Writer.Count);
		*outbytecodelength = codesize + c.Writer.Count + 1;

		for (int i = 0; i < strtbl.Count; i++) {
			delete strtbl[i];
		}

		delete[] code;
		return buff;
	}

	byte* Compiler::Run(int* outbytecodelength, int cpl, List<CString*>* strtbl) {
		this->CurrentLineParent = cpl;

		return this->Run(outbytecodelength, strtbl);
	}

	byte* Compiler::Run(int* outbytecodelength, List<CString*>* strtbl) {
		this->Writer.StringTable = strtbl;

		if (this->ChunkSize == 0) {
			*outbytecodelength = 0;
			return null;
		}

		int brackets_a = 0;
		int brackets_b = 0;
		int brackets_c = 0;
		bool inquotes = false;

		int f_a_cs = -1;
		int f_b_cs = -1;
		int f_c_cs = -1;

		int b_a_cs = -1;
		int b_b_cs = -1;
		int b_c_cs = -1;
		int q_cs = 0;

		int addline = this->Parent == null ? 1 : 0;

		char lastopened = null;
		List<char> bracketstack;
		List<int> bracketstackpos;
		for (int i = 0; i < this->ChunkSize; i++) {
			char c = this->CurrentChunk[i];

			if (c == '"' && (i == 0 || this->CurrentChunk[i - 1] != '\\')) {
				inquotes = !inquotes;
				if (inquotes) q_cs = i;
				continue;
			}

			if (!inquotes) {
				switch (c) {
					case '[': if (brackets_a++ == 0) b_a_cs = i; if (brackets_a == 1) f_a_cs = i; bracketstack.Add(']'); bracketstackpos.Add(i); break;
					case '{': if (brackets_b++ == 0) b_b_cs = i; if (brackets_b == 1) f_b_cs = i; bracketstack.Add('}'); bracketstackpos.Add(i); break;
					case '(': if (brackets_c++ == 0) b_c_cs = i; if (brackets_c == 1) f_c_cs = i; bracketstack.Add(')'); bracketstackpos.Add(i); break;

					case ']':{
						if (bracketstack[bracketstack.Count - 1] != c) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Expected '%c' tag, instead we got a '%c' tag, start tag at line %d", bracketstack[bracketstack.Count - 1], c, this->GetCurrentLine(this->CurrentChunk, bracketstackpos[bracketstackpos.Count - 1]) + addline));
						}

						if (--brackets_a < 0) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Square bracket count mismatch (-1), first '[' tag at line %d", this->GetCurrentLine(this->CurrentChunk, b_a_cs) + addline));
						}

						bracketstack.RemoveAt(bracketstack.Count - 1);
						bracketstackpos.RemoveAt(bracketstackpos.Count - 1);
					} break;

					case '}':{
						if (bracketstack[bracketstack.Count - 1] != c) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Expected '%c' tag, instead we got a '%c' tag, start tag at line %d", bracketstack[bracketstack.Count - 1], c, this->GetCurrentLine(this->CurrentChunk, bracketstackpos[bracketstackpos.Count - 1]) + addline));
						}

						if (--brackets_b < 0) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Curly bracket count mismatch (-1), first '}' tag at line %d", this->GetCurrentLine(this->CurrentChunk, b_b_cs) + addline));
						}

						bracketstack.RemoveAt(bracketstack.Count - 1);
						bracketstackpos.RemoveAt(bracketstackpos.Count - 1);
					} break;

					case ')':{
						if (bracketstack[bracketstack.Count - 1] != c) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Expected '%c' tag, instead we got a '%c' tag, start tag at line %d", bracketstack[bracketstack.Count - 1], c, this->GetCurrentLine(this->CurrentChunk, bracketstackpos[bracketstackpos.Count - 1]) + addline));
						}

						if (--brackets_c < 0) {
							this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i);
							this->ThrowError(CString::Format("Round bracket count mismatch (-1), first ')' tag at line %d", this->GetCurrentLine(this->CurrentChunk, b_c_cs) + addline));
						}

						bracketstack.RemoveAt(bracketstack.Count - 1);
						bracketstackpos.RemoveAt(bracketstackpos.Count - 1);
					} break;
				}
			}
		}

		if (brackets_a > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_a_cs); this->ThrowError(CString::Format("Square bracket count mismatch (%d)", brackets_a)); }
		if (brackets_b > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_b_cs); this->ThrowError(CString::Format("Curly bracket count mismatch (%d)", brackets_b)); }
		if (brackets_c > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_c_cs); this->ThrowError(CString::Format("Round bracket count mismatch (%d)", brackets_c)); }

		if (inquotes) {
			this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, q_cs);
			this->ThrowError(CString::Format("Ending quote not found"));
		}

		this->DoProcess();

		byte* buff = new byte[this->Writer.Count];
		memcpy(buff, this->Writer.Buffer, this->Writer.Count);
		*outbytecodelength = this->Writer.Count;
		this->Writer.Clear();

		return buff;
	}

	int Compiler::GetCurrentLine(char* haystack, int cpos) {
		int ret = 0;

		int i = 0;
		while (haystack[i] != 0 && i < cpos) {
			if (haystack[i] == '\n') {
				ret++;
			}

			i++;
		}

		return ret;
	}

	int Compiler::DoProcess() {
		bool redodefines = false;
		while (true) {
			if (redodefines) {
				redodefines = false;
				this->DoDefines();
			}

			int oldpos = this->CurrentPos;
			CString* tmpstr = this->ReadLine();
			if (tmpstr == null)
				break;

			CString curline = tmpstr->Trim();
			delete tmpstr;

			if (this->WriteLineNumbers) {
				this->Writer.WriteByte(Misc::ExecFuncs::CurrentLine);
				this->Writer.WriteInt(this->CurrentStatmentLine + this->CurrentLineParent);
			}

			if (curline.Size() == 0)
				continue;

			if (curline.StartsWith('#')) {
				List<CString> args;
				int ssize = curline.Size();
				int ppos = 1;
				bool inquote = false;

				for (int i = 2; i < ssize; i++) {
					if (curline[i] == '"' && curline[i - 1] != '\\') inquote = !inquote;

					if (!inquote && curline[i] == ' ') {
						args.Add(curline.Substring(ppos, i - ppos).Trim());
						ppos = i;
					}
				}

				if (ppos != ssize) {
					CString remainer = curline.Substring(ppos).Trim();
					if (remainer.Size() > 0) {
						args.Add(remainer);
					}
				}

				CString cmd = args[0].ToLower();

				for (int i = 1; i < args.Count; i++) {
					args[i] = this->Unescape(args[i]);
				}

				if (cmd == "define") {
					if (args.Count != 3) this->ThrowError(CString::Format("Expected 2 arguments at '#%s'", cmd.str_szBuffer));
					this->Defines[args[1]] = args[2];

					redodefines = true;
				} else if (cmd == "set") {
					if (args.Count != 3) this->ThrowError(CString::Format("Expected 2 arguments at '#%s'", cmd.str_szBuffer));
					this->Sets[args[1]] = args[2];
				} else if (cmd == "unset") {
					if (args.Count != 2) this->ThrowError(CString::Format("Expected 1 argument at '#%s'", cmd.str_szBuffer));
					this->Sets.RemoveByKey(args[1]);
				} else if (cmd == "undefine") {
					if (args.Count != 2) this->ThrowError(CString::Format("Expected 1 argument at '#%s'", cmd.str_szBuffer));
					this->Defines.RemoveByKey(args[1]);
				} else if (cmd == "if" || cmd == "elseif") {
					if (args.Count != 4) this->ThrowError(CString::Format("Expected 3 arguments at '#%s'", cmd.str_szBuffer));
					if (!this->Sets[args[1]]) {
						this->ThrowError(CString::Format("Unknown preprocessor if var '%s'", args[1].str_szBuffer));
					}

					bool doblock = args[2] == "==" ? this->Sets[args[1]] == args[3] : this->Sets[args[1]] != args[3];
					if (doblock) {
						if (this->DoProcess() == 1)
							this->SkipIfBlock();
					} else {
						if (this->SkipIfBlock() == 1)
							this->DoProcess();
					}
				} else if (cmd == "end") {
					return 2;
				} else if (cmd == "else") {
					return 1;
				} else if (cmd.StartsWith('!')) {
					// do absolutly nothing, yay linux hash-bang!
				} else {
					this->ThrowWarning(CString::Format("Unknown preprocessor '#%s'", cmd.str_szBuffer));
				}
			} else {
				if (curline.StartsWith("function") || curline.StartsWith("local function")) this->WriteFunction(curline);
				else if (curline.StartsWith("class ") || curline.StartsWith("local class ")) this->WriteClass(curline); //
				else if (curline.StartsWith("local ")) this->WriteSet(curline);
				else if (curline.StartsWith("while ") || curline.StartsWith("while(")) this->WriteWhile(curline);
				else if (curline.StartsWith("for ") || curline.StartsWith("for(")) this->WriteFor(curline);
				else if (curline.StartsWith("foreach ") || curline.StartsWith("foreach(")) this->WriteForeach(curline);
				else if (curline.StartsWith("if(") || curline.StartsWith("if (")) this->WriteIf(curline);
				else if (curline.StartsWith("goto ")) this->WriteJump(curline.Substring(5).Trim());
				else if (curline.EndsWith(":")) this->WriteLabel(curline.Substring(0, curline.Size() - 1));
				else if (curline == "continue") this->WriteContinue(curline);
				else if (curline == "break") this->WriteBreak(curline);
				else if (curline == "return" || curline.StartsWith("return ")) this->WriteReturn(curline);
				else if (curline == "delete" || curline.StartsWith("delete ")) this->WriteDelete(curline);
				else if (this->FindChar(curline, 0, "=", 1) > -1) this->WriteSet(curline);
				else {
					this->WriteArgumentData(curline);
					this->Writer.WriteOperator(Operators::Pop); // argumentdata always returns something, so off with it's head!
				}
			}
		}

		return 0;
	}

	int Compiler::SkipIfBlock() {
		int nested = 0;

		while (true) {
			int oldpos = this->CurrentPos;
			CString* tmpstr = this->ReadLine();
			if (tmpstr == null)
				return 2;

			CString cur = tmpstr->Trim().ToLower();
			delete tmpstr;

			if (cur.StartsWith("#")) {
				if (cur.StartsWith("#end")) {
					nested--;

					if (nested == -1) {
						return 2;
					}
				}

				if (cur.StartsWith("#elseif")) {
					this->CurrentPos = oldpos;
					return 1;
				}

				if (cur.StartsWith("#if")) {
					nested++;
				}

				if (cur.StartsWith("#else")) {
					return 1;
				}
			}
		}
	}

	int __uniquelabelid = 0;
	CString Compiler::GetUniqueLabelName() {
		return CString::Format("__BS_INTERNAL_ULID__%d__", ++__uniquelabelid);
	}

	CString Compiler::Unescape(CString str) {
		str = str.Replace("\\'", "\'");
		str = str.Replace("\\\"", "\"");
		str = str.Replace("\\\\", "\\");
		str = str.Replace("\\0", "\0");
		str = str.Replace("\\1", "\1");
		str = str.Replace("\\a", "\a");
		str = str.Replace("\\b", "\b");
		str = str.Replace("\\f", "\f");
		str = str.Replace("\\n", "\n");
		str = str.Replace("\\r", "\r");
		str = str.Replace("\\t", "\t");
		str = str.Replace("\\v", "\v");
		return str;
	}

	bool CharArrContains(char c, char* arr, int arrsize) {
		for (int i = 0; i < arrsize; i++) {
			if (c == arr[i]) {
				return true;
			}
		}

		return false;
	}

	bool CharIsNumber(char c) {
		return c >= '0' && c <= '9';
	}

	bool CharIsLetter(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
	}

	bool CharIsLetterOrNumber(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
	}

	bool IsEscaped(int i, char* str) {
		int num = 0;

		while (str[--i] == '\\')
			num++;

		return num % 2 == 1;
	}

	CString* Compiler::Peekline() {
		int cp = this->CurrentPos;

		CString* line = this->ReadLine();

		this->CurrentPos = cp;

		return line;
	}

	bool Compiler::EndOfLine(int i) {
		char enders[] = {'+', '-', '*', '^', '%', '<', '>', '/', '(', '[', '{', ',', '=', ';', '"', '.', ',', '|', '&'};

		for (; i < this->ChunkSize; i++) {
			char c = this->CurrentChunk[i];
			if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
				continue;

			return !CharArrContains(c, enders, sizeof(enders));
		}

		return true;
	}

	bool Compiler::PastEndOfLine(int i) {
		char enders[] = {'+', '-', '*', '^', '%', '<', '>', '/', '(', '[', '{', ';', ',', '=', '"', '.', ',', '|', '&'};

		for (; i < this->ChunkSize; i--) {
			char c = this->CurrentChunk[i];
			if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
				continue;

			return !CharArrContains(c, enders, sizeof(enders));
		}

		return true;
	}

	CString* Compiler::ReadLine() {
		this->CurrentStatmentLine = -1;

		if (this->CurrentPos >= this->ChunkSize)
			return null;

		int roundbrackers = 0;
		int tablebrackets = 0;
		int funcbrackets = 0;
		bool incomment = false;
		bool instring = false;
		bool canend = false;

		char* currentline = null;
		int currentindex = 0;

		int i = this->CurrentPos;
		char c;
		for (; i < this->ChunkSize; i++) {
			c = this->CurrentChunk[i];

			if (instring) {
				if (c == '"' && !IsEscaped(i, this->CurrentChunk)) {
					instring = false;
					canend = true;
				}
			} else {
				switch (c) {
					case '=': canend = false; break;
					case '"': instring = true; break;
					case '(': roundbrackers++; canend = false; break;
					case '[': tablebrackets++; canend = false; break;
					case '{': funcbrackets++; canend = false; break;
					case ')': roundbrackers--; canend = true; break;
					case ']': tablebrackets--; canend = true; break;
					case '}': funcbrackets--; canend = true; break;

					case ';':
						if (funcbrackets == 0 && roundbrackers == 0 && tablebrackets == 0) {
							goto doreturn;
						}
						break;

					case '#':
						// if we only encountered the pre-processor so far.
						if (this->CurrentStatmentLine == -1) {
							this->CurrentStatmentLine = GetCurrentLine(this->CurrentChunk, i);

							char* strptr = strstr(this->CurrentChunk + i, "\n");
							if (strptr == null)
								strptr = this->CurrentChunk + this->ChunkSize;

							currentindex = strptr - this->CurrentChunk - i;

							char* nbuff = new char[currentindex + 1];
							memcpy(nbuff, this->CurrentChunk + i, currentindex);

							delete[] currentline;
							currentline = nbuff;

							i = strptr - this->CurrentChunk;
							goto doreturn;
						}
						break;
					case '\n':
						if (funcbrackets == 0 && roundbrackers == 0 && tablebrackets == 0) {
							canend = true;
						}
						break;

					case '/':
						if (i + 1 < this->ChunkSize) {
							if (this->CurrentChunk[i + 1] == '/') {
								char* strptr = strstr(this->CurrentChunk + i, "\n");
								i = strptr == null ? this->ChunkSize : (int)(strptr - this->CurrentChunk) - 1;

								if (funcbrackets == 0 && roundbrackers == 0 && tablebrackets == 0) {
									canend = true;
								}
								continue;
							} else if (this->CurrentChunk[i + 1] == '*') {
								char* strptr = strstr(this->CurrentChunk + i, "*/");
								i = strptr == null ? this->ChunkSize : (int)(strptr - this->CurrentChunk) + 1;
								continue;
							}
						}
						break;
				}

				if (this->CurrentStatmentLine == -1 && CharIsLetter(c))
					this->CurrentStatmentLine = GetCurrentLine(this->CurrentChunk, i);
			}

			if (canend && !this->EndOfLine(i + 1)) {
				canend = false;
			}

			if (currentindex % 1024 <= 1) { // we'd do == 0 here, but below at the doreturn label we need to set the null terminator, so c/v the code there, check for this rather than 0
				char* tmpnew = new char[currentindex + 1024];

				if (currentline != null) {
					memcpy(tmpnew, currentline, currentindex);
					delete[] currentline;
				}

				currentline = tmpnew;
			}
			currentline[currentindex++] = c;

			if (!instring) {
				if (funcbrackets == 0 && roundbrackers == 0 && tablebrackets == 0 && canend) {

doreturn:
					currentline[currentindex] = 0;

					// now lets trim it
					char* trimmed = currentline;
					while (*trimmed == ' ' || *trimmed == '\n' || *trimmed == '\r' || *trimmed == '\t') trimmed++;

					for (int i2 = strlen(trimmed) - 1; i2 >= 0; i2--) {
						if (trimmed[i2] != ' ' && trimmed[i2] != '\n' && trimmed[i2] != '\r' && trimmed[i2] != '\t') {
							trimmed[i2 + 1] = '\0';
							break;
						}
					}

					// ignore 0 sized strings
					if (*trimmed == null && i < this->ChunkSize) {
						canend = false;
						continue;
					}

					CString* ret = new CString(trimmed);
					delete[] currentline;

					this->CurrentPos = ++i;
					return ret;
				}
			}
		}

		if (currentindex > 0) goto doreturn;

		// empty statment or whatever, MOVING ALONG!
		if (currentline != null) {
			delete[] currentline;
		}

		return null;
	}

	void Compiler::GetFunctionData(CString line, char splitter, List<CString>* tblargs) {
		bool kakquiots = false, kakbrackets = false;
		int kakietsanders = 0, kakbrackets2 = 0;

		int linelen = line.Size();
		for (int i = 0; i < linelen; i++) {
			if (line[i] == '"' && !(i > 0 && line[i - 1] == '\\'))
				kakquiots = !kakquiots;

			if (!kakquiots) {
				if (line[i] == '{') kakbrackets = true;
				else if (line[i] == '}') kakbrackets = false;
				else if (line[i] == '[') kakbrackets2++;
				else if (line[i] == ']') kakbrackets2--;
				else if (line[i] == '(') kakietsanders++;
				else if (line[i] == ')') kakietsanders--;
			}

			if (line[i] == splitter && !kakquiots && !kakbrackets && kakietsanders == 0 && kakbrackets2 == 0) {
				CString tmp = line.Substring(0, i).Trim();
				if (tmp.Size() > 0)
					tblargs->Add(tmp);

				int spos = i + (i + 1 < linelen ? 1 : 0) == 0 ? 1 : i + (i + 1 < linelen ? 1 : 0);
				line = line.Substring(spos, linelen - spos);
				i = -1;

				linelen = line.Size();
			}
		}

		if (linelen > 0)
			tblargs->Add(line);

		for (int i = 0; i < tblargs->Count; i++) {
			tblargs->Set(i, tblargs->Get(i).Trim());

			if (tblargs->Get(i).Size() == 0) {
				tblargs->RemoveAt(i);
				i--;
			}
		}
	}

	void Compiler::GetIndexes(CString line, List<CString>* ret) {
		CString toadd;
		int len = line.Size();
		int lastpos = 0;

		bool quotes = false;
		bool addquotes = false;
		bool endedfunccall = false;

		int squarebrackets = 0;
		int roundbrackets = 0;
		int curlybrackets = 0;

		for (int i = 0; i < len; i++) {
			char c = line[i];
			bool add = true;

			switch (c) {
				case '"': if (i == 0 || line[i - 1] != '\\') quotes = !quotes; break;
				case '(': if (!quotes) roundbrackets++;
					if (!quotes && curlybrackets == 0 && squarebrackets == 0 && roundbrackets == 1) {
						if (endedfunccall) {
							if (addquotes)
								toadd += '"';

							ret->Add(toadd);
							toadd = "";

							addquotes = false;

							endedfunccall = false;
						}

						if (addquotes) {
							toadd += '"';
							addquotes = false;
						}
					}

					break;
				case '{': if (!quotes) curlybrackets++; break;

				case ')': if (!quotes) roundbrackets--;
					if (!quotes && curlybrackets == 0 && squarebrackets == 0 && roundbrackets == 0)
						endedfunccall = true;
					break;

				case '}': if (!quotes) curlybrackets--; break;
				case ']': if (!quotes) squarebrackets--;
					if (!quotes && curlybrackets == 0 && squarebrackets == 0 && roundbrackets == 0)
						add = false;
					break;

				case '[':
					if (!quotes && curlybrackets == 0 && squarebrackets == 0 && roundbrackets == 0) {
						if (addquotes)
							toadd += '"';

						ret->Add(toadd);
						toadd = "";
						add = false;

						addquotes = false;
						endedfunccall = false;
					}

					if (!quotes) squarebrackets++;
					break;

				case '.':
					if (quotes || curlybrackets != 0 || squarebrackets != 0 || roundbrackets != 0) break;

					if (addquotes)
						toadd += '"';

					addquotes = true;

					ret->Add(toadd);
					toadd = '"';

					add = false;
					endedfunccall = false;
					break;
			}

			if (add)
				toadd += c;
		}

		if (toadd.Size() > 0) {
			if (addquotes)
				toadd += '"';

			ret->Add(toadd);
		}
	}

	int Compiler::FindCharInSyntaxString(CString& str, char tofind) {
		int brackets_a = 0;
		int brackets_b = 0;
		int brackets_c = 0;
		bool inquotes = false;

		int strlen = str.Size();
		for (int i2 = 0; i2 < strlen; i2++) {
			char c = str[i2];

			if (c == tofind) {
				if (!inquotes && brackets_a == 0 && brackets_b == 0 && brackets_c == 0) {
					return i2;
				}
			}

			switch (c) {
				case '[': brackets_a++; break;
				case ']': if (--brackets_a < 0) brackets_a = 0; break;
				case '{': brackets_b++; break;
				case '}': if (--brackets_b < 0) brackets_b = 0; break;
				case '(': brackets_c++; break;
				case ')': if (--brackets_c < 0) brackets_c = 0; break;

				case '"':
					if (i2 == 0 || str[i2 - 1] != '\\') {
						inquotes = !inquotes;
					}
					break;
			}
		}

		return -1;
	}

	byte* Compiler::GetArgumentData(CString line, int* size) { return null; }

	void Compiler::_WriteArgumentData(List<CString>& args, int& i) {
		char mathsplitters[] = {'+', '-', '*', '%', '/', '=', '!', '>', '<', '|', '&'};

		CString arg = args[i];
		if (this->FindCharInSyntaxString(arg, ' ') > -1) {
			if (!arg.StartsWith("new ") && !arg.StartsWith("enum ") && !arg.StartsWith("function ") && !arg.StartsWith("function(")) {
				this->ThrowError(CString::Format("Cannot compute '%s'", arg.str_szBuffer));
			}
		}

		if (arg == "false") {
			this->Writer.WriteOperator(Operators::StackBool);
			this->Writer.WriteBool(false);
		} else if (arg == "true") {
			this->Writer.WriteOperator(Operators::StackBool);
			this->Writer.WriteBool(true);
		} else if (arg == "null") {
			this->Writer.WriteOperator(Operators::StackNull);
		} else if (arg.StartsWith("enum ")) {
			int argpos = this->FindChar(arg, 0, "{", 1);
			if (argpos == -1) {
				this->ThrowError("Invalid enum block");
			}

			arg = arg.Substring(argpos + 1, -1);

			List<CString> tblargs;
			this->GetFunctionData(arg.Substring(1, -1), ',', &tblargs);

			this->Writer.WriteOperator(Operators::StackTable);

			CString prevkey;
			for (int i2 = 0; i2 < tblargs.Count; i2++) {
				int keypos = this->FindChar(tblargs[i2], 0, "=", 1);
				if (keypos != -1) {
					this->WriteArgumentData(tblargs[i2].Substring(keypos + 1).Trim());
					prevkey = tblargs[i2].Substring(0, keypos).Trim();

					this->Writer.WriteOperator(Operators::AddIndex);
					this->Writer.WriteBool(true);
					this->Writer.WriteString(prevkey);
				} else {
					if (i2 == 0) {
						this->WriteArgumentData("0"); // first value of an unasigned enum value should be 0
					} else {
						// get previouse value, and add +1 to it
						this->Writer.WriteOperator(Operators::Duplicate);
						this->WriteArgumentData(CString::Format("\"%s\"", prevkey.str_szBuffer));
						this->Writer.WriteOperator(Operators::GetIndex);
						this->WriteArgumentData("1");
						this->Writer.WriteOperator(Operators::ArithmeticAdd);
					}
					prevkey = tblargs[i2];

					this->Writer.WriteOperator(Operators::AddIndex);
					this->Writer.WriteBool(true);
					this->Writer.WriteString(prevkey);
				}
			}
		} else if (arg.StartsWith("function")) {
			int currentpos = this->CurrentPos;
			CString* tmp = this->FindStringSpecial(arg, "function", "{", 0);
			if (tmp == null)
				this->ThrowError("Invalid function block");

			CString funcline(tmp->Trim());
			delete tmp;

			int chunkpos = funcline.Size();

			CStackArray<CString> funcargs;
			int argpos = this->FindChar(funcline, 0, "(", 1);
			if (argpos > -1) {
				funcline.Substring(argpos + 1, funcline.Size() - 2 - argpos).Split(",", funcargs);

				for (int i2 = 0; i2 < funcargs.Count(); i2++) {
					funcargs[i2] = funcargs[i2].Trim();

					if (funcargs[i2].Size() == 0)
						delete funcargs.PopAt(i2--);
				}
			}

			int chunkstart = this->FindChar(arg, 8, "{", 1);
			CString funcchunk = arg.Substring(chunkstart + 1, -1);

			List<CompilerLocalData> locals;

			List<bool> isreflist;
			for (int i2 = 0; i2 < funcargs.Count(); i2++) {
				CString curarg = funcargs[i2];
				int spacepos = curarg.IndexOf(' ');
				int vartype = -1;
				if (spacepos > -1) {
					CString typestr = curarg.Substring(0, spacepos);
					curarg = curarg.Substring(spacepos + 1).Trim();

					if (typestr == "number") vartype = VariableType::Number;
					else if (typestr == "bool") vartype = VariableType::Bool;
					else if (typestr == "function") vartype = VariableType::Function;
					else if (typestr == "string") vartype = VariableType::String;
					else if (typestr == "table") vartype = VariableType::Table;
					else vartype = VariableType::Userdata;
				}

				if (curarg.StartsWith('&')) {
					curarg = curarg.Substring(1);
				}
				isreflist.Add(funcargs[i2].IndexOf('&') > -1);

				funcargs[i2] = curarg;
				locals.Add(CompilerLocalData(curarg, vartype));
			}

			List<CString> looplabels;
			Compiler chunkproc("Function", funcchunk.str_szBuffer, funcchunk.Size(), this->WriteLineNumbers, locals, this->Labels, this->Gotos, this->Warnings, this, looplabels);

			this->Writer.WriteOperator(Operators::StackFunction);
			this->Writer.WriteString(this->Filename);
			this->Writer.WriteStrings(funcargs, true);


			this->Writer.Count += 4; // to fix label offsets
			int labelstart = this->Labels.Count();
			int scopestart = this->Writer.Count;

			byte* bytecode;
			int codelen = 0;
			try {
				bytecode = chunkproc.Run(&codelen, this->CurrentLineParent + this->CurrentStatmentLine, this->Writer.StringTable);
			} catch (CompilerException e) {
				e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);
				e.CurrentLine += GetCurrentLine(this->CurrentChunk, chunkstart);

				throw e;
			}
			codelen++;

			this->Writer.Count -= 4; // to unfix label offsets
			for (int i = labelstart; i < this->Labels.Count(); i++) {
				CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

				if (ld.ScopeStart == -1) {
					ld.ScopeStart = scopestart;
					ld.ScopeEnd = scopestart + codelen;
				}
			}

			this->Writer.WriteInt(codelen);
			this->Writer.WriteBytes(bytecode, codelen - 1, false);
			this->Writer.WriteOperator(Operators::EndScope); // write the end scope to be sure the function always returns

			this->Writer.WriteInt(locals.Count);
			for (int i = 0; i < locals.Count; i++) {
				this->Writer.WriteString(locals[i].Name);
				this->Writer.WriteByte(i < funcargs.Count() ? (isreflist[i] ? 1 : 0) : 0); // locals in function scope are not references (todo perhaps?)
				this->Writer.WriteInt(locals[i].Type);
			}

			delete[] bytecode;
		} else if (arg == "new" || arg.StartsWith("new ")) {
			if (arg == "new") {
				i++;
				if (i >= args.Count) {
					this->ThrowError("Invalid function block");
				}

				arg = args[i];
			} else {
				arg = arg.Substring(4);
			}

			CString classname = "";
			CString funcargs = "";

			bool kakquiots = false;
			int kakietsanders = 0, kakietsanders2 = 0;
			int arglen = arg.Size();
			List<CString> setsafter;
			for (int i2 = 0; i2 < arglen; i2++) {
				if (arg[i2] == '"' && !(i2 > 0 && arg[i2 - 1] == '\\'))
					kakquiots = !kakquiots;

				if (!kakquiots && arg[i2] == '(')
					kakietsanders++;

				if (!kakquiots && arg[i2] == ')')
					kakietsanders--;

				if (!kakquiots && arg[i2] == '[')
					kakietsanders2++;

				if (!kakquiots && arg[i2] == ']')
					kakietsanders2--;

				if (arg[i2] == '(' && !kakquiots && kakietsanders == 1 && kakietsanders2 == 0) {
					classname = arg.Substring(0, i2);

					if (arg.EndsWith('}')) {
						int setspos = this->FindChar(arg, i2, "{", 1);
						funcargs = arg.Substring(i2 + 1, setspos - i2 - 2);
						this->GetFunctionData(arg.Substring(setspos + 1, -1), ',', &setsafter);
					} else {
						funcargs = arg.Substring(i2 + 1);
						funcargs = funcargs.Substring(0, funcargs.Size() - 1);
					}
					break;
				}
			}

			if (classname.Size() == 0)
				classname = arg;

			classname = classname.Trim();

			List<CString> paras;
			this->GetFunctionData(funcargs, ',', &paras);

			for (int i2 = paras.Count - 1; i2 > -1; i2--) {
				this->WriteArgumentData(paras[i2]);
			}

			this->WriteArgumentData(classname);
			this->Writer.WriteOperator(Operators::New);
			this->Writer.WriteInt(paras.Count);

			for (int i2 = 0; i2 < setsafter.Count; i2++) {
				CStackArray<CString> tmparr;
				setsafter[i2].Split("=", tmparr);
				if (tmparr.Count() != 2) {
					this->ThrowError("Expected 'key = value, key2 = value2, etc'");
				}

				this->WriteArgumentData(tmparr[1].Trim());

				this->Writer.WriteOperator(Operators::AddIndex);
				this->Writer.WriteBool(true);
				this->Writer.WriteString(tmparr[0].Trim());

			}
		} else if (arg.EndsWith("++") || arg.EndsWith("--")) {
			this->WriteArgumentData(arg.Substring(0, -2));
			this->Writer.WriteOperator(arg.EndsWith("++") ? Operators::PostIncrement : Operators::PostDecrement);
		} else if (arg.StartsWith("++") || arg.StartsWith("--")) {
			this->WriteArgumentData(arg.Substring(2));
			this->Writer.WriteOperator(arg[0] == '+' ? Operators::PreIncrement : Operators::PreDecrement);
		} else if (arg == "(") {
			CString dat = CString();
			int curopenings = 1;
			for (int i2 = i + 1; i2 < args.Count; i2++) {
				if (args[i2] == "(")
					curopenings++;

				if (args[i2] == ")")
					curopenings--;

				if (curopenings == 0) {
					i = i2;
					break;
				}

				dat = CString::Format("%s %s", dat.str_szBuffer, args[i2].str_szBuffer).Trim();
			}

			this->WriteArgumentData(dat);
		} else if (arg.StartsWith("\"") && arg.EndsWith("\"")) {
			this->Writer.WriteOperator(Operators::StackString);
			this->Writer.WriteString(arg.Substring(1, -1));
		} else if (arg.StartsWith("#")) {
			this->WriteArgumentData(arg.Substring(1));
			this->Writer.WriteOperator(Operators::GetCount);
		} else if (arg.StartsWith("{") && arg.EndsWith("}")) {
			List<CString> tblargs;
			this->GetFunctionData(arg.Substring(1, -1), ',', &tblargs);

			this->Writer.WriteOperator(Operators::StackTable);

			for (int i2 = 0; i2 < tblargs.Count; i2++) {
				int keypos = this->FindChar(tblargs[i2], 0, "=", 1);
				if (keypos != -1) {
					this->WriteArgumentData(tblargs[i2].Substring(keypos + 1).Trim());

					this->Writer.WriteOperator(Operators::AddIndex);
					this->Writer.WriteBool(true);
					this->Writer.WriteString(tblargs[i2].Substring(0, keypos).Trim());
				} else {
					this->WriteArgumentData(tblargs[i2]);

					this->Writer.WriteOperator(Operators::AddIndex);
					this->Writer.WriteBool(false);
				}
			}
		} else if (CharIsNumber(arg[0]) || ((arg[0] == '-' || arg[0] == '.') && arg.Size() > 1 && CharIsNumber(arg[1]))) {
			double num = 0;
			if (arg.StartsWith("0x")) {
				char* p = null;
				num = (double)strtoul(arg.str_szBuffer + 2, &p, 16);
			} else if (arg[0] == '-') {
				num = atof(arg.Substring(1).str_szBuffer) * -1;
			} else {
				num = atof(arg.str_szBuffer);
			}

			this->Writer.WriteOperator(Operators::StackNumber);
			this->Writer.WriteDouble(num);
		} else {
			List<CString> tbldata;
			this->GetIndexes(arg, &tbldata);

			if (tbldata.Count > 1) {
				CString tblname = tbldata[0].Trim();
				tblname = tblname.Trim();

				CString funcline = tblname;

				int argpos = this->FindChar(tblname, 0, "(", 1);
				if (argpos > -1) {
					int argposend = this->FindChar(funcline, 0, ")", 1);
					if (argposend == -1) this->ThrowError(CString::Format("Expected end of function call ')' at '%s'", funcline.str_szBuffer));
					if (argposend + 1 < funcline.Size()) this->ThrowError(CString::Format("Cannot compute '%s'", funcline.str_szBuffer));

					List<CString> tmpisfunctbl;
					this->GetIndexes(funcline.Substring(argpos + 1, argposend - argpos - 1), &tmpisfunctbl);

					funcline = funcline.Substring(argpos + 1, -1);
					tblname = tblname.Substring(0, argpos);

					this->WriteGet(tblname);
					this->WriteCall(funcline);
				} else {
					this->WriteGet(tblname);
				}

				for (int i2 = 1; i2 < tbldata.Count; i2++) {
					CString varname = tbldata[i2];
					funcline = varname;
					bool func = false;

					int argpos = this->FindChar(funcline, 0, "(", 1);
					if (argpos > -1) {
						int argposend = this->FindChar(funcline, 0, ")", 1);
						if (argposend == -1) this->ThrowError(CString::Format("Expected end of function call ')' at '%s'", funcline.str_szBuffer));
						if (argposend + 1 < funcline.Size()) this->ThrowError(CString::Format("Cannot compute '%s'", funcline.str_szBuffer));

						List<CString> tmpisfunctbl;
						this->GetIndexes(funcline.Substring(argpos + 1, argposend - argpos - 1), &tmpisfunctbl);

						func = true;
						funcline = funcline.Substring(argpos + 1, -1);
						varname = varname.Substring(0, argpos);

						// duplicate the tbl stack, so that we can use it as a this object in functions
						this->Writer.WriteOperator(Operators::Duplicate);
					}

					varname = varname.Trim();
					if (varname == "\"\"" || varname.Size() == 0) {
						if (!func) {
							this->ThrowError("Missing indexing key");
						}
					}

					this->WriteArgumentData(varname);
					if (varname != "\"\"" && varname.Size() > 0) {
						this->Writer.WriteOperator(Operators::GetIndex);
					}

					if (func) {
						this->WriteCallThis(funcline);
					}
				}
			} else if (!CharArrContains(arg[0], mathsplitters, sizeof(mathsplitters))) {
				int funcpos = arg.IndexOf('(');
				if (funcpos > -1 && CharIsLetter(arg[0])) {
					int argposend = this->FindChar(arg, 0, ")", 1);
					if (argposend == -1) this->ThrowError(CString::Format("Expected end of function call ')' at '%s'", arg.str_szBuffer));
					if (argposend + 1 < arg.Size()) this->ThrowError(CString::Format("Cannot compute '%s'", arg.str_szBuffer));

					CString funcname = arg.Substring(0, funcpos);
					this->WriteGet(funcname);
					this->WriteCall(arg.Substring(funcpos + 1, -1));
				} else {
					this->WriteGet(args[i].Trim());
				}
			} else {
				this->ThrowError(CString::Format("Cannot compute '%s'", arg.str_szBuffer));
			}
		}
	}

	void Compiler::WriteArgumentData(CString line) {
		List<CString> args;

		char mathsplitters[] = {'+', '-', '*', '%', '/', '=', '!', '>', '<', '|', '&'};

		if (line.StartsWith('{') && line.EndsWith('}')) {
			args.Add(line);
		} else {
			char splitters[] = {'+', '-', '*', '%', '/', '>', '<', '|', '&'};

			char ignoreopp[] = {' ', '(', ')'};
			bool kakquiots = false;
			int kakietsanders = 0, kakbrackets2 = 0, kakbrackets = 0;
			int curpos = 0;
			bool didop = false;
			int linelen = line.Size();
			for (int i = 0; i < linelen; i++) {
				if (line[i] == '"' && !(i > 0 && line[i - 1] == '\\'))
					kakquiots = !kakquiots;

				if (!kakquiots) {
					if (line[i] == '{') kakbrackets++;
					else if (line[i] == '}') kakbrackets--;
					else if (line[i] == '[') kakbrackets2++;
					else if (line[i] == ']') kakbrackets2--;
					else if (line[i] == '(') kakietsanders++;
					else if (line[i] == ')') kakietsanders--;
				}

				if (!kakquiots && kakbrackets == 0 && kakietsanders == 0 && kakbrackets2 == 0) {
					if (i + 1 < linelen && (line[i] == '=' || line[i] == '!') && line[i + 1] == '=' && !didop) {
						args.Add(line.Substring(curpos, i - curpos));
						args.Add(line.Substring(i, 2));
						curpos = i + 2;
						didop = true;
						i++; // this is to skip the next loop, this is a 2 len splitter, not a 1 len.
					} else if (i + 1 < linelen && (line[i] == '<' || line[i] == '>') && line[i + 1] == '=' && !didop) {
						args.Add(line.Substring(curpos, i - curpos));
						args.Add(line.Substring(i, 2));
						curpos = i + 2;
						didop = true;
						i++; // this is to skip the next loop, this is a 2 len splitter, not a 1 len.
					} else if (i + 1 < linelen && ((line[i] == '>' && line[i + 1] == '>') || (line[i] == '<' && line[i + 1] == '<')) && !didop) {
						args.Add(line.Substring(curpos, i - curpos));
						args.Add(line.Substring(i, 2));
						curpos = i + 2;
						didop = true;
						i++; // this is to skip the next loop, this is a 2 len splitter, not a 1 len.
					} else if (i + 1 < linelen && ((line[i] == '|' && line[i + 1] == '|') || (line[i] == '&' && line[i + 1] == '&')) && !didop) {
						args.Add(line.Substring(curpos, i - curpos));
						args.Add(line.Substring(i, 2));
						curpos = i + 2;
						didop = true;
						i++; // this is to skip the next loop, this is a 2 len splitter, not a 1 len.
					} else if (CharArrContains(line[i], splitters, sizeof(splitters)) && !didop && (i + 1 == linelen || (line.Substring(i, 2) != "++" && line.Substring(i, 2) != "--")) && (i - 1 == -1 || (line.Substring(i - 1, 2) != "++" && line.Substring(i - 1, 2) != "--"))) {
						args.Add(line.Substring(curpos, i - curpos));
						args.Add(line[i]);
						curpos = i + 1;
						didop = true;
					} else if (!CharArrContains(line[i], ignoreopp, 3)) didop = false;
				}
			}

			args.Add(line.Substring(curpos));

			for (int i = 0; i < args.Count; i++) {
				args.Set(i, args.Get(i).Trim());

				if (args[i].Size() == 0) {
					args.RemoveAt(i);
					i--;
				}
			}

			char mathprior[] = {'*', '/', '%', '|', '&'};
			char mathnonprior[] = {'+', '-', '=', '!', '>', '<'};
			bool prevop = true;

			for (int i = 0; i < args.Count; i++) {
				if (args[i][0] == '-' && prevop) {
					args.Set(i, args[i].Substring(1));
					if (args[i].Size() == 0)
						args.RemoveAt(i);

					args.Insert(i + 1, ')');

					args.Insert(i, '-');
					args.Insert(i, '0');
					args.Insert(i, '(');

					prevop = false;
					i += 4;
				} else if (args[i][0] == '!' && prevop) {
					args.Set(i, args[i].Substring(1));
					if (args[i].Size() == 0)
						args.RemoveAt(i);

					args.Insert(i + 1, ')');
					args.Insert(i, "==");
					args.Insert(i, "false");
					args.Insert(i, '(');

					prevop = false;
					i += 4;
				} else if (CharArrContains(args[i][0], mathprior, sizeof(mathprior)) || CharArrContains(args[i][0], mathnonprior, sizeof(mathnonprior))) {
					prevop = true;
				} else if (!CharArrContains(args[i][0], ignoreopp, 3)) {
					prevop = false;
				}
			}

			didop = false;
			int bpos = -1;
			int bracks = 0;
			for (int i = 0; i < args.Count; i++) {
				if (CharArrContains(args[i][0], mathprior, sizeof(mathprior)) && args[i].Size() == 1 && bpos == -1) {
					bpos = i - 1;
					didop = true;
				} else if (bpos > -1 && CharArrContains(args[i][0], mathnonprior, sizeof(mathnonprior)) && !didop && bracks == 0) {
					args.Insert(i, ')');
					args.Insert(bpos, '(');

					bpos = -1;
					didop = true;
				} else {
					didop = false;

					if (bpos > -1) {
						if (args[i][0] == '(') bracks++;
						else if (args[i][0] == ')') bracks--;
						else if (args[i][0] == ' ') bracks--;
					}
				}
			}

			if (bpos > 0) {
				args.Add(')');
				args.Insert(bpos, '(');

				bpos = -1;
			}

			for (int i = 0; i < args.Count; i++) {
				if (args[i].Size() == 0) {
					args.RemoveAt(i);
					i--;
					continue;
				} else if (args[i].Contains('"')) {
					args.Set(i, args[i].Replace("\\\"", "\""));
				}

				bool notafunc = false;
				if (args[i].StartsWith("(") && args[i].Size() > 1) {
					args.Set(i, args[i].Substring(1));
					args.Insert(i, "(");
					notafunc = true;
					i++;
				}

				if (notafunc && args[i].EndsWith(')') && args[i].Size() > 1) {
					args.Set(i, args[i].Substring(0, args[i].Size() - 1));
					args.Insert(i + 1, ")");

					i--;
				}
			}
		}

		CString scopeendlabel = this->GetUniqueLabelName();
		for (int i = 0; i < args.Count;) {
			CString arg = args[i];
			this->_WriteArgumentData(args, i);
			i++;

			while (i < args.Count) {
				arg = args[i];
				Operators opcode = Operators::Skip;
				if (arg == "+") { opcode = Operators::ArithmeticAdd;
				} else if (arg == "-") { opcode = Operators::ArithmeticSubstract;
				} else if (arg == "/") { opcode = Operators::ArithmeticDivide;
				} else if (arg == "*") { opcode = Operators::ArithmeticMultiply;
				} else if (arg == "%") { opcode = Operators::ArithmeticModulo;
				} else if (arg == "&&") { opcode = Operators::ArithmeticAnd;
				} else if (arg == "||") { opcode = Operators::ArithmeticOr;
				} else if (arg == ">") { opcode = Operators::ArithmeticGreaterThan;
				} else if (arg == ">=") { opcode = Operators::ArithmeticGreaterOrEqual;
				} else if (arg == "<") { opcode = Operators::ArithmeticLessThan;
				} else if (arg == "<=") { opcode = Operators::ArithmeticLessOrEqual;
				} else if (arg == "==") { opcode = Operators::ArithmeticEqual;
				} else if (arg == "!=") { opcode = Operators::ArithmeticNotEqual;
				} else if (arg == "<<") { opcode = Operators::ArithmeticBitwiseLeft;
				} else if (arg == ">>") { opcode = Operators::ArithmeticBitwiseRight;
				} else if (arg == "|") { opcode = Operators::ArithmeticBitwiseOr;
				} else if (arg == "&") { opcode = Operators::ArithmeticBitwiseAnd;
				}

				if (opcode != Operators::Skip) {
					if (i + 1 == args.Count) {
						this->ThrowError(CString::Format("Didn't expect a operator at the end of a statement! Statement is: '%s'", line.str_szBuffer));
					}

					if (opcode == Operators::ArithmeticOr) {
						CString tmplbl = this->GetUniqueLabelName();

						this->Writer.WriteOperator(Operators::Duplicate);
						this->WriteJumpNT(tmplbl); // if not true, jump to right arg
						this->WriteJump(scopeendlabel); // if true, jump to end of right arg
						this->WriteLabel(tmplbl);
					}

					// righter argument
					i++;
					this->_WriteArgumentData(args, i);

					if (opcode != Operators::ArithmeticOr) this->Writer.WriteByte((byte)opcode);


					i++;
				} else {
					break;
				}
			}
		}
		this->WriteLabel(scopeendlabel);
	}

	void Compiler::WriteCall(CString line) {
		List<CString> paras;
		this->GetFunctionData(line, ',', &paras);

		for (int i2 = paras.Count - 1; i2 > -1; i2--) {
			this->WriteArgumentData(paras[i2]);
		}

		this->Writer.WriteOperator(Operators::Call);
		this->Writer.WriteInt(paras.Count);
	}

	void Compiler::WriteCallThis(CString line) {
		List<CString> paras;
		this->GetFunctionData(line, ',', &paras);

		for (int i2 = paras.Count - 1; i2 > -1; i2--) {
			this->WriteArgumentData(paras[i2]);
		}

		this->Writer.WriteOperator(Operators::CallThis);
		this->Writer.WriteInt(paras.Count);
	}

	void Compiler::WriteGet(CString line) {
		int localindex = this->GetLocalIndex(line);
		if (localindex > -1) {
			this->Writer.WriteOperator(Operators::GetL);
			this->Writer.WriteInt(localindex);
		} else {
			this->Writer.WriteOperator(Operators::Get);
			this->Writer.WriteString(line);
		}
	}

	CString Compiler::FindString(CString text, CString start, CString end, int startpos) {
		int spos = text.IndexOf(start, startpos);
		if (spos == -1)
			return "";

		int epos = text.IndexOf(end, spos);
		if (epos == -1)
			return "";

		return text.Substring(spos + start.Size(), epos - (spos + start.Size()));
	}

	CString* Compiler::FindStringSpecial(CString text, CString start, CString end, int startpos) {
		bool quote = false;
		int funcbrackets = 0;
		int squarebrackers = 0;
		int tablebrackets = 0;

		int spos = -1;
		int epos = -1;
		int slen = start.Size();
		int elen = end.Size();
		int textlen = text.Size();
		for (int i = startpos; i < textlen; i++) {
			char c = text[i];

			if (!quote) {
				if (c == start[0] && funcbrackets == 0 && squarebrackers == 0 && tablebrackets == 0) {
					bool found = true;
					for (int i2 = 0; i2 < slen; i2++) {
						if (start[i2] != text[i + i2]) {
							found = false;
							break;
						}
					}

					if (found) {
						spos = i + slen;
						break;
					}
				}

				if (c == '(') funcbrackets++;
				else if (c == ')') funcbrackets--;
				else if (c == '[') squarebrackers++;
				else if (c == ']') squarebrackers--;
				else if (c == '{') tablebrackets++;
				else if (c == '}') tablebrackets--;
			}

			if (c == '"' && (i == 0 || text[i - 1] != '\\')) quote = !quote;
		}

		if (spos == -1)
			return null;

		quote = false;
		funcbrackets = 0;
		squarebrackers = 0;
		tablebrackets = 0;

		for (int i = spos; i < textlen; i++) {
			char c = text[i];

			if (!quote) {
				if (c == end[0] && funcbrackets == 0 && squarebrackers == 0 && tablebrackets == 0) {
					bool found = true;
					for (int i2 = 1; i2 < elen; i2++) {
						if (end[i2] != text[i + i2]) {
							found = false;
							break;
						}
					}

					if (found) {
						epos = i;
						break;
					}
				}

				if (c == '(') funcbrackets++;
				else if (c == ')') funcbrackets--;
				else if (c == '[') squarebrackers++;
				else if (c == ']') squarebrackers--;
				else if (c == '{') tablebrackets++;
				else if (c == '}') tablebrackets--;
			}

			if (c == '"' && (i == 0 || text[i - 1] != '\\')) quote = !quote;
		}

		if (epos == -1)
			return null;

		return new CString(text.Substring(spos, epos - spos));
	}

	int Compiler::FindChar(CString line, int startpos, char* tofind, int tofindlen) {
		bool quote = false;
		int funcbrackets = 0;
		int squarebrackers = 0;
		int tablebrackets = 0;

		for (int i = startpos; i < line.Size(); i++) {
			char c = line[i];

			if (!quote) {
				if (CharArrContains(c, tofind, tofindlen) && funcbrackets == 0 && squarebrackers == 0 && tablebrackets == 0) {
					return i;
				}

				if (c == '(') funcbrackets++;
				else if (c == ')') funcbrackets--;
				else if (c == '[') squarebrackers++;
				else if (c == ']') squarebrackers--;
				else if (c == '{') tablebrackets++;
				else if (c == '}') tablebrackets--;

				// if we search on ) } or ] it also needs to trigger after the ifs above, so let's do another check here.
				// but now, only if it's been one of the above signs, AND it doesn't come after it, because then it would return one after it.
				if (i + 1 == line.Size() || line[i + 1] != ')' && line[i + 1] != ']' && line[i + 1] != '}') {
					if (CharArrContains(c, tofind, tofindlen) && funcbrackets == 0 && squarebrackers == 0 && tablebrackets == 0) {
						return i;
					}
				}
			}

			if (c == '"' && (i == 0 || line[i - 1] != '\\')) quote = !quote;
		}

		return -1;
	}

	void Compiler::ThrowError(CString err) {
		int linenum = 1; // +1 as silly human offset
		Compiler* uplink = this;
		while (uplink != null) {
			linenum += uplink->CurrentStatmentLine;
			uplink = uplink->Parent;
		}

		throw CompilerException(err, this->Filename, linenum);
	}

	void Compiler::ThrowWarning(CString err) {
		int linenum = 1; // +1 as silly human offset
		Compiler* uplink = this;
		while (uplink != null) {
			linenum += uplink->CurrentStatmentLine;
			uplink = uplink->Parent;
		}

		this->Warnings.Add(CompilerException(err, this->Filename, linenum));
	}

	void Compiler::WriteReturn(CString line) {
		line = line.Substring(6).Trim();

		if (line.Size() > 0) {
			this->WriteArgumentData(line);
			this->Writer.WriteOperator(Operators::Return);
			this->Writer.WriteBool(true);
		} else {
			this->Writer.WriteOperator(Operators::Return);
			this->Writer.WriteBool(false);
		}
	}

	void Compiler::WriteDelete(CString line) {
		CString arg = line.Substring(6).Trim();
		if (arg.Size() == 0) this->ThrowError("Missing argument after delete keyword");

		this->WriteArgumentData(arg);
		this->Writer.WriteOperator(Operators::Delete);
	}

	void Compiler::DoDefines() {
		if (this->Defines.Count() == 0)
			return;

		bool quote = false;
		CString newchunk(this->CurrentChunk);
		int clen = this->ChunkSize;

		for (int i = this->CurrentPos; i < clen; i++) {
			char pc = newchunk[i];
			if (pc == '"' && newchunk[i - 1] != '\\')
				quote = !quote;

			if (!quote) {
				if (i + 1 < clen && pc == '/') {
					if (newchunk[i + 1] == '*') {
						i += 2;

						while (i + 2 < clen && (newchunk[i] != '*' || newchunk[i + 1] != '/')) { i++; }

						if (i >= clen) {
							break;
						}
					} else if (newchunk[i + 1] == '/') {
						while (i < clen && newchunk[++i] != '\n') {}
						i++;
					}
				}

				int dlen = this->Defines.Count();
				for (int di = 0; di < dlen; di++) {
					CString key = this->Defines.GetKeyByIndex(di);
					CString value = this->Defines.GetValueByIndex(di);
					int ksize = key.Size();

					if (clen > i + ksize) {
						CString str = CString(newchunk.str_szBuffer + i).Substring(0, ksize);

						if (str == key) {
							int vsize = value.Size();
							if (!CharIsLetter(newchunk[i - 1] && !CharIsLetter(newchunk.str_szBuffer[i + vsize]))) {
								int nsize = clen + vsize - ksize;

								char* newbuffer = new char[nsize + 1];
								memcpy(newbuffer, newchunk.str_szBuffer, i);
								memcpy(newbuffer + i, value.str_szBuffer, vsize);
								memcpy(newbuffer + i + vsize, newchunk.str_szBuffer + i + ksize, clen - i - ksize);
								newbuffer[nsize] = 0;

								delete[] newchunk.str_szBuffer;
								newchunk.str_szBuffer = newbuffer;

								clen += vsize - ksize;
								i += vsize;
							}
						}
					}
				}
			}
		}

		while (this->Defines.Count() > 0)
			this->Defines.RemoveByIndex(0);

		delete[] this->CurrentChunk;
		this->CurrentChunk = newchunk.str_szBuffer;
		newchunk.str_szBuffer = CString::str_szEmpty;

		this->ChunkSize = clen;
	}

	void Compiler::WriteFunction(CString line) {
		bool local = line.StartsWith("local function");
		if (local) {
			int equalspos = this->FindCharInSyntaxString(line, '=');
			int bracketpos = this->FindCharInSyntaxString(line, '{');

			if (equalspos > -1 && (bracketpos == -1 || equalspos < bracketpos)) {
				this->WriteSet(line);
				return;
			}

			line = line.Substring(6);
		}

		CString* tmp = this->FindStringSpecial(line, "function ", "{", 0);
		if (tmp == null)
			this->ThrowError("Invalid function block");

		CString funcline(tmp->Trim());
		delete tmp;

		int chunkpos = this->FindChar(line, 10, "{", 1);

		CString funcname = "";
		CString argsline = "";
		CStackArray<CString> funcargs;
		int argpos = this->FindChar(funcline, 0, "(", 1);
		if (argpos == -1){
			funcname = funcline;
			argsline = "";
		} else {
			argsline = funcline.Substring(argpos + 1, -1);
			funcname = funcline.Substring(0, argpos);
		}

		this->WriteSet(CString::Format("%s %s = function(%s) %s", local ? "local function" : "", funcname.str_szBuffer, argsline.str_szBuffer, line.Substring(chunkpos).str_szBuffer));
	}

	void Compiler::WriteWhile(CString line) {
		CString labelname_condition = this->GetUniqueLabelName();
		CString labelname_end = this->GetUniqueLabelName();

		this->LoopLabels.Add(labelname_condition);
		this->LoopLabels.Add(labelname_end);

		CString args = "";

		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos, ")", 1);
			if (argendpos == -1)
				this->ThrowError("Error, no ) at while");

			args = line.Substring(argpos + 1, argendpos - argpos - 1).Trim();
		}

		int chunkpos = this->FindChar(line, 0, "{", 1);

		CString codechunk;
		if (chunkpos == -1) {
			CString* tmp = this->ReadLine();
			if (tmp == null)
				this->ThrowError("No chunk at while");

			codechunk = tmp->Trim();
			delete tmp;
		} else {
			codechunk = line.Substring(chunkpos + 1);
			codechunk = codechunk.Substring(0, codechunk.Size() - 1);
		}

		this->WriteLabel(labelname_condition);

		// optimize out the if condition, if it's a infinite loop anyway
		if (args != "true") {
			this->WriteArgumentData(args);
			this->WriteJumpNT(labelname_end);
		}

		Compiler chunkproc("while", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);
		byte* bytecode;
		int codelen;
		try {
			bytecode = chunkproc.Run(&codelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.WriteBytes(bytecode, codelen, false);
		delete[] bytecode;

		this->WriteJump(labelname_condition);
		this->WriteLabel(labelname_end);

		// clear up the loop labels for break/continue
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
	}

	void Compiler::WriteForeach(CString line) {
		CString args = "";

		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos + 1, ")", 1);
			if (argendpos == -1)
				this->ThrowError("Error, no ) at foreach");

			args = line.Substring(argpos + 1, argendpos - argpos - 1);
		}

		List<CString> fordata;
		this->GetFunctionData(args, ',', &fordata);

		if (fordata.Count != 3) {
			this->ThrowError("Invalid syntax for foreach(key, value, tbl)");
		}

		int chunkpos = this->FindChar(line, 0, "{", 1);
		CString codechunk;
		if (chunkpos == -1) {
			CString* tmp = this->ReadLine();
			if (tmp == null)
				this->ThrowError("No chunk at foreach");

			codechunk = tmp->Trim();
			delete tmp;
		} else {
			codechunk = line.Substring(chunkpos + 1);
			codechunk = codechunk.Substring(0, codechunk.Size() - 1);
		}

		CString keyname = fordata[0];
		CString valuename = fordata[1];

		CString labelname_end = this->GetUniqueLabelName();
		CString labelname_condition = this->GetUniqueLabelName();
		CString labelname_iteratorvarname = this->GetUniqueLabelName();

		this->LoopLabels.Add(labelname_condition);
		this->LoopLabels.Add(labelname_end);

		this->WriteSet(CString::Format("local %s = null", keyname.str_szBuffer));
		this->WriteSet(CString::Format("local %s = null", valuename.str_szBuffer));
		this->WriteSet(CString::Format("local Iterator %s = new Iterator(%s)", labelname_iteratorvarname.str_szBuffer, fordata[2].str_szBuffer));


		this->WriteLabel(labelname_condition);
		this->WriteSet(CString::Format("%s = %s.NextKey()", keyname.str_szBuffer, labelname_iteratorvarname.str_szBuffer));

		this->WriteArgumentData(keyname);
		this->WriteJumpNT(labelname_end);

		this->WriteSet(CString::Format("%s = %s[%s]", valuename.str_szBuffer, labelname_iteratorvarname.str_szBuffer, keyname.str_szBuffer));

		Compiler chunkproc("foreach", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);
		byte* bytecode;
		int bytecodelen;
		try {
			bytecode = chunkproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);
			throw e;
		}

		this->Writer.WriteBytes(bytecode, bytecodelen, false);

		this->WriteJump(labelname_condition);
		this->WriteLabel(labelname_end);

		delete[] bytecode;

		// clear up the loop labels for break/continue
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
	}

	void Compiler::WriteFor(CString line) {
		CString labelname_condition = this->GetUniqueLabelName();
		CString labelname_after = this->GetUniqueLabelName();
		CString labelname_end = this->GetUniqueLabelName();

		this->LoopLabels.Add(labelname_after);
		this->LoopLabels.Add(labelname_end);

		// find condition data
		CString args = "";

		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos + 1, ")", 1);
			if (argendpos == -1)
				this->ThrowError("Error, no ) at for");

			args = line.Substring(argpos + 1, argendpos - argpos - 1);
		}

		List<CString> fordata;
		this->GetFunctionData(args, ',', &fordata);
		if (fordata.Count != 3)
			this->ThrowError("Error, invalid (init, check, after) statment at for loop");

		// find chunk
		int chunkpos = this->FindChar(line, 0, "{", 1);
		CString codechunk;
		if (chunkpos == -1) {
			CString* tmp = this->ReadLine();
			if (tmp == null)
				this->ThrowError("No chunk at for");

			codechunk = tmp->Trim();
			delete tmp;
		} else {
			codechunk = line.Substring(chunkpos + 1);
			codechunk = codechunk.Substring(0, codechunk.Size() - 1);
		}

		// write init chunk
		Compiler initproc("for (>><<, {}, {})", fordata[0].str_szBuffer, fordata[0].Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);
		byte* bytecodeinit;
		int bytecodeinitlen;
		try {
			bytecodeinit = initproc.Run(&bytecodeinitlen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.WriteBytes(bytecodeinit, bytecodeinitlen, false);

		// write jump that skips the loop end chunk for the first run
		this->WriteJump(labelname_condition);
		this->WriteLabel(labelname_after);

		// write endchunk data
		Compiler endproc("for ({} {}, >><<)", fordata[2].str_szBuffer, fordata[2].Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);
		byte* bytecodeend;
		int bytecodeendlen;
		try {
			bytecodeend = endproc.Run(&bytecodeendlen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.WriteBytes(bytecodeend, bytecodeendlen, false);

		// write the condition
		this->WriteLabel(labelname_condition);
		this->WriteArgumentData(fordata[1]);

		this->WriteJumpNT(labelname_end);

		Compiler chunkproc("for", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);
		byte* bytecode;
		int bytecodelen;
		try {
			bytecode = chunkproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.WriteBytes(bytecode, bytecodelen, false);

		this->WriteJump(labelname_after);
		this->WriteLabel(labelname_end);

		delete[] bytecodeinit;
		delete[] bytecode;
		delete[] bytecodeend;

		// clear up the loop labels for break/continue
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
		this->LoopLabels.RemoveAt(this->LoopLabels.Count - 1);
	}

	int Compiler::GetLocalIndex(CString& key) {
		for (int i = this->Locals.Count - 1; i >= 0; i--) {
			if (this->Locals[i].Name == key) {
				return i;
			}
		}

		return -1;
	}

	void Compiler::WriteSet(CString line) {
		bool local = line.StartsWith("local ");
		int vartype = -1;
		if (local) {
			line = line.Substring(6);
		}

		int argpos = this->FindChar(line, 0, "=", 1);
		if (argpos == -1)
			this->ThrowError("Error, missing =");

		CString name = line.Substring(0, argpos).Trim();
		CString restdata = line.Substring(argpos + 1).Trim();

		char mathsigns[] = {'+', '-', '*', '/', '%', '^'};
		if (CharArrContains(line[argpos - 1], mathsigns, sizeof(mathsigns))) {
			name = name.Substring(0, -1).Trim();
			restdata = CString::Format("%s%c%s", name.str_szBuffer, line[argpos - 1], restdata.str_szBuffer);
		}

		if (local) {
			int spacepos = name.IndexOf(' ');
			if (spacepos > -1) {
				CString typestr = name.Substring(0, spacepos);
				name = name.Substring(spacepos + 1);

				if (typestr == "number") vartype = VariableType::Number;
				else if (typestr == "bool") vartype = VariableType::Bool;
				else if (typestr == "function") vartype = VariableType::Function;
				else if (typestr == "string") vartype = VariableType::String;
				else if (typestr == "table") vartype = VariableType::Table;
				else vartype = VariableType::Userdata;
			}

			this->Locals.Add(CompilerLocalData(name, vartype));
		}

		this->WriteArgumentData(restdata);

		List<CString> tbldata;
		this->GetIndexes(name, &tbldata);

		if (tbldata.Count > 1) {
			if (local) {
				this->ThrowError("You can't localize a table index");
			}

			CString tblname = tbldata[0].Trim();
			this->WriteGet(tblname);

			for (int i2 = 1; i2 < tbldata.Count; i2++) {
				CString varname = tbldata[i2];
				CString funcline = varname;
				bool func = false;

				int argpos = this->FindChar(funcline, 0, "(", 1);
				if (argpos > -1) {
					int argposend = this->FindChar(funcline, 0, ")", 1);
					if (argposend == -1) this->ThrowError(CString::Format("Expected end of function call ')' at '%s'", funcline.str_szBuffer));
					if (argposend + 1 < funcline.Size()) this->ThrowError(CString::Format("Cannot compute '%s'", funcline.str_szBuffer));

					if (funcline.Substring(argpos + 1, -1).Trim().Size() == 0) {
						List<CString> tmpisfunctbl;
						this->GetIndexes(funcline.Substring(argpos + 1, argposend - argpos - 1), &tmpisfunctbl);

						func = true;
						funcline = funcline.Substring(argpos + 1, -1);
						varname = varname.Substring(0, argpos);

						// duplicate the tbl stack, so that we can use it as a this object in functions
						this->Writer.WriteOperator(Operators::Duplicate);
					}
				}

				varname = varname.Trim();
				if (varname == "\"\"" || varname.Size() == 0) {
					if (!func) {
						this->ThrowError("Missing indexing key");
					}
				}

				this->WriteArgumentData(varname);

				if (i2 + 1 == tbldata.Count) {
					if (func) {
						this->WriteCallThis(funcline);
					}

					this->Writer.WriteOperator(Operators::SetIndex);
				} else {
					if (varname != "\"\"" && varname.Size() > 0) {
						this->Writer.WriteOperator(Operators::GetIndex);
					}

					if (func) {
						this->WriteCallThis(funcline);
					}
				}
			}
		} else {
			if (local) {
				this->Writer.WriteOperator(Operators::SetL);
				this->Writer.WriteInt(this->Locals.Count - 1);
			} else {
				int localindex = this->GetLocalIndex(name);
				if (localindex > -1) {
					this->Writer.WriteOperator(Operators::SetL);
					this->Writer.WriteInt(localindex);
				} else {
					this->Writer.WriteOperator(Operators::Set);
					this->Writer.WriteString(name);
				}
			}
		}
	}

	void Compiler::WriteIf(CString line) {
		CString labelname_end = this->GetUniqueLabelName();
		CString labelname_nextblock = labelname_end;

		byte curif = 1;
		while (curif != 0) {
			int argstartpos = 0;
			int argpos = 0;
			int chunkpos = 0;

			if (curif != 2) {
				argstartpos = this->FindChar(line, 0, "(", 1);
				if (argstartpos == -1)
					this->ThrowError("Error, no ( at if/elseif");

				argpos = this->FindChar(line, argstartpos, ")", 1);
				if (argpos == -1)
					this->ThrowError("Error, no ) at if/elseif");

				chunkpos = this->FindChar(line, argpos + 1, "{", 1);
			} else {
				chunkpos = this->FindChar(line, 0, "{", 1);
			}

			CString* tmp;
			CString codechunk;
			if (chunkpos == -1) {
				tmp = this->ReadLine();
				if (tmp == null)
					this->ThrowError("Could not find code chunk");

				codechunk = CString(tmp);
				delete tmp;
			} else {
				codechunk = line.Substring(chunkpos + 1);
				codechunk = codechunk.Substring(0, codechunk.Size() - 1);
			}

			byte nextif = 0;
			CString* nextlineptr = this->Peekline();
			CString nextline;
			if (nextlineptr != null) {
				nextline = CString(nextlineptr);
				if (nextline.StartsWith("elseif (") || nextline.StartsWith("elseif(") || nextline.StartsWith("else if") || nextline == "elseif") {
					nextif = 1;
					labelname_nextblock = this->GetUniqueLabelName();
				} else if (nextline.StartsWith("else{") || nextline.StartsWith("else {") || (nextline.StartsWith("else") && nextline.Size() == 4)) {
					nextif = 2;
					labelname_nextblock = this->GetUniqueLabelName();
				}

				delete nextlineptr;
			}

			if (curif != 2) {
				this->WriteArgumentData(line.Substring(argstartpos + 1, argpos - argstartpos - 1));
				this->WriteJumpNT(labelname_nextblock);
			}


			Compiler blockproc("codeblock", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);

			byte* bytecode;
			int bytecodelen;
			try {
				bytecode = blockproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
			} catch (CompilerException e) {
				e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

				throw e;
			}

			this->Writer.WriteBytes(bytecode, bytecodelen, false);
			delete[] bytecode;

			this->WriteJump(labelname_end);

			if (labelname_nextblock != labelname_end) {
				this->WriteLabel(labelname_nextblock);
			}

			if (nextif != 0) {
				labelname_nextblock = nextif == 2 ? labelname_end : this->GetUniqueLabelName();

				nextlineptr = this->ReadLine();
				line = CString(nextlineptr);
				delete nextlineptr;
			}

			curif = nextif;
		}

		this->WriteLabel(labelname_end);
	}

	void Compiler::WriteClass(CString line) {
		bool local = line.StartsWith("local ");
		if (local)
			line = line.Substring(6);

		CString name;
		CString args;
		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos, ")", 1);
			if (argendpos == -1)
				this->ThrowError("Error, no ) at class");

			args = line.Substring(argpos + 1, argendpos - argpos - 1);
		}

		int chunkpos = this->FindChar(line, argpos == -1 ? 0 : argpos, "{", 1);

		if (argpos == 0)
			name = line.Substring(6, chunkpos - 6);
		else
			name = line.Substring(6, argpos - 6);

		CString chunk = line.Substring(chunkpos + 1);
		chunk = chunk.Substring(0, chunk.Size() - 1);

		List<CString> tableargs;
		this->GetIndexes(args, &tableargs);

		this->Writer.WriteByte(Misc::ExecFuncs::Class);
		this->Writer.WriteBool(local);
		this->Writer.WriteString(name);
		this->Writer.WriteStrings(tableargs, true);

		if (local) this->Locals.Add(CompilerLocalData(name, VariableType::Class));

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc(name, chunk.str_szBuffer, chunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this, this->LoopLabels);

		byte* bytecode;
		int bytecodelen;
		try {
			bytecode = chunkproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.Count -= 4; // to unfix label offsets
		for (int i = labelstart; i < this->Labels.Count(); i++) {
			CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

			if (ld.ScopeStart == -1) {
				ld.ScopeStart = scopestart;
				ld.ScopeEnd = scopestart + bytecodelen;
			}
		}

		this->Writer.WriteBytes(bytecode, bytecodelen, true);

		delete[] bytecode;
	}

	void Compiler::WriteJump(CString line) {
		int pos = 0;
		Compiler* uplink = this;
		while (uplink != null) {
			pos += uplink->Writer.Count;
			uplink = uplink->Parent;
		}

		this->Writer.WriteOperator(Operators::Jump);
		this->Gotos.Add(CompilerLabelData(line, pos + 1, this->CurrentStatmentLine + this->CurrentLineParent));
		this->Writer.WriteInt(0); // jmp pos
	}

	void Compiler::WriteJumpNT(CString line) {
		int pos = 0;
		Compiler* uplink = this;
		while (uplink != null) {
			pos += uplink->Writer.Count;
			uplink = uplink->Parent;
		}

		this->Writer.WriteOperator(Operators::JumpNT);
		this->Gotos.Add(CompilerLabelData(line, pos + 1, this->CurrentStatmentLine + this->CurrentLineParent));
		this->Writer.WriteInt(0); // jmp pos
	}

	void Compiler::WriteLabel(CString line) {
		int pos = 0;
		Compiler* uplink = this;
		while (uplink != null) {
			pos += uplink->Writer.Count;
			uplink = uplink->Parent;
		}

		this->Labels[line] = CompilerLabelData(line, pos, this->CurrentStatmentLine + this->CurrentLineParent);
	}

	void Compiler::WriteBreak(CString line) {
		if (this->LoopLabels.Count == 0) {
			this->ThrowError("Unexpected break keyword");
		}

		this->WriteJump(this->LoopLabels[this->LoopLabels.Count - 1]);
	}

	void Compiler::WriteContinue(CString line) {
		if (this->LoopLabels.Count == 0) {
			this->ThrowError("Unexpected break keyword");
		}

		this->WriteJump(this->LoopLabels[this->LoopLabels.Count - 2]);
	}
}
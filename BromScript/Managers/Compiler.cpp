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
	Compiler::Compiler(CString filename, const char* chunk, int chunklen, bool addcurline, List<CompilerLocalData>& locals, CDictionary<CString, CompilerLabelData>& labels, List<CompilerLabelData>& gotos, List<CompilerException>& warnings, Compiler* parent) : Parent(parent), Gotos(gotos), Locals(locals), Warnings(warnings), Labels(labels), CurrentChunk(null), Filename(filename), WriteLineNumbers(addcurline), CurrentLineParent(0), CurrentPos(0), ChunkSize(chunklen) {
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
		Compiler c(filename, chunk, chunklen, writecodelines, locals, labels, gotos, warnings, null);
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

		*(int*)(code + localsjumppos) = codesize;
		c.Writer.WriteInt(locals.Count);
		for (int i = 0; i < locals.Count; i++) {
			c.Writer.WriteString(locals[i].Name);
			c.Writer.WriteInt(locals[i].Type);
		}

		*(int*)(code + stringtbljumppos) = codesize + c.Writer.Count;
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
				ld.ScopeEnd = scopestart + codesize;
			}
		}

		for (int i = 0; i < gotos.Count; i++) {
			CompilerLabelData gd = gotos[i];

			if (!labels.HasKey(gd.Name)) {
				c.CurrentStatmentLine = gd.Line;
				c.ThrowError(CString::Format("Cannot find label '%s'", gd.Name.str_szBuffer));
			}

			CompilerLabelData ld = labels[gd.Name];
			if (ld.ScopeDept - gd.ScopeDept > 0) {
				for (int i = 0; i < strtbl.Count; i++) {
					delete strtbl[i];
				}
				delete[] code;

				c.CurrentStatmentLine = gd.Line;
				c.ThrowError("Cannot goto inside scopes, only outwards");
			}

			if (gd.Offset < ld.ScopeStart || gd.Offset > ld.ScopeEnd) {
				// overwrite scope to 0, just jump and hope the person has a "back" in there :v
				ld.ScopeDept = gd.ScopeDept;
				c.CurrentStatmentLine = gd.Line;
				c.ThrowWarning("Jumping inside of another function, unsafe!");
			}

			*(short*)(code + gd.Offset) = ld.ScopeDept - gd.ScopeDept;
			*(int*)(code + gd.Offset + 2) = ld.Offset;
		}

		byte* buff = new byte[codesize + c.Writer.Count];
		memcpy(buff, code, codesize);
		memcpy(buff + codesize, c.Writer.Buffer, c.Writer.Count);
		*outbytecodelength = codesize + c.Writer.Count;

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
			byte* buff = new byte[1];
			buff[0] = 0;
			*outbytecodelength = 1;

			return buff;
		}

		int brackets_a = 0;
		int brackets_b = 0;
		int brackets_c = 0;
		bool inquotes = false;

		int b_a_cs = 0;
		int b_b_cs = 0;
		int b_c_cs = 0;
		int q_cs = 0;

		int addline = this->Parent == null ? 1 : 0;

		for (int i = 0; i < this->ChunkSize; i++) {
			char c = this->CurrentChunk[i];

			switch (c) {
				case '[': if (brackets_a++ == 0) b_a_cs = i; break;
				case '{': if (brackets_b++ == 0) b_b_cs = i; break;
				case '(': if (brackets_c++ == 0) b_c_cs = i; break;

				case ']': if (--brackets_a < 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i) + addline; this->ThrowError(CString::Format("Square bracket count mismatch (-1)")); } break;
				case '}': if (--brackets_b < 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i) + addline; this->ThrowError(CString::Format("Curly bracket count mismatch (-1)")); } break;
				case ')': if (--brackets_c < 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, i) + addline; this->ThrowError(CString::Format("Round bracket count mismatch (-1)")); } break;

				case '"':
					if (i == 0 || this->CurrentChunk[i - 1] != '\\') {
						inquotes = !inquotes;
						if (inquotes) q_cs = i;
					}
					break;
			}
		}

		if (brackets_a > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_a_cs) + addline; this->ThrowError(CString::Format("Square bracket count mismatch (%d)", brackets_a)); }
		if (brackets_b > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_b_cs) + addline; this->ThrowError(CString::Format("Curly bracket count mismatch (%d)", brackets_b)); }
		if (brackets_c > 0) { this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, b_c_cs) + addline; this->ThrowError(CString::Format("Round bracket count mismatch (%d)", brackets_c)); }

		if (inquotes) {
			this->CurrentStatmentLine = this->GetCurrentLine(this->CurrentChunk, q_cs) + addline;
			this->ThrowError(CString::Format("Quote count mismatch"));
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
				else if (curline.StartsWith("class ") || curline.StartsWith("local class ")) this->WriteClass(curline);
				else if (curline.StartsWith("local ")) this->WriteSetVar(curline);
				else if (curline.StartsWith("while ") || curline.StartsWith("while(")) this->WriteWhile(curline);
				else if (curline.StartsWith("for ") || curline.StartsWith("for(")) this->WriteFor(curline);
				else if (curline.StartsWith("foreach ") || curline.StartsWith("foreach(")) this->WriteForeach(curline);
				else if (curline.StartsWith("if(") || curline.StartsWith("if (")) this->WriteIf(curline);
				else if (curline.StartsWith("loop(") || curline.StartsWith("loop (")) this->WriteLoop(curline);
				else if (curline.StartsWith("goto ")) this->WriteGoto(curline);
				else if (curline.EndsWith(":")) this->WriteLabel(curline);
				else if (curline == "continue") this->Writer.WriteByte(Misc::ExecFuncs::Continue);
				else if (curline == "break") this->Writer.WriteByte(Misc::ExecFuncs::Break);
				else if (curline == "rewind") this->Writer.WriteByte(Misc::ExecFuncs::Rewind);
				else if (curline == "return" || curline.StartsWith("return ")) this->WriteReturn(curline);
				else if (curline == "delete" || curline.StartsWith("delete ")) this->WriteDelete(curline);
				else this->WriteMisc(curline);
			}
		}

		this->Writer.WriteByte(Misc::ExecFuncs::End);
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

			if (currentindex % 1024 == 0) {
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

	byte* Compiler::GetArgumentData(CString line, int* size) {
		ByteWriter w;
		w.StringTable = this->Writer.StringTable;
		List<CString> args;

		char mathsplitters[] = {'+', '-', '*', '%', '/', '=', '!', '>', '<', '|', '&'};

		if (line.StartsWith('{') && line.EndsWith('}')) {
			args.Add(line);
		} else {
			char splitters[] = {'+', '-', '*', '%', '/', '>', '<', '|', '&'};

			char ignoreopp[] = {' ', '(', ')'};
			bool kakquiots = false, kakbrackets = false;
			int kakietsanders = 0, kakbrackets2 = 0;
			int curpos = 0;
			bool didop = false;
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

				if (!kakquiots && !kakbrackets && kakietsanders == 0 && kakbrackets2 == 0) {
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
					} else if (CharArrContains(line[i], splitters, sizeof(splitters)) && !didop) {
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
				if (args[i].StartsWith("(") && args[i] != "(") {
					args.Set(i, args[i].Substring(1));
					args.Insert(i, "(");
					notafunc = true;
					i++;
				}

				if (notafunc && args[i].EndsWith(')') && args[i] != ")") {
					args.Set(i, args[i].Substring(0, args[i].Size() - 1));
					args.Insert(i + 1, ")");

					i--;
				}
			}
		}

		w.WriteByte(Misc::ExecFuncs::MergeStart);
		int mergelenpos = w.Count;
		w.WriteInt(0);
		for (int i = 0; i < args.Count; i++) {
			CString arg = args[i];
			if (this->FindCharInSyntaxString(arg, ' ') > -1) {
				if (arg != "new" && !arg.StartsWith("new ") && !arg.StartsWith("function ")) {
					this->ThrowError(CString::Format("Cannot compute '%s'", arg.str_szBuffer));
				}
			}

			if (args[i] == "false") { w.WriteByte(Misc::ExecFuncs::Bool); w.WriteByte(0); }
			else if (args[i] == "true") { w.WriteByte(Misc::ExecFuncs::Bool); w.WriteByte(1); }
			else if (args[i] == "null") { w.WriteByte(Misc::ExecFuncs::Bool); w.WriteByte(2); }
			else if (args[i].StartsWith("enum")) {
				int argpos = this->FindChar(args[i], 0, "{", 1);
				if (argpos == -1)
					this->ThrowError("Invalid enum block");

				args.Set(i, args[i].Substring(argpos + 1, -1));

				List<CString> tblargs;
				this->GetFunctionData(args[i], ',', &tblargs);

				w.WriteByte(Misc::ExecFuncs::Enum);
				w.WriteInt(tblargs.Count);
				for (int i2 = 0; i2 < tblargs.Count; i2++) {
					int keypos = this->FindChar(tblargs[i2], 0, "=", 1);
					if (keypos != -1) {
						w.WriteString(tblargs[i2].Substring(0, keypos).Trim());
						w.WriteBool(true);

						int len = 0;
						byte* argpointer = this->GetArgumentData(tblargs[i2].Substring(keypos + 1).Trim(), &len);
						w.WriteBytes(argpointer, len, false);
						delete[] argpointer;
					} else {
						w.WriteString(tblargs[i2].Trim());
						w.WriteBool(false);
					}
				}
			} else if (args[i].StartsWith("function")) {
				CString* tmp = this->FindStringSpecial(line, "function", "{", 0);
				if (tmp == null)
					this->ThrowError("Invalid function block");

				CString funcline(tmp->Trim());
				delete tmp;

				int chunkpos = funcline.Size();

				CStackArray<CString> funcargs;
				int argpos = this->FindChar(funcline, 0, "(", 1);
				if (argpos > -1) {
					funcline.Substring(argpos + 1, funcline.Size() - 2 - argpos).Split(",", funcargs);

					for (int i = 0; i < funcargs.Count(); i++) {
						funcargs[i] = funcargs[i].Trim();

						if (funcargs[i].Size() == 0)
							delete funcargs.PopAt(i--);
					}
				}
				CString funcchunk = line.Substring(this->FindChar(line, 8, "{", 1) + 1, -1).Trim();

				List<CompilerLocalData> locals;
				Compiler chunkproc("AnonFunction", funcchunk.str_szBuffer, funcchunk.Size(), this->WriteLineNumbers, locals, this->Labels, this->Gotos, this->Warnings, this);


				w.WriteByte(Misc::ExecFuncs::AnonFunction);
				w.WriteString(this->Filename);
				this->Writer.Count += 4; // to fix label offsets
				int labelstart = this->Labels.Count();
				int scopestart = this->Writer.Count;

				byte* bytecode;
				int codelen = 0;
				try {
					bytecode = chunkproc.Run(&codelen, this->CurrentLineParent + GetCurrentLine(this->CurrentChunk, this->CurrentPos), this->Writer.StringTable);
				} catch (CompilerException e) {
					e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);
					e.CurrentLine += GetCurrentLine(this->CurrentChunk, this->CurrentPos);

					throw e;
				}

				this->Writer.Count -= 4; // to unfix label offsets
				for (int i = labelstart; i < this->Labels.Count(); i++) {
					CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

					if (ld.ScopeStart == -1) {
						ld.ScopeStart = scopestart;
						ld.ScopeEnd = scopestart + codelen;
					}
				}

				w.WriteBytes(bytecode, codelen, true);
				w.WriteStrings(funcargs, true);

				w.WriteInt(locals.Count);
				for (int i = 0; i < locals.Count; i++) {
					w.WriteString(locals[i].Name);
					w.WriteInt(locals[i].Type);
				}

				delete[] bytecode;
			} else if (args[i] == "new" || args[i].StartsWith("new ")) { // CSnew is NOT used in C++, so redirect this to 'new'
				if (args[i] == "new") {
					i++;
				} else {
					args.Set(i, args[i].Substring(4));
				}

				CString classname = "";
				CString funcargs = "";

				bool kakquiots = false;
				int kakietsanders = 0, kakietsanders2 = 0;
				int arglen = args[i].Size();
				List<CString> setsafter;
				for (int i2 = 0; i2 < arglen; i2++) {
					if (args[i][i2] == '"' && !(i2 > 0 && args[i][i2 - 1] == '\\'))
						kakquiots = !kakquiots;

					if (!kakquiots && args[i][i2] == '(')
						kakietsanders++;

					if (!kakquiots && args[i][i2] == ')')
						kakietsanders--;

					if (!kakquiots && args[i][i2] == '[')
						kakietsanders2++;

					if (!kakquiots && args[i][i2] == ']')
						kakietsanders2--;

					if (args[i][i2] == '(' && !kakquiots && kakietsanders == 1 && kakietsanders2 == 0) {
						classname = args[i].Substring(0, i2);

						if (args[i].EndsWith('}')) {
							int setspos = this->FindChar(args[i], i2, "{", 1);
							funcargs = args[i].Substring(i2 + 1, setspos - i2 - 2);
							this->GetFunctionData(args[i].Substring(setspos + 1, -1), ',', &setsafter);
						} else {
							funcargs = args[i].Substring(i2 + 1);
							funcargs = funcargs.Substring(0, funcargs.Size() - 1);
						}
						break;
					}
				}

				if (classname.Size() == 0)
					classname = args[i];

				classname = classname.Trim();

				List<CString> classargs;
				this->GetFunctionData(funcargs, ',', &classargs);

				int bytecodenamelen;
				byte* bytecodename = this->GetArgumentData(classname, &bytecodenamelen);

				w.WriteByte(Misc::ExecFuncs::New);
				w.WriteBytes(bytecodename, bytecodenamelen, false);

				delete[] bytecodename;

				w.WriteInt(classargs.Count);
				for (int i2 = 0; i2 < classargs.Count; i2++) {
					int len = 0;
					byte* argpointer = this->GetArgumentData(classargs[i2], &len);
					w.WriteBytes(argpointer, len, false);
					delete[] argpointer;
				}

				w.WriteInt(setsafter.Count);
				for (int i2 = 0; i2 < setsafter.Count; i2++) {
					CStackArray<CString> tmparr;
					setsafter[i2].Split("=", tmparr);

					w.WriteString(tmparr[0].Trim());

					int len = 0;
					byte* argpointer = this->GetArgumentData(tmparr[1].Trim(), &len);
					w.WriteBytes(argpointer, len, false);
					delete[] argpointer;

				}
			} else if (args[i] == "(") {
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

					dat += CString::Format(" %s", args[i2].str_szBuffer);
				}

				int len = 0;
				byte* argpointer = this->GetArgumentData(dat.Substring(1, dat.Size() - 1), &len);
				w.WriteByte(Misc::ExecFuncs::MergeStart);
				w.WriteInt(len + 6); // start byte, end byte, and 4 for block size
				w.WriteBytes(argpointer, len, false);
				w.WriteByte(Misc::ExecFuncs::MergeEnd);
				delete[] argpointer;
				continue;
			} else if (args[i].StartsWith("\"")) {
				args.Set(i, args[i].Substring(1));

				if (args[i].EndsWith('"') && !args[i].EndsWith("\\\""))
					args.Set(i, args[i].Substring(0, args[i].Size() - 1));

				w.WriteByte(Misc::ExecFuncs::String);
				w.WriteString(args[i]);

				i++;
			} else if (args[i].StartsWith("#")) {
				int len = 0;
				byte* argpointer = this->GetArgumentData(args[i].Substring(1, args[i].Size() - 1), &len);
				w.WriteByte(Misc::ExecFuncs::GetCount);
				w.WriteBytes(argpointer, len, false);
				delete[] argpointer;

				i++;
			} else if (i + 2 < args.Count && ((args[i + 1] == "+" && args[i + 2] == "+") || (args[i + 1] == "-" && args[i + 2] == "-"))) {
				w.WriteByte(args[i + 1] == "+" ? Misc::ExecFuncs::PostIncrement : Misc::ExecFuncs::PostDecrement);

				int len = 0;
				byte* argpointer = this->GetArgumentData(args[i], &len);
				w.WriteBytes(argpointer, len, false);
				delete[] argpointer;

				i += 3;
			} else if (i + 1 < args.Count && ((args[i] == "+" && args[i + 1][0] == '+') || (args[i] == "-" && args[i + 1][0] == '-'))) {
				w.WriteByte(args[i] == "+" ? Misc::ExecFuncs::PreIncrement : Misc::ExecFuncs::PreDecrement);

				int len = 0;
				byte* argpointer = this->GetArgumentData(args[i + 1].Substring(1), &len);
				w.WriteBytes(argpointer, len, false);
				delete[] argpointer;

				i += 3;
			} else if (args[i].StartsWith("{")) {
				args.Set(i, args[i].Substring(1, args[i].Size() - 2));

				List<CString> tblargs;
				this->GetFunctionData(args[i], ',', &tblargs);

				w.WriteByte(Misc::ExecFuncs::Table);
				w.WriteInt(tblargs.Count);
				for (int i2 = 0; i2 < tblargs.Count; i2++) {
					int keypos = this->FindChar(tblargs[i2], 0, "=", 1);
					if (keypos != -1) {
						w.WriteBool(true);
						w.WriteString(tblargs[i2].Substring(0, keypos).Trim());

						int len = 0;
						byte* argpointer = this->GetArgumentData(tblargs[i2].Substring(keypos + 1).Trim(), &len);
						w.WriteBytes(argpointer, len, false);
						delete[] argpointer;
					} else {
						w.WriteBool(false);
						int len = 0;
						byte* argpointer = this->GetArgumentData(tblargs[i2], &len);
						w.WriteBytes(argpointer, len, false);
						delete[] argpointer;
					}
				}

				break;
			} else if (CharIsNumber(args[i][0]) || ((args[i][0] == '-' || args[i][0] == '.') && args[i].Size() > 1 && CharIsNumber(args[i][1]))) {
				double num = 0;
				if (args[i].StartsWith("0x")) {
					char* p = null;
					num = strtoul(args[i].str_szBuffer + 2, &p, 16);
				} else if (args[i][0] == '-') {
					num = atof(args[i].Substring(1).str_szBuffer) * -1;
				} else {
					num = atof(args[i].str_szBuffer);
				}

				w.WriteByte(Misc::ExecFuncs::Number);
				w.WriteDouble(num);
			} else {
				List<CString> tbldata;
				this->GetIndexes(args[i], &tbldata);

				if (tbldata.Count > 1) {
					CString tblname = tbldata[0].Trim();
					if (tblname.EndsWith(')')) {
						bool kakquiots = false, kakbrackets = false;
						int kakietsanders = -1;

						int funclen = tblname.Size();
						for (int i3 = 0; i3 < funclen; i3++) {
							if (tblname[i3] == '"' && !(i3 > 0 && tblname[i3 - 1] == '\\'))
								kakquiots = !kakquiots;

							if (!kakquiots && tblname[i3] == '{')
								kakbrackets = true;

							if (!kakquiots && tblname[i3] == '}')
								kakbrackets = false;

							if (!kakquiots && !kakbrackets && tblname[i3] == '(') {
								kakietsanders++;

								if (kakietsanders == 0) {
									tbldata.Insert(1, tblname.Substring(i3));
									tblname = tblname.Substring(0, i3);
									break;
								}
							}

							if (!kakquiots && !kakbrackets && tblname[i3] == ')')
								kakietsanders--;
						}
					}

					tblname = tblname.Trim();

					w.WriteByte(Misc::ExecFuncs::GetTblIndex);
					int localindex = this->GetLocalIndex(tblname);
					if (localindex > -1) {
						w.WriteBool(true);
						w.WriteInt(localindex);
					} else {
						w.WriteBool(false);
						w.WriteString(tblname);
					}
					w.WriteByte(tbldata.Count - 1);

					for (int i2 = 1; i2 < tbldata.Count; i2++) {
						CString varname = tbldata[i2];
						CString funcline = varname;
						bool func = false;

						List<CString> tmpisfunctbl;
						this->GetIndexes(funcline, &tmpisfunctbl);

						if (tmpisfunctbl.Count == 1 && funcline.EndsWith(')')) {
							bool kakquiots = false, kakbrackets = false;
							int kakietsanders = -1;

							int funclen = funcline.Size();
							for (int i3 = 0; i3 < funclen; i3++) {
								if (funcline[i3] == '"' && !(i3 > 0 && funcline[i3 - 1] == '\\'))
									kakquiots = !kakquiots;

								if (!kakquiots && funcline[i3] == '{')
									kakbrackets = true;

								if (!kakquiots && funcline[i3] == '}')
									kakbrackets = false;

								if (!kakquiots && !kakbrackets && funcline[i3] == '(') {
									kakietsanders++;

									if (kakietsanders == 0) {
										func = true;
										funcline = funcline.Substring(i3 + 1, -1);
										varname = varname.Substring(0, i3);
										break;
									}
								}

								if (!kakquiots && !kakbrackets && funcline[i3] == ')')
									kakietsanders--;
							}
						}

						if (varname.Size() == 0)
							varname = "\"\"";

						varname = varname.Trim();

						int len = 0;
						byte* argpointer = this->GetArgumentData(varname, &len);
						w.WriteBytes(argpointer, len, false);
						delete[] argpointer;

						w.WriteByte(func ? 1 : 0);
						if (func) {
							List<CString> paras;
							this->GetFunctionData(funcline, ',', &paras);
							w.WriteInt(paras.Count);
							for (int i3 = 0; i3 < paras.Count; i3++) {
								int len = 0;
								byte* argpointer = this->GetArgumentData(paras[i3], &len);
								w.WriteBytes(argpointer, len, false);
								delete[] argpointer;
							}
						}
					}

					i++;
				} else if (!CharArrContains(args[i][0], mathsplitters, sizeof(mathsplitters))) {
					int funcpos = args[i].IndexOf('(');
					if (funcpos > -1 && CharIsLetter(args[i][0])) {
						CString funcname = args[i].Substring(0, funcpos);
						List<CString> paras;
						this->GetFunctionData(args[i].Substring(funcname.Size() + 1, args[i].Size() - funcname.Size() - 2), ',', &paras);

						w.WriteByte(Misc::ExecFuncs::ExecuteFunction);

						funcname = funcname.Trim();
						int localindex = this->GetLocalIndex(funcname);
						if (localindex > -1) {
							w.WriteBool(true);
							w.WriteInt(localindex);
						} else {
							w.WriteBool(false);
							w.WriteString(funcname);
						}
						w.WriteInt(paras.Count);
						for (int i2 = 0; i2 < paras.Count; i2++) {
							int len = 0;
							byte* argpointer = this->GetArgumentData(paras[i2], &len);
							w.WriteBytes(argpointer, len, false);
							delete[] argpointer;
						}
					} else {
						w.WriteByte(Misc::ExecFuncs::GetVar);

						CString localindexstr = args[i].Trim();
						int localindex = this->GetLocalIndex(localindexstr);
						if (localindex > -1) {
							w.WriteBool(true);
							w.WriteInt(localindex);
						} else {
							w.WriteBool(false);
							w.WriteString(args[i]);
						}
					}

					i++;
				}
			}

			if (i < args.Count) {
				CString c = args[i];
				bool check = false;
				if (c == "+") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Add); } else if (c == "-") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Substract); } else if (c == "/") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Divide); } else if (c == "*") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Multiply); } else if (c == "%") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Modulo); } else if (c == "&&") { check = true; w.WriteByte(Misc::ArithmaticFuncs::And); } else if (c == "||") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Or); } else if (c == ">") { check = true; w.WriteByte(Misc::ArithmaticFuncs::GreaterThan); } else if (c == ">=") { check = true; w.WriteByte(Misc::ArithmaticFuncs::GreaterOrEqual); } else if (c == "<") { check = true; w.WriteByte(Misc::ArithmaticFuncs::LessThan); } else if (c == "<=") { check = true; w.WriteByte(Misc::ArithmaticFuncs::LessOrEqual); } else if (c == "==") { check = true; w.WriteByte(Misc::ArithmaticFuncs::Equal); } else if (c == "!=") { check = true; w.WriteByte(Misc::ArithmaticFuncs::NotEqual); } else if (c == "<<") { check = true; w.WriteByte(Misc::ArithmaticFuncs::BitwiseLeft); } else if (c == ">>") { check = true; w.WriteByte(Misc::ArithmaticFuncs::BitwiseRight); } else if (c == "|") { check = true; w.WriteByte(Misc::ArithmaticFuncs::BitwiseOr); } else if (c == "&") { check = true; w.WriteByte(Misc::ArithmaticFuncs::BitwiseAnd); }

				if (check && i == args.Count - 1)
					this->ThrowError(CString::Format("Didn't expect a operator at the end of a statement! Statement is: '%s'", line.str_szBuffer));
			}
		}

		if (w.Count == 5)
			this->ThrowError(CString::Format("Invalid argument data: '%s'", line.str_szBuffer));

		w.WriteByte(Misc::ExecFuncs::MergeEnd);


		byte* ret = new byte[w.Count];
		memcpy(ret, w.Buffer, w.Count);
		*size = w.Count;

		memcpy(ret + mergelenpos, &w.Count, 4);

		return ret;
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
			}

			if (c == '"' && (i == 0 || line[i - 1] != '\\')) quote = !quote;
		}

		return -1;
	}

	void Compiler::ThrowError(CString err) {
		throw CompilerException(err, this->Filename, this->CurrentStatmentLine);
	}

	void Compiler::ThrowWarning(CString err) {
		this->Warnings.Add(CompilerException(err, this->Filename, this->CurrentStatmentLine));
	}

	void Compiler::WriteReturn(CString line) {
		this->Writer.WriteByte(Misc::ExecFuncs::Return);

		int argssize = 0;
		if (line.Size() > 6) {
			byte* argspointer = this->GetArgumentData(line.Substring(7, line.Size() - 7), &argssize);
			this->Writer.WriteBytes(argspointer, argssize, false);
			delete[] argspointer;
		} else this->Writer.WriteByte(0);
	}

	void Compiler::WriteDelete(CString line) {
		this->Writer.WriteByte(Misc::ExecFuncs::Delete);

		if (line.Size() < 7) this->ThrowError("Missing argument after delete keyword");
		CString arg = line.Substring(7);

		if (arg.Size() == 0) {
			int argssize = 0;
			byte* argspointer = this->GetArgumentData(arg, &argssize);
			this->Writer.WriteBytes(argspointer, argssize, false);
			delete[] argspointer;
		} else {
			this->ThrowError("Missing argument after delete keyword");
		}
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
				this->WriteSetVar(line);
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
		CStackArray<CString> funcargs;
		int argpos = this->FindChar(funcline, 0, "(", 1);
		if (argpos == -1)
			funcname = funcline;
		else {
			funcname = funcline.Substring(0, argpos);
			funcline.Substring(argpos + 1, funcline.Size() - 2 - argpos).Split(",", funcargs);

			for (int i = 0; i < funcargs.Count(); i++) {
				funcargs[i] = funcargs[i].Trim();

				if (funcargs[i].Size() == 0)
					delete funcargs.PopAt(i--);
			}
		}
		CString funcchunk = line.Substring(chunkpos + 1, -1).Trim();

		List<CString> tblindexes;
		this->GetIndexes(funcname, &tblindexes);
		for (int i = 0; i < tblindexes.Count; i++) {
			tblindexes.Set(i, tblindexes[i].Trim('\"'));
		}

		if (local) this->Locals.Add(CompilerLocalData(tblindexes[0], VariableType::Function));

		this->Writer.WriteByte(Misc::ExecFuncs::Function);
		this->Writer.WriteBool(local);

		if (local) {
			this->Writer.WriteInt(this->Locals.Count - 1);
		} else {
			CString localindexstr = tblindexes[0];
			int localindex = this->GetLocalIndex(localindexstr);
			if (localindex > -1) {
				this->Writer.WriteBool(true);
				this->Writer.WriteInt(localindex);
			} else {
				this->Writer.WriteBool(false);
				this->Writer.WriteString(tblindexes[0]);
			}
		}
		this->Writer.WriteString(this->Filename);

		List<CompilerLocalData> locals;

		for (int i = 0; i < funcargs.Count(); i++) {
			locals.Add(CompilerLocalData(funcargs[i], -1));
		}

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;
		Compiler chunkproc(funcname, funcchunk.str_szBuffer, funcchunk.Size(), this->WriteLineNumbers, locals, this->Labels, this->Gotos, this->Warnings, this);

		byte* bytecode;
		int codelen = 0;
		try {
			bytecode = chunkproc.Run(&codelen, this->CurrentLineParent + this->CurrentStatmentLine, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.Count -= 4; // to unfix label offsets
		for (int i = labelstart; i < this->Labels.Count(); i++) {
			CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

			if (ld.ScopeStart == -1) {
				ld.ScopeStart = scopestart;
				ld.ScopeEnd = scopestart + codelen;
			}
		}

		this->Writer.WriteBytes(bytecode, codelen, true);
		this->Writer.WriteStrings(funcargs, true);
		this->Writer.WriteInt(locals.Count);
		for (int i = 0; i < locals.Count; i++) {
			this->Writer.WriteString(locals[i].Name);
			this->Writer.WriteInt((byte)locals[i].Type);
		}
		this->Writer.WriteStrings(tblindexes, true);

		delete[] bytecode;
	}

	void Compiler::WriteWhile(CString line) {
		CString args = "";

		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos + 1, ")", 1);
			if (argendpos == -1)
				this->ThrowError("Error, no ) at while");

			args = line.Substring(argpos + 1, argendpos - argpos - 1);
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


		int bytecodeiflen;
		byte* bytecodeif = this->GetArgumentData(args, &bytecodeiflen);

		this->Writer.WriteByte(Misc::ExecFuncs::While);
		this->Writer.WriteBytes(bytecodeif, bytecodeiflen, true);

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc("while", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
		byte* bytecode;
		int codelen;
		try {
			bytecode = chunkproc.Run(&codelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.Count -= 4; // to unfix label offsets
		for (int i = labelstart; i < this->Labels.Count(); i++) {
			CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

			if (ld.ScopeStart == -1) {
				ld.ScopeStart = scopestart;
				ld.ScopeEnd = scopestart + codelen;
			}
		}

		this->Writer.WriteBytes(bytecode, codelen, true);
		this->Writer.WriteByte(0);

		delete[] bytecodeif;
		delete[] bytecode;
	}

	void Compiler::WriteLoop(CString line) {
		CString args = "";

		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos + 1, ")", 1);
			if (argendpos == -1) {
				this->ThrowError("Error, no ) at loop");
			}

			args = line.Substring(argpos + 1, argendpos - argpos - 1);
		}

		List<CString> fordata;
		this->GetFunctionData(args, ',', &fordata);

		if (fordata.Count != 2) {
			this->ThrowError("Invalid syntax for loop(key, loops)");
		}

		int chunkpos = this->FindChar(line, 0, "{", 1);
		CString codechunk;
		if (chunkpos == -1) {
			this->CurrentStatmentLine = -1;
			CString* tmp = this->ReadLine();
			if (tmp == null)
				this->ThrowError("No chunk at loop");

			codechunk = tmp->Trim();
			delete tmp;
		} else {
			codechunk = line.Substring(chunkpos + 1);
			codechunk = codechunk.Substring(0, codechunk.Size() - 1);
		}

		this->Locals.Add(CompilerLocalData(fordata[0], -1));

		this->Writer.WriteByte(Misc::ExecFuncs::Loop);
		this->Writer.WriteInt(this->Locals.Count - 1);

		int bytecodetbllen;
		byte* bytecodetbl = this->GetArgumentData(fordata[1], &bytecodetbllen);

		this->Writer.WriteBytes(bytecodetbl, bytecodetbllen, false);

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc("loop", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
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

		this->Writer.WriteInt(bytecodelen + 1);
		this->Writer.WriteBytes(bytecode, bytecodelen, false);
		this->Writer.WriteByte(0);

		delete[] bytecodetbl;
		delete[] bytecode;
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
			this->ThrowError("Invalid syntax for loop(key, loops)");
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


		this->Locals.Add(CompilerLocalData(fordata[0], -1));
		this->Locals.Add(CompilerLocalData(fordata[1], -1));

		this->Writer.WriteByte(Misc::ExecFuncs::ForEach);
		this->Writer.WriteInt(this->Locals.Count - 2);
		this->Writer.WriteInt(this->Locals.Count - 1);

		int bytecodetbllen;
		byte* bytecodetbl = this->GetArgumentData(fordata[2], &bytecodetbllen);

		this->Writer.WriteBytes(bytecodetbl, bytecodetbllen, false);

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc("foreach", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
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

		this->Writer.WriteInt(bytecodelen + 1);
		this->Writer.WriteBytes(bytecode, bytecodelen, false);
		this->Writer.WriteByte(0);

		delete[] bytecodetbl;
		delete[] bytecode;
	}

	void Compiler::WriteFor(CString line) {
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

		Compiler initproc("for (>><<, {}, {})", fordata[0].str_szBuffer, fordata[0].Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
		byte* bytecodeinit;
		int bytecodeinitlen;
		try {
			bytecodeinit = initproc.Run(&bytecodeinitlen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		Compiler endproc("for ({} {}, >><<)", fordata[2].str_szBuffer, fordata[2].Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
		byte* bytecodeend;
		int bytecodeendlen;
		try {
			bytecodeend = endproc.Run(&bytecodeendlen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		int bytecodelooplen;
		byte* bytecodeloop = this->GetArgumentData(fordata[1], &bytecodelooplen);

		this->Writer.WriteByte(Misc::ExecFuncs::For);
		this->Writer.WriteInt(bytecodeinitlen);
		this->Writer.WriteInt(bytecodelooplen);

		this->Writer.Count += bytecodeinitlen + bytecodelooplen + 8; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc("for", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
		byte* bytecode;
		int bytecodelen;
		try {
			bytecode = chunkproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
		} catch (CompilerException e) {
			e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);

			throw e;
		}

		this->Writer.Count -= bytecodeinitlen + bytecodelooplen + 8;; // to unfix label offsets
		for (int i = labelstart; i < this->Labels.Count(); i++) {
			CompilerLabelData& ld = this->Labels.GetValueByIndex(i);

			if (ld.ScopeStart == -1) {
				ld.ScopeStart = scopestart;
				ld.ScopeEnd = scopestart + bytecodelen;
			}
		}

		this->Writer.WriteInt(bytecodelen);
		this->Writer.WriteInt(bytecodeendlen);

		this->Writer.WriteBytes(bytecodeinit, bytecodeinitlen, false);
		this->Writer.WriteBytes(bytecodeloop, bytecodelooplen, false);
		this->Writer.WriteBytes(bytecode, bytecodelen, false);
		this->Writer.WriteBytes(bytecodeend, bytecodeendlen, false);

		delete[] bytecodeinit;
		delete[] bytecodeloop;
		delete[] bytecode;
		delete[] bytecodeend;
	}

	void Compiler::WriteMisc(CString line) {
		int setvarpos = this->FindChar(line, 0, "=", 1);

		if (setvarpos > -1) {
			this->WriteSetVar(line);
		} else if (line.EndsWith(')')) {
			List<CString> tbldata;
			this->GetIndexes(line, &tbldata);

			if (tbldata.Count > 1) {
				CString tblname = tbldata[0].Trim();
				if (tblname.EndsWith(')')) {
					bool kakquiots = false, kakbrackets = false;
					int kakietsanders = -1;

					int funclen = tblname.Size();
					for (int i3 = 0; i3 < funclen; i3++) {
						if (tblname[i3] == '"' && !(i3 > 0 && tblname[i3 - 1] == '\\'))
							kakquiots = !kakquiots;

						if (!kakquiots && tblname[i3] == '{')
							kakbrackets = true;

						if (!kakquiots && tblname[i3] == '}')
							kakbrackets = false;

						if (!kakquiots && !kakbrackets && tblname[i3] == '(') {
							kakietsanders++;

							if (kakietsanders == 0) {
								tbldata.Insert(1, tblname.Substring(i3));
								tblname = tblname.Substring(0, i3);
								break;
							}
						}

						if (!kakquiots && !kakbrackets && tblname[i3] == ')')
							kakietsanders--;
					}
				}

				this->Writer.WriteByte(Misc::ExecFuncs::GetTblIndex);
				int localindex = this->GetLocalIndex(tblname);
				if (localindex > -1) {
					this->Writer.WriteBool(true);
					this->Writer.WriteInt(localindex);
				} else {
					this->Writer.WriteBool(false);
					this->Writer.WriteString(tblname);
				}
				this->Writer.WriteByte((tbldata.Count - 1));

				for (int i2 = 1; i2 < tbldata.Count; i2++) {
					CString varname = tbldata[i2].Trim();
					CString funcline = varname;
					bool func = false;

					List<CString> tmpisfunctbl;
					this->GetIndexes(funcline, &tmpisfunctbl);

					if (tmpisfunctbl.Count == 1 && funcline.EndsWith(')')) {
						bool kakquiots = false, kakbrackets = false;
						int kakietsanders = -1;

						int funclen = funcline.Size();
						for (int i3 = 0; i3 < funclen; i3++) {
							if (funcline[i3] == '"' && !(i3 > 0 && funcline[i3 - 1] == '\\'))
								kakquiots = !kakquiots;

							if (!kakquiots && funcline[i3] == '{')
								kakbrackets = true;

							if (!kakquiots && funcline[i3] == '}')
								kakbrackets = false;

							if (!kakquiots && !kakbrackets && funcline[i3] == '(') {
								kakietsanders++;

								if (kakietsanders == 0) {
									func = true;
									funcline = funcline.Substring(i3 + 1, -1);
									varname = varname.Substring(0, i3).Trim();
									break;
								}
							}

							if (!kakquiots && !kakbrackets && funcline[i3] == ')')
								kakietsanders--;
						}
					}

					if (varname.Size() == 0)
						varname = "\"\"";

					int len = 0;
					byte* argpointer = this->GetArgumentData(varname, &len);
					this->Writer.WriteBytes(argpointer, len, false);
					delete[] argpointer;

					this->Writer.WriteByte(func ? 1 : 0);
					if (func) {
						List<CString> paras;
						this->GetFunctionData(funcline, ',', &paras);
						this->Writer.WriteInt(paras.Count);
						for (int i3 = 0; i3 < paras.Count; i3++) {
							int len = 0;
							byte* argpointer = this->GetArgumentData(paras[i3], &len);
							this->Writer.WriteBytes(argpointer, len, false);
							delete[] argpointer;
						}
					}
				}
			} else {
				CString funcname = line.Substring(0, line.IndexOf('('));
				CString funcline = line.Substring(funcname.Size() + 1, line.Size() - funcname.Size() - 2);

				funcname = funcname.Trim();

				List<CString> paras;
				this->GetFunctionData(funcline, ',', &paras);

				this->Writer.WriteByte(Misc::ExecFuncs::ExecuteFunction);
				int localindex = this->GetLocalIndex(funcname);
				if (localindex > -1) {
					this->Writer.WriteBool(true);
					this->Writer.WriteInt(localindex);
				} else {
					this->Writer.WriteBool(false);
					this->Writer.WriteString(funcname);
				}
				this->Writer.WriteInt(paras.Count);
				for (int i2 = 0; i2 < paras.Count; i2++) {
					int len = 0;
					byte* argspointer = this->GetArgumentData(paras[i2], &len);
					this->Writer.WriteBytes(argspointer, len, false);
					delete[] argspointer;
				}
			}
		} else if (line.EndsWith("++")) {
			this->Writer.WriteByte(Misc::ExecFuncs::PostIncrement);
			int len = 0;
			byte* argspointer = this->GetArgumentData(line.Substring(0, -2), &len);
			this->Writer.WriteBytes(argspointer, len, false);
			delete[] argspointer;
		} else if (line.EndsWith("--")) {
			this->Writer.WriteByte(Misc::ExecFuncs::PostDecrement);
			int len = 0;
			byte* argspointer = this->GetArgumentData(line.Substring(0, -2), &len);
			this->Writer.WriteBytes(argspointer, len, false);
			delete[] argspointer;
		} else if (line.StartsWith("++")) {
			this->Writer.WriteByte(Misc::ExecFuncs::PreIncrement);
			int len = 0;
			byte* argspointer = this->GetArgumentData(line.Substring(2), &len);
			this->Writer.WriteBytes(argspointer, len, false);
			delete[] argspointer;
		} else if (line.StartsWith("--")) {
			this->Writer.WriteByte(Misc::ExecFuncs::PreDecrement);
			int len = 0;
			byte* argspointer = this->GetArgumentData(line.Substring(2), &len);
			this->Writer.WriteBytes(argspointer, len, false);
			delete[] argspointer;
		} else {
			int len = 0;
			byte* argspointer = this->GetArgumentData(line, &len);
			this->Writer.WriteBytes(argspointer, len, false);
			delete[] argspointer;
		}
	}

	int Compiler::GetLocalIndex(CString& key) {
		for (int i = this->Locals.Count - 1; i >= 0; i--) {
			if (this->Locals[i].Name == key) {
				return i;
			}
		}

		return -1;
	}

	void Compiler::WriteSetVar(CString line) {
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
		}

		byte* argpointer;

		List<CString> tbldata;
		this->GetIndexes(name, &tbldata);

		if (tbldata.Count > 1) {
			if (local) {
				this->ThrowError("You can't localize a table index");
			}

			List<CString> tblindexes;
			this->GetIndexes(name, &tblindexes);

			this->Writer.WriteByte(Misc::ExecFuncs::SetTblIndex);
			CString localindexstr = tblindexes[0];
			int localindex = this->GetLocalIndex(localindexstr);
			if (localindex > -1) {
				this->Writer.WriteBool(true);
				this->Writer.WriteInt(localindex);
			} else {
				this->Writer.WriteBool(false);
				this->Writer.WriteString(tblindexes[0]);
			}

			this->Writer.WriteByte(tblindexes.Count - 1);
			for (int i2 = 1; i2 < tblindexes.Count; i2++) {
				CString varname = tbldata[i2];
				CString funcline = varname;
				bool func = false;

				List<CString> tmpisfunctbl;
				this->GetIndexes(funcline, &tmpisfunctbl);

				if (tmpisfunctbl.Count == 1 && funcline.EndsWith(')')) {
					bool kakquiots = false, kakbrackets = false;
					int kakietsanders = -1;

					int funclen = funcline.Size();
					for (int i3 = 0; i3 < funclen; i3++) {
						if (funcline[i3] == '"' && !(i3 > 0 && funcline[i3 - 1] == '\\'))
							kakquiots = !kakquiots;

						if (!kakquiots && funcline[i3] == '{')
							kakbrackets = true;

						if (!kakquiots && funcline[i3] == '}')
							kakbrackets = false;

						if (!kakquiots && !kakbrackets && funcline[i3] == '(') {
							kakietsanders++;

							if (kakietsanders == 0) {
								func = true;

								funcline = funcline.Substring(i3 + 1, -1);
								varname = varname.Substring(0, i3);
								break;
							}
						}

						if (!kakquiots && !kakbrackets && funcline[i3] == ')')
							kakietsanders--;
					}
				}

				if (varname.Size() == 0)
					varname = "\"\"";

				int len = 0;
				byte* argpointer = this->GetArgumentData(varname, &len);
				this->Writer.WriteBytes(argpointer, len, false);
				delete[] argpointer;

				this->Writer.WriteByte(func ? 1 : 0);
				if (func) {
					List<CString> paras;
					this->GetFunctionData(funcline, ',', &paras);
					this->Writer.WriteInt(paras.Count);
					for (int i3 = 0; i3 < paras.Count; i3++) {
						int len = 0;
						byte* argpointer = this->GetArgumentData(paras[i3], &len);
						this->Writer.WriteBytes(argpointer, len, false);
						delete[] argpointer;
					}
				}
			}
		} else {
			if (local) {
				this->Locals.Add(CompilerLocalData(name, vartype));

				this->Writer.WriteByte(Misc::ExecFuncs::LSetVar);
				this->Writer.WriteInt(this->Locals.Count - 1);
			} else {
				this->Writer.WriteByte(Misc::ExecFuncs::SetVar);

				int localindex = this->GetLocalIndex(name);
				if (localindex > -1) {
					this->Writer.WriteBool(true);
					this->Writer.WriteInt(localindex);
				} else {
					this->Writer.WriteBool(false);
					this->Writer.WriteString(name);
				}
			}
		}

		int len = 0;
		argpointer = this->GetArgumentData(restdata, &len);
		this->Writer.WriteBytes(argpointer, len, false);
		delete[] argpointer;
	}

	void Compiler::WriteIf(CString line) {
		int argstartpos = this->FindChar(line, 0, "(", 1);
		if (argstartpos == -1)
			this->ThrowError("Error, no ( at if");

		int argpos = this->FindChar(line, argstartpos + 1, ")", 1);
		if (argpos == -1)
			this->ThrowError("Error, no ) at if");

		int chunkpos = this->FindChar(line, argpos + 1, "{", 1);
		// function<space> = 9, + 1 for { and - 1 for }

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

		CString* nextline = this->Peekline();
		byte nextif = 0;

		if (nextline != null) {
			if (nextline->StartsWith("elseif (") || nextline->StartsWith("elseif(") || (nextline->StartsWith("elseif") && nextline->Size() == 6))
				nextif = 1;
			else if (nextline->StartsWith("else{") || nextline->StartsWith("else {") || (nextline->StartsWith("else") && nextline->Size() == 4))
				nextif = 2;

			delete nextline;
		}

		int arglen;
		byte* argdata = this->GetArgumentData(line.Substring(argstartpos + 1, argpos - argstartpos - 1), &arglen);

		this->Writer.WriteByte(Misc::ExecFuncs::If);
		this->Writer.WriteByte(nextif);
		this->Writer.WriteBytes(argdata, arglen, true);

		this->Writer.Count += 4; // to fix label offsets
		int labelstart = this->Labels.Count();
		int scopestart = this->Writer.Count;

		Compiler chunkproc("if statement", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
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

		delete[] argdata;
		delete[] bytecode;

		while (nextif != 0) {
			CString* tmp = this->ReadLine();

			line = CString(tmp);
			delete tmp;

			int chunkpos = this->FindChar(line, 0, "{", 1);
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

			nextif = 0;
			nextline = this->Peekline();

			if (nextline != null) {
				if (nextline->StartsWith("elseif (") || nextline->StartsWith("elseif(") || (nextline->StartsWith("elseif") && nextline->Size() == 6))
					nextif = 1;
				else if (nextline->StartsWith("else{") || nextline->StartsWith("else {") || (nextline->StartsWith("else") && nextline->Size() == 4))
					nextif = 2;

				delete nextline;
			}

			if (line.StartsWith("elseif (") || line.StartsWith("elseif(") || line == "elseif") {
				int argstartpos = this->FindChar(line, 0, "(", 1);
				if (argstartpos == -1)
					this->ThrowError("Error, no ( at if");

				int argpos = this->FindChar(line, argstartpos + 1, ")", 1);
				if (argpos == -1)
					this->ThrowError("Error, no ) at if");


				argdata = this->GetArgumentData(line.Substring(argstartpos + 1, argpos - argstartpos - 1), &arglen);

				this->Writer.WriteByte(Misc::ExecFuncs::ElseIf);
				this->Writer.WriteByte(nextif);
				this->Writer.WriteBytes(argdata, arglen, true);

				this->Writer.Count += 4; // to fix label offsets
				int labelstart = this->Labels.Count();
				int scopestart = this->Writer.Count;

				Compiler elseifproc("elseif statement", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
				try {
					bytecode = elseifproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
				} catch (CompilerException e) {
					e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);
					e.CurrentLine += this->CurrentStatmentLine;

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

				delete[] argdata;
				delete[] bytecode;
			} else if (line.StartsWith("else{") || line.StartsWith("else {") || line == "else") {

				this->Writer.WriteByte(Misc::ExecFuncs::Else);
				this->Writer.Count += 4; // to fix label offsets
				int labelstart = this->Labels.Count();
				int scopestart = this->Writer.Count;

				Compiler elseproc("else statement", codechunk.str_szBuffer, codechunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);
				try {
					bytecode = elseproc.Run(&bytecodelen, this->CurrentStatmentLine + this->CurrentLineParent, this->Writer.StringTable);
				} catch (CompilerException e) {
					e.CurrentFile = CString::Format("%s - %s", this->Filename.str_szBuffer, e.CurrentFile.str_szBuffer);
					e.CurrentLine += this->CurrentStatmentLine + this->CurrentLineParent;

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
		}
	}

	void Compiler::WriteClass(CString line) {
		bool local = line.StartsWith("local ");
		if (local)
			line = line.Substring(6);

		CString name;
		CString args;
		int argpos = this->FindChar(line, 0, "(", 1);
		if (argpos != 0) {
			int argendpos = this->FindChar(line, argpos + 1, ")", 1);
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

		Compiler chunkproc(name, chunk.str_szBuffer, chunk.Size(), this->WriteLineNumbers, this->Locals, this->Labels, this->Gotos, this->Warnings, this);

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

	void Compiler::WriteGoto(CString line) {
		line = line.Substring(5).Trim();

		for (int i = 0; i < line.Size(); i++) {
			char c = line[i];

			if ((c <= 'a' || c <= 'z') && (c <= 'A' && c >= 'Z')) {
				this->ThrowError("Invalid label name. Labels cannot contain any non-letters");
			}
		}

		int dept = -1;
		int pos = 0;
		Compiler* uplink = this;
		while (uplink != null) {
			pos += uplink->Writer.Count;
			dept++;

			uplink = uplink->Parent;
		}

		this->Writer.WriteByte(Misc::ExecFuncs::Goto);
		this->Gotos.Add(CompilerLabelData(line, pos + 1, dept, this->CurrentStatmentLine + this->CurrentLineParent));
		this->Writer.WriteShort(0); // stack balance
		this->Writer.WriteInt(0); // jmp pos
	}

	void Compiler::WriteLabel(CString line) {
		line = line.Substring(0, line.Size() - 1);

		for (int i = 0; i < line.Size(); i++) {
			char c = line[i];

			if ((c <= 'a' || c <= 'z') && (c <= 'A' && c >= 'Z')) {
				this->ThrowError("Invalid label name. Labels cannot contain any non-letters");
			}
		}

		int dept = -1;
		int pos = 0;
		Compiler* uplink = this;
		while (uplink != null) {
			pos += uplink->Writer.Count;
			dept++;

			uplink = uplink->Parent;
		}

		this->Labels[line] = CompilerLabelData(line, pos, dept, this->CurrentStatmentLine + this->CurrentLineParent);
	}
}
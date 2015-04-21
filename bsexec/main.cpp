#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#include <BromScript.h>
#include <string>

#ifndef _MSC_VER
#include <readline/readline.h>
#include <readline/history.h>
#endif

BS_FUNCTION(Print) {
	std::string out;
	for (int i = 0; i < args->Count; i++) {
		if (i > 0)
			out += '\t';

		out += BromScript::Converter::VariableToString(bsi, args->GetVariable(i));
	}

	printf("%s\n", out.c_str());
	return null;
}

BS_ERRORFUNCTION(errfunc) {
	printf("BromScript error!\n");

	for (int i = 0; i < stacksize; i++) {
		printf("SID: %i, File: %s:%d, Name: %s\n", i, stack[i].Filename.str_szBuffer, stack[i].LineNumber, stack[i].Name.str_szBuffer);
	}

	printf("Message: %s\n", msg);
}

char * getline() {
	char* line = (char*)malloc(100), *linep = line;
	size_t lenmax = 100, len = lenmax;
	int c;

	if (line == NULL)
		return NULL;

	while (true) {
		c = fgetc(stdin);
		if (c == EOF)
			break;

		if (--len == 0) {
			len = lenmax;
			char* linen = (char*)realloc(linep, lenmax *= 2);

			if (linen == NULL) {
				free(linep);
				return NULL;
			}
			line = linen + (line - linep);
			linep = linen;
		}

		if ((*line++ = c) == '\n')
			break;
	}

	*line = '\0';
	return linep;
}

#define OUTPUT_SUCCESS(type, file, line, msg) line > -1 ? (formated ? fprintf(stderr, "SUCCESS::%s:%s:%d:%s\n", type, file, line, msg) : fprintf(stderr, "%s success::%s:%d:%s\n", type, file, line, msg)) : (formated ? fprintf(stderr, "SUCCESS::%s:%s:?:%s\n", type, file, msg) : fprintf(stderr, "%s success::%s:?:%s\n", type, file, msg))
#define OUTPUT_WARNING(type, file, line, msg) line > -1 ? (formated ? fprintf(stderr, "WARNING::%s:%s:%d:%s\n", type, file, line, msg) : fprintf(stderr, "%s warning::%s:%d:%s\n", type, file, line, msg)) : (formated ? fprintf(stderr, "WARNING::%s:%s:?:%s\n", type, file, msg) : fprintf(stderr, "%s warning::%s:?:%s\n", type, file, msg))
#define OUTPUT_NOTICE(type, file, msg) formated ? fprintf(stderr, "NOTICE::%s:%s:%s\n", type, file, msg) : fprintf(stderr, "%s notice::%s:%s\n", type, file, msg)
#define OUTPUT_ERROR(type, file, line, msg) line > -1 ? (formated ? fprintf(stderr, "ERROR::%s:%s:%d:%s\n", type, file, line, msg) : fprintf(stderr, "%s error::%s:%d:%s\n", type, file, line, msg)) : (formated ? fprintf(stderr, "ERROR::%s:%s:?:%s\n", type, file, msg) : fprintf(stderr, "%s error::%s:?:%s\n", type, file, msg))
#define DOSLEEPIFNEEDED while (sleepatend && fgetc(stdin) != '\n') {}

int main(int argc, char* argv[]) {
	bool shouldcompile = false;
	bool shouldrun = true;
	bool debug = true;
	bool runstring = false;
	bool formated = false;
	bool sleepatend = false;
	bool openshell = true;
	bool forceopenshell = false;
	bool warningsareerrors = false;

	int outi = -1;

	std::string runstr;
	std::string outname = "out.cbs";

	BromScript::List<BromScript::CompilerException> warnings;

	for (int i = 1; i < argc; i++) {
		if (outi == i) continue;

		std::string arg = argv[i];
		if (runstring) {
			runstr += arg + " ";

			if (i + 1 == argc) {
				BromScript::Instance bsi;
				bsi.LoadDefaultLibaries();

				bsi.SetErrorCallback(errfunc);
				bsi.RegisterFunction("print", Print);

				try {
					bsi.DoString("cmdargs", runstr.c_str());
				} catch (BromScript::RuntimeException err) {
					BromScript::Function* func = bsi.GetCurrentFunction();
					if (func != null) {
						OUTPUT_ERROR("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
					} else {
						OUTPUT_ERROR("Runtime", "?", -1, err.Message.str_szBuffer);
					}

				} catch (BromScript::CompilerException err) {
					OUTPUT_ERROR("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
				}

				DOSLEEPIFNEEDED;
				return 0;
			}

			continue;
		}

		if (arg[0] == '-') {
			for (unsigned int i2 = 0; i2 < arg.size(); i2++) {
				switch (arg[i2]) {
					case 'i': forceopenshell = true; break;
					case 'w': sleepatend = true; break;
					case 'd': debug = false; break;
					case 'f': formated = true; break;
					case 'r': shouldrun = true; break;
					case 'n': warningsareerrors = true; break;

					case 'c':
						shouldcompile = true;
						shouldrun = false;
						runstring = false;
						break;


					case 's':
						shouldcompile = false;
						shouldrun = false;
						runstring = true;
						break;

					case 'o':
						i++;

						if (i == argc) {
							printf("argument after -o flag expected\n");
							return 1;
						}

						outname = argv[i];
						outi = i;
						i--;
						break;

					case 'h':
						printf("No arguments for shell\n");
						printf("-c -- compile\n");
						printf("-d -- diable debug bytecode\n");
						printf("-o -- outfile for compiler, defaults to 'out.cbs'\n");
						printf("-r -- run compiled code after compile, add after -c flag\n");
						printf("-s -- run string, all arguments after that will be converted read as code, watch out with quotes\n");
						printf("-f -- formated output\n");
						printf("-n -- treat warnings as errors\n");
						printf("-w -- sleep until newline at stdin after tasks\n");
						printf("-i -- open interpreter shell\n");
						printf("last argument should be the file you want to run\n");
						printf("If you just want to execute a file, just supply the filename\n");
						printf("\n");
						printf("Examples:\n");
						printf("bsexec -cdrwo compiled.cbs humancode.bs\n");
						printf("bsexec humancode.bs\n");
						printf("bsexec -s \"print(\\\"hello\\\")\"\n");
						printf("bsexec -cdwo out1.cbs in1.bs -o out2.cbs in2.bs -o out3.cbs in3.bs\n");
						return 0;
				}
			}
		} else {
			openshell = false;

			if (shouldcompile) {
				Scratch::CFileStream fs;
				if (!fs.Open(arg.c_str(), "rb")) {
					OUTPUT_ERROR("Compiling", arg.c_str(), -1, "Cannot open file");
					return 0;
				}

				int filesize = fs.Size();
				char* buff = (char*)fs.Read(filesize);
				fs.Close();

				unsigned char* bytecode;

				try {
					bytecode = BromScript::Compiler::Run(arg.c_str(), buff, filesize, &filesize, debug, true, warnings);
				} catch (BromScript::CompilerException err) {
					OUTPUT_ERROR("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
					DOSLEEPIFNEEDED;
					return 0;
				}

				for (int i2 = 0; i2 < warnings.Count; i2++) {
					BromScript::CompilerException w = warnings[i2];
					if (warningsareerrors) {
						OUTPUT_ERROR("Compiling", w.CurrentFile.str_szBuffer, w.CurrentLine, w.Message.str_szBuffer);
					} else {
						OUTPUT_WARNING("Compiling", w.CurrentFile.str_szBuffer, w.CurrentLine, w.Message.str_szBuffer);
					}
				}

				if (warningsareerrors && warnings.Count > 0) {
					DOSLEEPIFNEEDED;
					return 0;
				}

				remove(outname.c_str());
				FILE* f = fopen(outname.c_str(), "wb");
				if (f == null) {
					OUTPUT_ERROR("Compiling", outname.c_str(), -1, "Cannot write file");
					DOSLEEPIFNEEDED;
					return 0;
				}

				fwrite(bytecode, 1, filesize, f);
				fclose(f);

				delete[] bytecode;
				delete[] buff;

				OUTPUT_NOTICE("Compiling", arg.c_str(), Scratch::CString::Format("Done compiling to %s", outname.c_str()));
				arg = outname;
			}

			if (shouldrun) {
				BromScript::Instance bsi;
				bsi.LoadDefaultLibaries();

				bsi.SetErrorCallback(errfunc);
				bsi.RegisterFunction("print", Print);

				try {
					bsi.DoFile(arg.c_str());
				} catch (BromScript::RuntimeException err) {
					BromScript::Function* func = bsi.GetCurrentFunction();
					if (func != null) {
						OUTPUT_ERROR("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
					} else {
						OUTPUT_ERROR("Runtime", "?", -1, err.Message.str_szBuffer);
					}

					DOSLEEPIFNEEDED;
					return 0;
				} catch (BromScript::CompilerException err) {
					OUTPUT_ERROR("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
					DOSLEEPIFNEEDED;
					return 0;
				}

				OUTPUT_NOTICE("Runtime", arg.c_str(), Scratch::CString::Format("Done executing %s", arg.c_str()));
			}

			if (i + 1 == argc) {
				DOSLEEPIFNEEDED;
				return 0;
			}
		}
	}

	if (!openshell && !forceopenshell) return 0;

	BromScript::Instance bsi;
	bsi.LoadDefaultLibaries();

	bsi.SetErrorCallback(errfunc);
	bsi.RegisterFunction("print", Print);

	printf("Bromscript shell\n> ");

	char* strptr;

	while (true) {
#ifdef _MSC_VER
		strptr = getline();
#else
		strptr = readline("");
#endif

		if (strptr == NULL) {
			return 1;
		}

#ifndef _MSC_VER
		char *expansion;
		int result;

		result = history_expand(strptr, &expansion);
		if (result)
			fprintf(stderr, "%s\n", expansion);

		if (result < 0 || result == 2) {
			free(expansion);
			continue;
		}

		add_history(expansion);
		strncpy(strptr, expansion, strlen(strptr));
		free(expansion);
#endif

		try {
			BromScript::Variable* var = bsi.DoString("stdin", strptr);
			if (var != null) {
				printf("%s\n", var->ToString().str_szBuffer);
			}
		} catch (BromScript::RuntimeException err) {
			BromScript::Function* func = bsi.GetCurrentFunction();
			if (func != null) {
				OUTPUT_ERROR("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
			} else {
				OUTPUT_ERROR("Runtime", "?", -1, err.Message.str_szBuffer);
			}
		} catch (BromScript::CompilerException err) {
			OUTPUT_ERROR("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
		}

		printf("> ");
		delete strptr;

	}

	return 0;
}

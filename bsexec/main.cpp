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

bool shouldcompile = false;
bool shouldrun = true;
bool debug = true;
bool runstring = false;
bool formated = false;
bool sleepatend = false;
bool openshell = true;
bool forceopenshell = false;
bool warningsareerrors = false;
bool attachdebugger = false;

class CVector {
public:
	double x;
	double y;
	double z;
};

// Our wrappers 
// Constructor of the class, return a pointer 
BS_FUNCTION_CTOR(CVectorCtor) {
	CVector* ct = new CVector();

	ct->x = args->GetNumber(0);
	ct->y = args->GetNumber(1);
	ct->z = args->GetNumber(2);

	// returning null here is fine too, BS will detect it and it will revert the variable to a null type 
	return ct;
}

// deconstructor, data is a pointer, delete this here, and do other stuff if you want to. 
BS_FUNCTION_DTOR(CVectorDtor) {
	// warning, if you dont delete your data here, then you'll have to clean it up yourself, not deleting it
	// would be logical for an entity system to prevent crashes
	// like checking in the entitiy functions itself if a flag has been set 
	delete (CVector*)data;
}

// Wrapper functions 
BS_FUNCTION(CVectorSetFunc) {
	CVector* ct = (CVector*)args->GetUserdata(0, 999);
	ct->x = args->GetNumber(1);
	ct->y = args->GetNumber(2);
	ct->z = args->GetNumber(3);

	return null;
}

BS_FUNCTION(CVectorGetXFunc) {
	CVector* ct = (CVector*)args->GetUserdata(0, 999);
	return BromScript::Converter::ToVariable(bsi, ct->x); // again, returning null here is fine. 
}

BS_FUNCTION(CVectorGetYFunc) {
	CVector* ct = (CVector*)args->GetUserdata(0, 999);
	return BromScript::Converter::ToVariable(bsi, ct->y);
}

BS_FUNCTION(CVectorGetZFunc) {
	CVector* ct = (CVector*)args->GetUserdata(0, 999);
	return BromScript::Converter::ToVariable(bsi, ct->z);
}

BS_FUNCTION(CVector__AddOperator) {
	// You can also register Operators, these work the same way as normal BSFunctions 
	// However, they always get called with 2 arguments, a + b, a is index 0, b is index 1. 
	// As example, a is an CVector class, and b is a number. you can do a + b
	// this will make the second argument a number, and the first one a CVector 
	// However, if you do b + a, the same function also gets called. at that moment
	// the 0 index argument will be the number and not the CVector 
	// So you'll need to typecheck to see what a is, and what b is. 

	if (!args->CheckType(0, 999)) return null;
	if (!args->CheckType(1, 999)) return null;

	CVector* leftvalue = (CVector*)args->GetUserdata(0, 999);
	CVector* rightvalue = (CVector*)args->GetUserdata(1, 999);

	CVector* newvec = new CVector();
	newvec->x = leftvalue->x + rightvalue->x;
	newvec->y = leftvalue->y + rightvalue->y;
	newvec->z = leftvalue->z + rightvalue->z;

	return args->BromScript->CreateUserdata(999, newvec, true);
}

void OutputSuccess(const char* type, const char* file, int line, const char* msg) {
	if (line > -1) {
		if (formated) {
			fprintf(stderr, "SUCCESS::%s:%s:%d:%s\n", type, file, line, msg);
		} else {
			fprintf(stderr, "%s success::%s:%d:%s\n", type, file, line, msg);
		}
	} else {
		if (formated) {
			fprintf(stderr, "SUCCESS::%s:%s:?:%s\n", type, file, msg);
		} else {
			fprintf(stderr, "%s success::%s:?:%s\n", type, file, msg);
		}
	}
}

void OutputWarning(const char* type, const char* file, int line, const char* msg) {
	if (line > -1) {
		if (formated) {
			fprintf(stderr, "WARNING::%s:%s:%d:%s\n", type, file, line, msg);
		} else {
			fprintf(stderr, "%s warning::%s:%d:%s\n", type, file, line, msg);
		}
	} else {
		if (formated) {
			fprintf(stderr, "WARNING::%s:%s:?:%s\n", type, file, msg);
		} else {
			fprintf(stderr, "%s warning::%s:?:%s\n", type, file, msg);
		}
	}
}

void OutputError(const char* type, const char* file, int line, const char* msg) {
	if (line > -1) {
		if (formated) {
			fprintf(stderr, "ERROR::%s:%s:%d:%s\n", type, file, line, msg);
		} else {
			fprintf(stderr, "%s error::%s:%d:%s\n", type, file, line, msg);
		}
	} else {
		if (formated) {
			fprintf(stderr, "ERROR::%s:%s:?:%s\n", type, file, msg);
		} else {
			fprintf(stderr, "%s error::%s:?:%s\n", type, file, msg);
		}
	}
}

void OutputNotice(const char* type, const char* file, const char* msg) {
	if (formated) {
		fprintf(stderr, "NOTICE::%s:%s:%s\n", type, file, msg);
	} else {
		fprintf(stderr, "%s notice::%s:%s\n", type, file, msg);
	}
}

int DoSleepIfNeeded() {
	while (sleepatend && fgetc(stdin) != '\n') { }
	return 0;
}

int main(int argc, char* argv[]) {
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
						OutputError("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
					} else {
						OutputError("Runtime", "?", -1, err.Message.str_szBuffer);
					}

				} catch (BromScript::CompilerException err) {
					OutputError("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
				}

				return DoSleepIfNeeded();
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
					case 'a': attachdebugger = true; break;

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
						printf("-d -- disable debug bytecode\n");
						printf("-o -- outfile for compiler, defaults to 'out.cbs'\n");
						printf("-r -- run compiled code after compile, add after -c flag\n");
						printf("-s -- run string, all arguments after that will be converted read as code, watch out with quotes\n");
						printf("-f -- formatted output\n");
						printf("-n -- treat warnings as errors\n");
						printf("-w -- sleep until newline at stdin after tasks\n");
						printf("-i -- open interpreter shell\n");
						printf("-a -- attach debugger pre-execution\n");
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
					OutputError("Compiling", arg.c_str(), -1, "Cannot open file");
					return 0;
				}

				int filesize = fs.Size();
				char* buff = (char*)fs.Read(filesize);
				fs.Close();

				unsigned char* bytecode;
				
				try {
					bytecode = BromScript::Compiler::Run(outname.c_str(), buff, filesize, &filesize, debug, true, warnings);
				} catch (BromScript::CompilerException err) {
					OutputError("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
					return DoSleepIfNeeded();
				}

				for (int i2 = 0; i2 < warnings.Count; i2++) {
					BromScript::CompilerException w = warnings[i2];
					if (warningsareerrors) {
						OutputError("Compiling", w.CurrentFile.str_szBuffer, w.CurrentLine, w.Message.str_szBuffer);
					} else {
						OutputWarning("Compiling", w.CurrentFile.str_szBuffer, w.CurrentLine, w.Message.str_szBuffer);
					}
				}

				if (warningsareerrors && warnings.Count > 0) {
					return DoSleepIfNeeded();
				}

				remove(outname.c_str());
				FILE* f = fopen(outname.c_str(), "wb");
				if (f == null) {
					OutputError("Compiling", outname.c_str(), -1, "Cannot write file");
					return DoSleepIfNeeded();
				}

				fwrite(bytecode, 1, filesize, f);
				fclose(f);

				delete[] bytecode;
				delete[] buff;

				OutputNotice("Compiling", arg.c_str(), Scratch::CString::Format("Done compiling to %s", outname.c_str()).str_szBuffer);
				arg = outname;
			}

			if (shouldrun) {
				BromScript::Instance bsi;
				bsi.LoadDefaultLibaries();

				bsi.SetErrorCallback(errfunc);
				bsi.RegisterFunction("print", Print);
				
				// Register everything in the bsi, the CTor and DTor can be null, it'll throw an error inside BS when the user
				// tries to create a type, and it will just not call the dtor incase that's null(watch out with leaks here!) 
				BromScript::Userdata* vd = bsi.RegisterUserdata("Vector", 999, sizeof(CVector), CVectorCtor, CVectorDtor);

				// add indexes for x, y and x so we can do "vector.x = 15"
				// Don't use string as membertype unless you're also supplying a getter and setter, using them witouth will cause memory leaks
				// BS has no idea if it should delete old ones, or keep them in memory
				// should it allocate and put a pointer back? or is it an fixed sized array? We dont know!
				vd->RegisterMember("x", offsetof(CVector, x), BromScript::MemberType::Double);
				vd->RegisterMember("y", offsetof(CVector, y), BromScript::MemberType::Double);
				vd->RegisterMember("z", offsetof(CVector, z), BromScript::MemberType::Double);

				// Our wrapper funcs 
				vd->RegisterFunction("Set", CVectorSetFunc);
				vd->RegisterFunction("GetX", CVectorGetXFunc);
				vd->RegisterFunction("GetY", CVectorGetYFunc);
				vd->RegisterFunction("GetZ", CVectorGetZFunc);

				// Our add operator
				vd->RegisterOperator(BromScript::Operators::ArithmeticAdd, CVector__AddOperator);

				// And now, we can use the following code to create a new Vector in BromScript 
				// local vec = new Vector() 

				if (attachdebugger) {
					if (!bsi.Debug->Connect()) {
						OutputWarning("Runtime", arg.c_str(), -1, "Failed to attach pre-execution debugger");
					} else {
						OutputNotice("Runtime", arg.c_str(), "Attach pre-execution debugger");
					}
				}

				try {
					bsi.DoFile(arg.c_str());
				} catch (BromScript::RuntimeException err) {
					BromScript::Function* func = bsi.GetCurrentFunction();
					if (func != null) {
						OutputError("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
					} else {
						OutputError("Runtime", "?", -1, err.Message.str_szBuffer);
					}

					return DoSleepIfNeeded();
				} catch (BromScript::CompilerException err) {
					OutputError("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
					return DoSleepIfNeeded();
				}

				OutputNotice("Runtime", arg.c_str(), Scratch::CString::Format("Done executing %s", arg.c_str()).str_szBuffer);
			}

			if (i + 1 == argc) {
				return DoSleepIfNeeded();
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
				OutputError("Runtime", func->Filename.str_szBuffer, func->CurrentSourceFileLine, err.Message.str_szBuffer);
			} else {
				OutputError("Runtime", "?", -1, err.Message.str_szBuffer);
			}
		} catch (BromScript::CompilerException err) {
			OutputError("Compiling", err.CurrentFile.str_szBuffer, err.CurrentLine, err.Message.str_szBuffer);
		}

		printf("> ");
		delete strptr;

	}

	return 0;
}

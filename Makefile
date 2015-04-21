INCLUDES =
LIBS =
OPTIMIZEFLAG=-O0
sourcefiles = 
objfiles = 

.PHONY: all
all: bslib bsexec

.PHONY: build_BSLIB_settings
build_BSLIB_settings:
	$(eval sourcefiles := $(shell find BromScript/ -name "*.cpp"))
	$(eval objfiles := $(patsubst %.cpp,%.o,$(sourcefiles)))
	$(eval LIBS = -lm -L/usr/lib64 -lstdc++ -lpthread -ldl)
	$(eval INCLUDES = -fpermissive -w -shared)
	
.PHONY: build_BSEXEC_settings
build_BSEXEC_settings:
	$(eval sourcefiles := $(shell find bsexec/ -name "*.cpp"))
	$(eval objfiles := $(patsubst %.cpp,%.o,$(sourcefiles)))
	$(eval LIBS = -lreadline -lBromScript -lm -L/usr/lib64 -lstdc++ -lpthread -ldl)
	$(eval INCLUDES = -IBromScript/ -fpermissive -w)
	
.PHONY: release_set_flag
release_set_flag:
	$(eval OPTIMIZEFLAG=-O3)

.PHONY: bsexec
bsexec: build_BSEXEC_settings internal_bsexec
.PHONY: bslib
bslib: build_BSLIB_settings internal_bslib

.PHONY: release
release: release_set_flag all

.PHONY: clean
clean:
	rm -f $(shell find . -name "*.o") bin/libBromScript.so bin/bsexec

.PHONY: internal_bslib
internal_bslib: $(objfiles)
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) -o bin/libBromScript.so $(objfiles) $(LIBS)

.PHONY: internal_bsexec
internal_bsexec: $(objfiles)
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) -o bin/bsexec $(objfiles) $(LIBS)

%.o : %.cpp
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) $(LIBS) -c $< -o $@
INCLUDES		= -IBromScript/ -fpermissive -w
LIBS			= -lm -L/usr/lib64 -lstdc++ -lpthread -ldl
OPTIMIZEFLAG	= -O0

sourcefiles_bromscript := $(shell find BromScript/ -name "*.cpp")
objfiles_bromscript := $(patsubst %.cpp,%.o,$(sourcefiles_bromscript))

sourcefiles_bsexec := $(shell find bsexec/ -name "*.cpp")
objfiles_bsexec := $(patsubst %.cpp,%.o,$(sourcefiles_bsexec))

.PHONY: all
all: bin/libBromScript.so bin/bsexec

bin/libBromScript.so: $(objfiles_bromscript)
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) -shared -o bin/libBromScript.so $(objfiles_bromscript) $(LIBS)

bin/bsexec: $(objfiles_bsexec)
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) -o bin/bsexec $(objfiles_bsexec) -lreadline -lBromScript $(LIBS)

%.o : %.cpp
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(INCLUDES) $(LIBS) -c $< -o $@

.PHONY: clean all release
clean:
	rm -f $(shell find . -name "*.o") bin/libBromScript.so bin/bsexec
	
release_set_flag:
	$(eval OPTIMIZEFLAG=-O3)

release: release_set_flag clean all
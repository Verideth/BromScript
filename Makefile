INCLUDES		= -IBromScript/
LIBS			= -Lbin -lm -L/usr/lib64 -lstdc++ -lpthread -ldl
OPTIMIZEFLAG	= -O0
CXXFLAGS		= -fpermissive -w -fPIC

sourcefiles_bromscript := $(shell find BromScript/ -name "*.cpp")
objfiles_bromscript := $(patsubst %.cpp,%.o,$(sourcefiles_bromscript))

sourcefiles_bsexec := $(shell find bsexec/ -name "*.cpp")
objfiles_bsexec := $(patsubst %.cpp,%.o,$(sourcefiles_bsexec))

print-%: ; @echo $*=$($*)

.PHONY: all
all: bin/libBromScript.so bin/bsexec
#bin/libBromScript.a

bin/libBromScript.so: $(objfiles_bromscript)
	mkdir -p bin lib
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(CXXFLAGS) $(INCLUDES) -shared -o bin/libBromScript.so $(objfiles_bromscript) $(LIBS)

install:
	cp bin/libBromScript.so /lib

#bin/libBromScript.a: $(objfiles_bromscript)
#	ar -cvq bin/libBromScript.a $(objfiles_bromscript)

bin/bsexec: $(objfiles_bsexec)
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(CXXFLAGS) $(INCLUDES) -o bin/bsexec $(objfiles_bsexec) -lreadline $(LIBS) -lBromScript

%.o : %.cpp
	$(CXX) -g -ggdb -std=c++11 $(OPTIMIZEFLAG) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@

.PHONY: clean all release
clean:
	rm -f $(shell find . -name "*.o") bin/libBromScript.so bin/bsexec
	rm -r bin lib
	
release_set_flag:
	$(eval OPTIMIZEFLAG=-O3)

release: release_set_flag clean all

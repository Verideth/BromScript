LIBS			= -lm -L/usr/lib64 -lstdc++ -ldl
FLAGS           = -fpermissive

sourcefiles		:= $(shell find . -name "*.cpp")
objfiles		:= $(patsubst %.cpp,%.o,$(sourcefiles))

all : main

main: $(objfiles)
	rm -f bromscript.a
	ar -cvq bromscript.a $(objfiles)

%.o : %.cpp
	$(CXX) -c -std=c++11 -ggdb -w $(LIBS) $(FLAGS) -m32 -o $@ $<

.PHONY: clean

clean:
	rm -f $(shell find . -name "*.o") 

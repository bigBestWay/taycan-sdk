CC       = g++
OBJ      = example.o memshell.o
LINKOBJ  = example.o memshell.o
LIBS     = -ltaycan
LINKDIR  = -Llib
JDK_HOME = ${JAVA_HOME}
INCS     = -I$(JDK_HOME)/include -I$(JDK_HOME)/include/linux
CXXINCS  = 
BIN      = libmy.so
CXXFLAGS = $(CXXINCS)  -shared
CFLAGS   = $(INCS) -Wall -fpic -O2
RM       = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) $(CXXFLAGS) -o $(BIN) $(LIBS) $(LINKDIR)

example.o: example.cpp
	$(CC) -c example.cpp -o example.o $(CFLAGS)

memshell.o: memshell.cpp
	$(CC) -c memshell.cpp -o memshell.o $(CFLAGS)


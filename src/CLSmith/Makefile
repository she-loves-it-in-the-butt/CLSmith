# Makefile for the CLSmith. Only handles files found int src/CLSmith/. Before
# calling this, you must first call make in src/ and then delete
# RandomProgramGenerator.o (for now at least).

CC=g++
CFLAGS=-c -Wall -I../ -std=c++0x -g
LFLAGS=-std=c++0x
SOURCES=CLOutputMgr.cpp CLProgramGenerator.cpp Globals.cpp CLRandomProgramGenerator.cpp Walker.cpp Divergence.cpp CLExpression.cpp CLStatement.cpp CLVariable.cpp StatementBarrier.cpp MemoryBuffer.cpp Vector.cpp CLOptions.cpp ExpressionVector.cpp ExpressionAtomic.cpp StatementEMI.cpp StatementAtomicResult.cpp FunctionInvocationBuiltIn.cpp ExpressionID.cpp StatementComm.cpp StatementAtomicReduction.cpp StatementMessage.cpp
OBJS=$(filter-out ../csmith-RandomProgramGenerator.o, $(wildcard ../*.o)) $(SOURCES:.cpp=.o)
BIN=CLSmith

all: generator launcher

generator: $(SOURCES) $(BIN)

launcher: cl_launcher.c
	gcc -Wall -I/homes/$(USER)/OpenCL/ cl_launcher.c -g -o cl_launcher -lOpenCL

$(BIN): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o

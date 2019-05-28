CC = g++
LD = g++
RISCVAR = riscv64-unknown-elf-ar
RISCVCC = riscv64-unknown-elf-gcc -march=rv64i
RISCVLD = riscv64-unknown-elf-ld
INCPATH = ./src
INCBOOST = /usr/local/include
LIBBOOST = /usr/local/lib
RISCVLIBDIR = ./mylib

all: simu libmyc.a ackermann add double-float matrix-mul mul-div n! qsort simple-function

simu: main.o machine.o bitmap.o riscvsim.o pred.o cache.o memory.o
	$(LD) -I $(INCPATH) -I $(INCBOOST) -L $(LIBBOOST) -D_GLIBCXX_USE_CXX11_ABI=0 -o simu main.o machine.o bitmap.o riscvsim.o pred.o cache.o memory.o -lboost_program_options

main.o: ./src/main.cpp
	$(CC) -I $(INCPATH) -I $(INCBOOST) -c -o main.o ./src/main.cpp

cache.o: ./src/cache.cc ./src/cache.h ./src/storage.h
	$(CC) -I $(INCPATH) -c -o cache.o ./src/cache.cc

memory.o: ./src/memory.cc ./src/memory.h ./src/storage.h
	$(CC) -I $(INCPATH) -c -o memory.o ./src/memory.cc

machine.o: ./src/machine.h  ./src/bitmap.h ./src/pred.h ./src/machine.cpp
	$(CC) -I $(INCPATH) -c -o machine.o ./src/machine.cpp

bitmap.o: ./src/bitmap.h ./src/bitmap.cpp
	$(CC) -I $(INCPATH) -c -o bitmap.o ./src/bitmap.cpp

riscvsim.o:	./src/machine.h ./src/pred.h ./src/riscvsim.cpp
	$(CC) -I $(INCPATH) -c -o riscvsim.o ./src/riscvsim.cpp

pred.o:	./src/pred.h ./src/pred.cpp 
	$(CC) -I $(INCPATH) -c -o pred.o ./src/pred.cpp


libmyc.a: syscall.o
	$(RISCVAR) crv ./mylib/libmyc.a ./mylib/syscall.o

syscall.o: ./mylib/syscall.h ./mylib/syscall.c
	$(RISCVCC) -c -o ./mylib/syscall.o ./mylib/syscall.c

add: add.o 
	$(RISCVCC) -L $(RISCVLIBDIR) -o add add.o -lmyc

add.o: ./userprog/add.c ./mylib/syscall.h
	$(RISCVCC) -c -I ./mylib -o add.o ./userprog/add.c

ackermann: ./userprog/ackermann.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o ackermann ./userprog/ackermann.c -lmyc

double-float: ./userprog/double-float.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o double-float ./userprog/double-float.c -lmyc

matrix-mul: ./userprog/matrix-mul.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o matrix-mul ./userprog/matrix-mul.c -lmyc

mul-div: ./userprog/mul-div.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o mul-div ./userprog/mul-div.c -lmyc

n!: ./userprog/n!.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o n! ./userprog/n!.c -lmyc

qsort: ./userprog/qsort.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o qsort ./userprog/qsort.c -lmyc

simple-function:  ./userprog/simple-function.c ./mylib/syscall.h
	$(RISCVCC) -I ./mylib -L $(RISCVLIBDIR) -o simple-function ./userprog/simple-function.c -lmyc
 
clean:
	rm *.o simu

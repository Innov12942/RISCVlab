#include "syscall.h"

void sleep_(int t){
    asm("addi a7, zero, 88");
    asm("ecall");
}
void printfint(int x){
    asm("addi a7, zero, 89");
    asm("ecall");
}

void printfchar(char c){
	asm("addi a7, zero, 90");
	asm("ecall");
}

void printfstr(char *str){
	asm("addi a7, zero, 91");
	asm("ecall");
}

int scanfint(){
    int result;
	asm("addi a7, zero, 92");
	asm("ecall");
    asm("addi %0, a0, 0" : "=r" (result));
}

void exit_(int ec)
{
	asm("addi a7, zero, 93");
	asm("ecall");
}


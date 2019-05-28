#ifndef SYSCALL_H
#define SYSCALL_H

void sleep_(int t);		// %a7=88
void printfint(int x); 		// %a7=89
void printfchar(char c);	// %a7=90
void printfstr(char *str);	// %a7=91
int  scanfint();		// %a7=92
void exit_(int ec);		// $a7=93

#endif


#ifndef MACHINE_H
#define MACHINE_H

#include "bitmap.h"
#include "pred.h"
#include "memory.h"
#include "cache.h"
#include <time.h>
#include <stdio.h>
#include <map>

#define REG_NUM 32
#define PAGE_SIZE 4096
#define PYS_PAGE_NUM 1024
#define MEM_SIZE (PAGE_SIZE * PYS_PAGE_NUM)
#define STACK_PAGES 10

/*instruction num -- a little more than real*/
#define INSTRNUM 60
/*read write num*/
#define WRNUM 10
#define RMEM 1
#define WMEM 2
#define RCACHE 3
#define WCACHE 4

/*registers*/
#define ZEROREG 0
#define RAREG 1
#define SPREG 2
#define A7Reg  17
#define A0Reg 10

/*page table entry*/
typedef struct{
    uint64_t vpn;
    int ppn;
    int timestamp;
    bool valid;
}pageEntry;

/*Instruction*/
typedef struct{
    uint64_t addr;
    uint32_t ival;
    uint32_t opcode;
    int name;
    int type;
}Instruction;

/*pipline register*/
typedef struct{
    Instruction instr;
    int64_t srcA;   /*rs1*/
    int64_t srcB;   /*rs2*/
    int64_t valA;   /*reg[rs1]*/
    int64_t valB;   /*reg[rs2]*/
    int64_t valC;
    int64_t valE;   /*ALU result*/
    int64_t valM;   /*value from memory waiting to be written into register*/
    int64_t imm;    /*imm[:]*/
    int64_t dstE;
    int64_t dstM;

    bool predJ;     /*branch prediction*/
    bool bubble;
    bool stall;
}PipReg;

/*machine stats*/
typedef struct{
    int cycle;
    int instrCnt;
    int dataHazard;
    int loadUseHazard;
    int controlHazard;
    long long misPrediction;
    long long sucPrediction;
    clock_t stTime;
    clock_t edTime;
    int ecallNum;
    int FSTALL;
}stat;

class Machine{

public: 
    Machine();     

    ~Machine();
    /*cpu environment*/
    int64_t registers[REG_NUM];
    uint64_t stackTop;
    uint64_t PC;

    /*some stats about machine*/
    int machineCycle;
    stat machineStats;
    int TicksPerCycle;
    int instrPfm[INSTRNUM]; /*instrucition performance*/
    int memPfm[WRNUM];      /*memory performance*/

    /*userprogram in memory?*/
    bool loadSuccess;

    /*debug flag*/
    bool sgStep;
    bool debug;


    Predictor MyPred;               /*branch predictor*/

    /*memory related*/
    Memory PhyMem;
    StorageLatency Memory_latency;
    Cache L1;
    CacheConfig L1_config;
    StorageLatency L1_latency;
    Cache L2;
    CacheConfig L2_config;
    StorageLatency L2_latency;
    Cache LLC;
    CacheConfig LLC_config;
    StorageLatency LLC_latency;
   // char *mainMem;                  /*main memory in host*/
    int pageNum;                    /*physical page number*/

    BitMap *bmp;                    /*bitmap for managing physical page*/
    std::map<uint64_t, pageEntry> ptb;  /*page table*/

    /*reading user program*/
    bool ReadUserProg(const char *fileName);
    void PfmConfig(FILE *f);

    /*reading byte(s) from main memory[addr] into val*/
    void readBytes(uint64_t addr, int nbytes, void *val);
    void writeBytes(uint64_t addr, int nbytes, void *val);

    /*translate virtual address*/
    uint64_t translateAddr(uint64_t virAddr);

    /*syscall*/
    void syscall();

    /*machine operations when starting and ending*/
    void StackAllocate();
    void Run();
    void Halt();

    /*select pred scheme*/
    void SetPredScheme(Scheme s);
    void SetCacheConfig();

    void printSingleStep();
    void printReg();
    void printPipe();

private:
    /*page operation*/
    void AllocPageEntry(uint64_t vpn);
    void AllocPysPage(pageEntry &pe);

    /*pipeline related*/
private:
    uint64_t predPC;
    bool forwardA;   /*Was data forwarding already did by the former stage?*/
    bool forwardB;

    /*pipeline register*/
    PipReg FReg;
    PipReg DReg;
    PipReg DRegO;   /*output signal by fetch*/
    PipReg EReg;
    PipReg ERegO;
    PipReg MReg;
    PipReg MRegO;
    PipReg WReg;
    PipReg WRegO;

    void Fetch();
    void Decode();
    void Execute();
    void MemStage();
    void Writeback();

};
#endif


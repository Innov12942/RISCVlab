#include <elfio/elfio.hpp>
#include "machine.h"
#include "utils.h"
#include <stdio.h>

using namespace std;

Machine::Machine() : PhyMem(MEM_SIZE), L1(), L2(), LLC(){
    for(int i = 0; i < REG_NUM; i++)
        registers[i] = 0;

    stackTop = 0;
    machineCycle = 0;
    loadSuccess = false;
    pageNum = MEM_SIZE / PAGE_SIZE + 1;

    /*initialize bmp & page table*/
    bmp = new BitMap(pageNum);
    MyPred = Predictor(); 
    ptb.clear();

    machineStats.cycle = 0;
    machineStats.dataHazard = 0;
    machineStats.loadUseHazard = 0;
    machineStats.controlHazard = 0;
    machineStats.sucPrediction = 0;
    machineStats.instrCnt = 0;
    machineStats.misPrediction = 0;
    machineStats.ecallNum = 0;
    machineStats.FSTALL = 0;

    for(int i = 0; i < INSTRNUM; i++)
        instrPfm[i] = 1;
    for(int i = 0; i < WRNUM; i++)
        memPfm[i] = 1;

    /*cache and memory*/
    StorageStats s;
    s.access_counter = 0;
    s.access_time = 0;
    s.miss_num = 0;
    s.access_time = 0; 
    s.replace_num = 0; 
    s.fetch_num = 0;
    s.prefetch_num = 0; 
    PhyMem.SetStats(s);
    L1.SetStats(s);
    L2.SetStats(s);
    LLC.SetStats(s);

    /*default config for memory and cache*/

    //memory latency for reading userprog
    Memory_latency.bus_latency = 50;
    Memory_latency.hit_latency = 0;
    PhyMem.SetLatency(Memory_latency);

    //L1 latency
    L1_latency.bus_latency = 0;
    L1_latency.hit_latency = 1; /*CPU cycle*/

    //L2 latency
    L2_latency.bus_latency = 0;
    L2_latency.hit_latency = 1; /*CPU cycle*/

    //LLC latency
    LLC_latency.bus_latency = 0;
    LLC_latency.hit_latency = 20; /*CPU cycle*/

    //L1 cache config
    L1_config.size = 32 * 1024;
    L1_config.associativity = 8;
    L1_config.set_num = L1_config.size / (L1_config.associativity * BLOCK_SIZE);
    if(L1_config.size % (L1_config.associativity * BLOCK_SIZE) != 0){
        printf("Please give proper cache config!\n");
        ASSERT(false);
    }
    L1_config.write_through = 0;
    L1_config.write_allocate = 0;

    L2_config = L1_config;
    LLC_config = L1_config;
}

Machine::~Machine(){
    delete bmp;
}

void
Machine::SetCacheConfig(){
    /*clear stats*/
    StorageStats s;
    s.access_counter = 0;
    s.access_time = 0;
    s.miss_num = 0;
    s.access_time = 0; 
    s.replace_num = 0; 
    s.fetch_num = 0;
    s.prefetch_num = 0; 
    PhyMem.SetStats(s);
    L1.SetStats(s);
    L2.SetStats(s);
    LLC.SetStats(s);


    PhyMem.SetLatency(Memory_latency);
    L1.SetLatency(L1_latency);
    L2.SetLatency(L2_latency);
    LLC.SetLatency(LLC_latency);

    L1.SetConfig(L1_config);
    L2.SetConfig(L2_config);
    LLC.SetConfig(LLC_config);

    L1.SetLower(&L2);
    L2.SetLower(&LLC);
    LLC.SetLower(&PhyMem);
}

void
Machine::SetPredScheme(Scheme s){
    MyPred.SetScheme(s);
}


void
Machine::readBytes(uint64_t addr, int nbytes, void *val){
    ASSERT(addr + nbytes < MEM_SIZE + 1);
    int hit, time;
    L1.HandleRequest(addr, nbytes, 1, (char *)val, hit, time);

    if(time > TicksPerCycle)
        TicksPerCycle = time;

    if(debug)
        printf("Usrprog readBytes at:%lx Use cpu cycles:%d\n", addr, time);
}

void 
Machine::writeBytes(uint64_t addr, int nbytes, void *val){
    ASSERT(addr + nbytes < MEM_SIZE + 1);
    int hit, time;
    L1.HandleRequest(addr, nbytes, 0, (char *)val, hit, time);
    
    if(time > TicksPerCycle)
        TicksPerCycle = time;

    if(debug)
        printf("Usrprog writeBytes at:%lx Use cpu cycles:%d\n", addr, time);
}


void
Machine::AllocPageEntry(uint64_t vpn){
    pageEntry pe;
    pe.vpn = vpn;
    pe.valid = false;
    if(ptb.find(vpn) != ptb.end())
        return;
    ptb.insert(pair<int, pageEntry>(vpn, pe));
}

void
Machine::AllocPysPage(pageEntry &pe){
    if(pe.valid)
        return;
    int onepp = bmp->FindSet();
    if(onepp == -1){
        printf("Not enough memory!\n");
        abort();
    }
    pe.ppn = onepp;
    pe.valid = true;
}

uint64_t
Machine::translateAddr(uint64_t virAddr){
    uint64_t vpn = virAddr / PAGE_SIZE;

    std::map<uint64_t, pageEntry>::iterator itr = ptb.find(vpn);
    if(itr == ptb.end()){
        printf("PageFault Exception at %llx at cycle:%d\n", virAddr, machineCycle);
        fflush(stdout);
        printf("%s\n", "virtual Address has no corresponding page entry!");
        ASSERT(false);
    }
    pageEntry tarEntry = itr->second;
    ASSERT(tarEntry.valid);
    int pysPage = tarEntry.ppn;
    uint64_t offset = virAddr % PAGE_SIZE;
    uint64_t finalPA = pysPage * PAGE_SIZE + offset;
    if(finalPA < 0){
        printf("virAddr:%lu pysPage:%d offset:%d\n", virAddr, pysPage, offset);
        fflush(stdout);
    }
    ASSERT(finalPA < MEM_SIZE);
    ASSERT(finalPA >= 0)
    return finalPA;
}

bool
Machine::ReadUserProg(const char *fileName){
    ELFIO::elfio reader;
    if(!reader.load(fileName)) {
        fprintf(stderr, "Fail to load %s!\n", fileName);
        return false;
    }
    printf("Reading user program...\n");
    int segNum = reader.segments.size();
    printf("segNum:%d\n", segNum);
    uint64_t initPC = reader.get_entry();

    this->PC = initPC;
    this->predPC = initPC;

    for (int i = 0; i < segNum; ++i){
        const ELFIO::segment *pseg = reader.segments[i];
        uint64_t vaddr = pseg->get_virtual_address();
        uint64_t fileSize = pseg->get_file_size();
        uint64_t memSize = pseg->get_memory_size();

        uint64_t segPageNum = memSize / PAGE_SIZE;
        uint64_t vpn = vaddr / PAGE_SIZE;
        while((vpn + segPageNum) * PAGE_SIZE < vaddr + memSize)
            segPageNum ++;

        printf("seg:%d vaddr:%lu fileSize:%lu memSize:%lu segPages:%lu\n",     \
             i, vaddr, fileSize, memSize, segPageNum);

        for(int k = 0; k < segPageNum; k++){
            AllocPageEntry(vpn + k);
            AllocPysPage((ptb.find(vpn + k)->second));
        }

        const char *seg_data = pseg->get_data();
        char *seg_data_buf = new char[fileSize];
        for(int k = 0; k < fileSize; k++)
            seg_data_buf[k] = seg_data[k];
        for(int k = 0; k < fileSize; k++){
            uint64_t virAddr = vaddr + k;
            uint64_t pysAddr = translateAddr(virAddr);
            int hit, time;
            PhyMem.HandleRequest(pysAddr, 1, 0, seg_data_buf + k, hit, time);
        }
        delete[] seg_data_buf;
    }
    loadSuccess = true;
    return true;
}

void
Machine::StackAllocate(){
    uint64_t stackTop = 0x80000000;
    uint64_t vpn = stackTop / PAGE_SIZE;

    for(int i = -1; i < STACK_PAGES; i++){
        AllocPageEntry(vpn - i);
        AllocPysPage(ptb.find(vpn - i)->second);
    }

    registers[SPREG] = stackTop;
}

void
Machine::Run(){
    if(!loadSuccess){
        printf("please load user program!\n");
        return;
    }
    printf("program start running\n");
    printf("initialize stack\n");

    StackAllocate();
    SetCacheConfig();

    /*set bubble and stall*/
    FReg.bubble = false;
    FReg.stall = false;
    DReg.bubble = true;
    DReg.stall = false;
    EReg.bubble = true;
    EReg.stall = false;
    MReg.bubble = true;
    MReg.stall = false;
    WReg.bubble = true;
    WReg.stall = false;

    printf("start pipeline...\n\n");
    if(sgStep){
        printf("usage:\n");
        printf("c               ----continue runing next instruction\n");
        printf("rn              ----read register[n]\n");
        printf("m <addr> <size> ----read size bytes in memory at 0x0\n");
        printf("i               ----show instruction in five stage\n");
        printf("s <cycle>       ----skip certain cycles\n");
        printf("h               ----stop\n");                          
    }

    machineStats.stTime = clock();
    while(true){
        TicksPerCycle = 1;
        /*print INFO*/
        if(sgStep || debug)
            printf("\n<<<<cycle:%d>>>>>\n", machineCycle);

        /*five stage*/
        Fetch();
        Decode();
        Execute();
        MemStage();
        Writeback();
      

        if(debug){
            printReg();
            printPipe();
        }

        if(sgStep || debug){
            printf("<<<<end cycle>>>>\n\n");
        }

        /*print INFO*/
        if(sgStep)
            printSingleStep();

         /*
            Special predPC condition caused by load-use hazard.
            Need to set PC back;
        */
        if(FReg.stall){
            FReg.stall = false;
            this->predPC = this->PC;
            machineStats.FSTALL ++;
        }
        

        if(DReg.stall)
            DReg.stall = false;
        else 
            DReg = DRegO;

        if(EReg.stall)
            EReg.stall = false;
        else 
            EReg = ERegO;

        if(MReg.stall)
            MReg.stall = false;
        else
            MReg = MRegO;

        if(WReg.stall)
            WReg.stall = false;
        else 
            WReg = WRegO;

        /*data forwading signal reset*/
        forwardA = false;
        forwardB = false;

        registers[ZEROREG] = 0;  //keep zero reg 0
        machineCycle ++;        //cycle + 1
        machineStats.cycle += TicksPerCycle;
    }
}

const char *instrName_cstr[60] ={
    "add", "mul", "sub", "sll", "mulh", "slt", "xor", "div", "srl", "sra",
    "or", "rem", "and", "lb", "lh", "lw", "ld", "addi", "slli", "slti",
    "xori", "srli", "srai", "ori", "andi", "addiw", "jalr", "ecall", "sb", "sh",
    "sw", "sd", "beq", "bne", "blt", "bge", "auipc", "lui", "jal", "bltu",
    "bgeu", "lbu", "lhu", "lwu", "sltiu", "sltu", "slliw", "srliw", "sraiw", "addw",
    "subw", "sllw", "srlw", "sraw", "nop", "", ""
};

const char *regName_cstr[33] = {
    "ZR ", "RA ", "SP ", "GP ", "TP ", "T0 ", "T1 ", "T2 ", "S0 ", "S1 ", 
    "A0 ", "A1 ", "A2 ", "A3 ", "A4 ", "A5 ", "A6 ", "A7 ", "S2 ", "S3 ",  
    "S4 ", "S5 ", "S6 ", "S7 ", "S8 ", "S9 ", "S10", "S11", "T3 ", "T4 ",  
    "T5 ", "T6 "
};

const char *pscmName[10] = {
    "Always Taken", "Always Not Taken", "Bimodal", "Self Adjustment"
};


void
Machine::PfmConfig(FILE *f){
    printf(".........Reading config file................\n");
    fflush(stdout);
    char buf[600 + 10];
    char *retVal = buf;
    const char *L1_Latency = "L1_Latency";
    const char *L1_Config = "L1_Config";
    const char *L2_Latency = "L2_Latency";
    const char *L2_Config = "L2_Config";
    const char *LLC_Latency = "LLC_Latency";
    const char *LLC_Config = "LLC_Config";
    const char *Mem_Latency = "Mem_Latency";

    for(int i = 0; i < INSTRNUM + 100; i++){
        retVal = fgets(buf, 600, f);
        if(retVal == NULL)
            break;

        if(buf[0] == '/' || buf[0] == '*' || buf[0] == ' ' || buf[0] == '\0')
            continue;

        char instName[40] = {};
        int instId = INSTRNUM - 1;
        int performance = 1;

        sscanf(buf, "%s %d", instName, &performance);

        for(int k = 0; k < 54; k++)
            if(strcmp(instName, instrName_cstr[k]) == 0){
                instId = k;
                break;
            }

        if(strcmp(L1_Latency, instName) == 0){
            int hit_lat = 0, bus_lat = 0;
            sscanf(buf, "%s %d %d", instName, &hit_lat, &bus_lat);
            L1_latency.hit_latency = hit_lat;
            L1_latency.bus_latency = bus_lat;
            printf("L1 hit:%d bus:%d\n", hit_lat, bus_lat);
        }
        else if(strcmp(L2_Latency, instName) == 0){
            int hit_lat = 0, bus_lat = 0;
            sscanf(buf, "%s %d %d", instName, &hit_lat, &bus_lat);
            L2_latency.hit_latency = hit_lat;
            L2_latency.bus_latency = bus_lat;
            printf("L2 hit:%d bus:%d\n", hit_lat, bus_lat);
        }
        else if(strcmp(LLC_Latency, instName) == 0){
            int hit_lat = 0, bus_lat = 0;
            sscanf(buf, "%s %d %d", instName, &hit_lat, &bus_lat);
            LLC_latency.hit_latency = hit_lat;
            LLC_latency.bus_latency = bus_lat;
            printf("LLC hit:%d bus:%d\n", hit_lat, bus_lat);
        }
        else if(strcmp(Mem_Latency, instName) == 0){
            int hit_lat = 0, bus_lat = 0;
            sscanf(buf, "%s %d %d", instName, &hit_lat, &bus_lat);
            Memory_latency.hit_latency = hit_lat;
            Memory_latency.bus_latency = bus_lat;
            printf("Memory hit:%d bus:%d\n", hit_lat, bus_lat);
        }
        else if(strcmp(L1_Config, instName) == 0){
            int conf_size, conf_associa, conf_wt, conf_wa;
            sscanf(buf, "%s %d %d %d %d", 
                instName , &conf_size, &conf_associa, &conf_wt, &conf_wa);
            L1_config.size = conf_size;
            L1_config.associativity = conf_associa;
            L1_config.set_num = L1_config.size / (L1_config.associativity * BLOCK_SIZE);
            if(L1_config.size % (L1_config.associativity * BLOCK_SIZE) != 0){
                printf("Please give proper cache config!\n");
                ASSERT(false);
            }
            L1_config.write_through = conf_wt;
            L1_config.write_allocate = conf_wa;
            printf("L1 size:%d associativity:%d\n", conf_size, conf_associa);
        }
        else if(strcmp(L2_Config, instName) == 0){
            int conf_size, conf_associa, conf_wt, conf_wa;
            sscanf(buf, "%s %d %d %d %d", 
                instName , &conf_size, &conf_associa, &conf_wt, &conf_wa);
            L2_config.size = conf_size;
            L2_config.associativity = conf_associa;
            L2_config.set_num = L2_config.size / (L2_config.associativity * BLOCK_SIZE);
            if(L2_config.size % (L2_config.associativity * BLOCK_SIZE) != 0){
                printf("Please give proper cache config!\n");
                ASSERT(false);
            }
            L2_config.write_through = conf_wt;
            L2_config.write_allocate = conf_wa;
            printf("L2 size:%d associativity:%d\n", conf_size, conf_associa);
        }
        else if(strcmp(LLC_Config, instName) == 0){
            int conf_size, conf_associa, conf_wt, conf_wa;
            sscanf(buf, "%s %d %d %d %d", 
                instName , &conf_size, &conf_associa, &conf_wt, &conf_wa);
            LLC_config.size = conf_size;
            LLC_config.associativity = conf_associa;
            LLC_config.set_num = LLC_config.size / (LLC_config.associativity * BLOCK_SIZE);
            if(LLC_config.size % (LLC_config.associativity * BLOCK_SIZE) != 0){
                printf("Please give proper cache config!\n");
                ASSERT(false);
            }
            LLC_config.write_through = conf_wt;
            LLC_config.write_allocate = conf_wa;
            printf("LLC size:%d associativity:%d\n", conf_size, conf_associa);
        }
        else
            instrPfm[instId] = performance;
    }
    printf(".........Reading config file succeed........\n\n");
}

void 
Machine::Halt(){
    machineStats.edTime = clock();
    int misP = machineStats.misPrediction;
    int sucP = machineStats.sucPrediction;
    
    double acc = 0;
    if(misP + sucP > 0)
        acc = (double)sucP / (double)(misP + sucP);


    double secs = (double)(machineStats.edTime - machineStats.stTime) / CLOCKS_PER_SEC;
    printf("Machine halting!\n");
    printf("----------STATS----------------------------\n");
    printf("Pipline Cycles:                     %d\n", machineCycle);
    printf("Total Ticks (cpu cycle):          %d\n", machineStats.cycle);
    printf("Program CPI:                        %.4f\n", 
            (double)machineStats.cycle / machineStats.instrCnt);
    printf("Instr:                              %d\n", machineStats.instrCnt); 
    printf("Seconds:                            %.4f\n", secs);
    printf("Data forwarding:                    %d\n", machineStats.dataHazard);
    printf("Load-use hazard:                    %d\n", machineStats.loadUseHazard);
    printf("Control hazard:                     %d\n", machineStats.controlHazard);
    printf("Branch prediction scheme:           %s\n", pscmName[MyPred.predScm]);
    printf("Branch prediction Acc:              %.3f (%d / %d)\n", acc, sucP, (misP + sucP));
    printf("ECALL num:                          %d\n", machineStats.ecallNum);
    printf("FSTALL num:                         %d\n", machineStats.FSTALL);

    StorageStats s;
    L1.GetStats(s);
    printf("\nCache L1 miss rate:%.4f (%d / %d)  access_time:%d cycle\n",
     (double)s.miss_num / (double)s.access_counter, s.miss_num, s.access_counter, s.access_time);

    L2.GetStats(s);
    printf("\nCache L2 miss rate:%.4f (%d / %d)  access_time:%d cycle\n",
     (double)s.miss_num / (double)s.access_counter, s.miss_num, s.access_counter, s.access_time);

    LLC.GetStats(s);
    printf("\nCache LLC miss rate:%.4f (%d / %d)  access_time:%d cycle\n",
     (double)s.miss_num / (double)s.access_counter, s.miss_num, s.access_counter, s.access_time);

    PhyMem.GetStats(s);
    StorageLatency tmpl;
    PhyMem.GetLatency(tmpl);
    printf("\nPhysical memory access_counter:%d access_time:%d cycle\n\n",
         s.access_counter, s.access_time);
    exit(0);
}

void
Machine::printReg(){
    for(int i = 0; i < REG_NUM; i++){
        int j = i;
        for(j = i; j < i + 4 && j < REG_NUM; j++)
            printf("%s:[%08x] ", regName_cstr[j], (uint32_t)registers[j]);

        printf("\n");
        i = j - 1;
    }
}

int skipCycle= 0;

void
Machine::printPipe(){
    printf("*****PIPLINE*****\n");
    fflush(stdout);
    if(FReg.bubble)
        printf("Fetch: \n-BUBBLE-\n");
    else if(FReg.stall)
        printf("Fetch: \n-STALL-\n");
    else
        printf("Fetch:     PC:%llx\n", this->PC);

    if(DReg.bubble)
        printf("Decode: \n-BUBBLE-\n");
    else if(DReg.stall)
        printf("Decode: \n-STALL-\n");
    else
        printf("Decode:    [%llx] instr:%s\n", 
            ERegO.instr.addr, instrName_cstr[ERegO.instr.name]);

    if(EReg.bubble)
        printf("Execute: \n-BUBBLE-\n");
    else if(EReg.stall)
        printf("Execute: \n-STALL-\n");
    else
        printf("Execute:   [%llx] instr:%s valE:%llx valC:%llx\n", 
            MRegO.instr.addr, instrName_cstr[EReg.instr.name], MRegO.valE, MRegO.valC);

    if(MReg.bubble)
        printf("MemStage: \n-BUBBLE-\n");
    else if(MReg.stall)
        printf("MemStage: \n-STALL-\n");
    else
        printf("MemStage:    [%llx] instr:%s valM:%llx valE:%llx\n", 
            WRegO.instr.addr, instrName_cstr[MReg.instr.name], WRegO.valM, WRegO.valE);

    if(WReg.bubble)
        printf("Writeback: \n-BUBBLE-\n");
    else if(WReg.stall)
        printf("Writeback: \n-STALL-\n");
    else
        printf("Writeback: [%llx] instr:%s dstM:%s dstE:%s\n",
            WReg.instr.addr, instrName_cstr[WReg.instr.name], regName_cstr[WReg.dstM], regName_cstr[WReg.dstE]);
}

void
Machine::printSingleStep(){
    char buf[20];

    uint64_t addr = 0;printf("*****PIPLINE*****\n");
                fflush(stdout);
                if(FReg.bubble)
                    printf("Fetch: \n-BUBBLE-\n");
                else if(FReg.stall)
                    printf("Fetch: \n-STALL-\n");
                else
                    printf("Fetch:     PC:%llx\n", this->PC);

                if(DReg.bubble)
                    printf("Decode: \n-BUBBLE-\n");
                else if(DReg.stall)
                    printf("Decode: \n-STALL-\n");
                else
                    printf("Decode:    [%llx] instr:%s\n", 
                        ERegO.instr.addr, instrName_cstr[ERegO.instr.name]);

                if(EReg.bubble)
                    printf("Execute: \n-BUBBLE-\n");
                else if(FReg.stall)
                    printf("Execute: \n-STALL-\n");
                else
                    printf("Execute:   [%llx] instr:%s valE:%llx valC:%llx\n", 
                        MRegO.instr.addr, instrName_cstr[EReg.instr.name], MRegO.valE, MReg.valC);

                if(MReg.bubble)
                    printf("MemStage: \n-BUBBLE-\n");
                else if(FReg.stall)
                    printf("MemStage: \n-STALL-\n");
                else
                    printf("MemStage:    [%llx] instr:%s valM:%llx valE:%llx\n", 
                        WRegO.instr.addr, instrName_cstr[MReg.instr.name], WRegO.valM, WRegO.valE);

                if(WReg.bubble)
                    printf("Writeback: \n-BUBBLE-\n");
                else if(FReg.stall)
                    printf("Writeback: \n-STALL-\n");
                else
                    printf("Writeback: [%llx] instr:%s dstM:%llx dstE:%llx\n",
                        WReg.instr.addr, instrName_cstr[WReg.instr.name], WReg.dstM, WReg.dstE);
    int size = 0;
    char c;
    int64_t memVal = 0;
    int regID = 0;

    while(true){
        while(skipCycle > 0){
            skipCycle --;
            return;
        }
        fgets(buf, 19, stdin);
        printf("echo %s\n", buf);

        switch(buf[0]){
            case 'c':
                return;
            case 'r':
                if(buf[1] == 'a'){
                    printReg();
                }
                else{
                    sscanf(buf, "%c %d", &c, &regID);
                    printf("reg[%d]:%llx\n", regID, registers[regID]);
                }
                break;
            case 'm':
                addr = 0;
                size = 0;
                c = '\0';
                sscanf(buf, "%c %llx %d", &c, &addr, &size);
                if(addr < 0 || addr > MEM_SIZE){
                    printf("wrong address\n");
                    break;
                }
                if(size != 1 && size != 2 && size != 4 && size != 8){
                    printf("wrong length\n");
                    break;
                }

                memVal = 0;
                readBytes(addr, size, &memVal);
                printf("%llx\n", memVal);
                break;
            case 'i':
                printPipe();
                break;
            case 's':
                skipCycle = 0;
                sscanf(buf, "%c %d", &c, &skipCycle);
                return;
                break;
            case 'q':
                Halt();
                break;
            default :
                printf("unknown command\n");
                printf("usage:\n");
                printf("c               ----continue runing next instruction\n");
                printf("rn              ----read register[n]\n");
                printf("ra              ----show all register\n");
                printf("m <addr> <size> ----read size bytes in memory at 0x0\n");
                printf("i               ----show instruction in five stage\n");
                printf("s <cycle>       ----skip certain cycles\n");
                printf("q               ----stop\n");                  
        }
    }
}


#include "memory.h"
#include "utils.h"
void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
    if(addr + bytes > memSize){
        printf("Memory Request addr:%lu bytes:%d\n", addr, bytes);
        fflush(stdout);
        ASSERT(false);
    }
    if(addr < 0)
        ASSERT(false);
    hit = 1;
    time = latency_.hit_latency + latency_.bus_latency;
    stats_.access_time += time;
    stats_.access_counter ++;
    //read bytes
    if(read){
        for(int i = 0; i < bytes; i++)
            content[i] = mainMem[addr + i];
    }
    else{       //write bytes
       for(int i = 0; i < bytes; i++)
            mainMem[addr + i] = content[i];
    }
}

Memory::Memory(int size){
    memSize = size;
    mainMem = new char[memSize];
    for(int i = 0; i < memSize; i++)
        mainMem[i] = 0;
}

Memory::~Memory(){
    delete []mainMem;
}



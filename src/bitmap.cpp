#include "bitmap.h"

BitMap::BitMap(int n){
    nbits = n;
    nint32 = nbits / 32 + 1;
    innerMap = new int[nint32];
    for(int i = 0; i < nint32; i++)
        innerMap[i] = 0;
}

BitMap::~BitMap(){
    delete []innerMap;
}

int
BitMap::FindSet(){
    for(int k = 0; k < nbits; k++){
        int whichInt = k / 32;
        int intOff = k % 32;
        if(!(innerMap[whichInt] & (1 << intOff) ) ){
            innerMap[whichInt] |= (1 << intOff);
            return k;
        }
    }
    return -1;
}

bool 
BitMap::Exist(int k){
    int whichInt = k / 32;
    int intOff = k % 32;
    if(innerMap[whichInt] & (1 << intOff))
        return true;
    else 
        return false;
}

int 
BitMap::Clear(int k){
    int whichInt = k / 32;
    int intOff = k % 32;
    if(innerMap[whichInt] & (1 << intOff)){
        innerMap[whichInt] &= ~(1 << intOff);
        return 0;
    }
    else 
        return -1;
}

void 
BitMap::Empty(){
    for(int i = 0; i < nint32; i++)
        innerMap[i] = 0;
}
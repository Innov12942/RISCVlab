#ifndef BITMAP_H
#define BITMAP_H
class BitMap{
public:
    int nbits;
    int nint32;
    int *innerMap;
    BitMap(int n);
    
    ~BitMap();
    int FindSet();
    bool Exist(int k);
    int Clear(int k);
    void Empty();
};

#endif
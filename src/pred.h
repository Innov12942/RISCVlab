#ifndef PRED_H
#define PRED_H

#include <stdint.h>
#include "utils.h"

#define BUFNUM 16
#define HISTORYN 4  //num of bits to save history status
                    // should not be more than 32 (sizeof(int))

enum Scheme{
    AlwaysTaken, AlwaysNotTaken, Bimodal, SelfAdj
};

class Predictor{
public:
    Predictor();
    bool Predict(int64_t addr);
    void update(bool success, int64_t addr);
    void SetScheme(Scheme scm);

    /*member variable*/
    Scheme predScm;
    char bitBuf[BUFNUM];

    char saBuf[BUFNUM][1  << HISTORYN];
    int saHis[BUFNUM];
};

#endif

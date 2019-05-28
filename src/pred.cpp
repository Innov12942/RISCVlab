#include "pred.h"
#include "utils.h"

inline bool twoBitChoose(char tb){
    if(tb == 0)
        return false;
    if(tb == 1)
        return false;
    if(tb == 2)
        return true;
    if(tb == 3)
        return true;

}

inline int maskHis(int his){
    return his & ((1 << HISTORYN) - 1);
}

Predictor::Predictor(){
    predScm = AlwaysTaken;
    for(int i = 0; i < BUFNUM; i++)
        bitBuf[i] = 0;
    for(int i = 0; i < BUFNUM; i++){
        saHis[i] = 0;
        for(int j = 0; j < (1 << HISTORYN); j++)
            saBuf[i][j] = 0;
    }
}

void
Predictor::SetScheme(Scheme scm){
    predScm = scm;
}

bool
Predictor::Predict(int64_t addr){
    int bufID = (addr >> 2) % BUFNUM;
    ASSERT((bufID >= 0 && bufID < 256));
    if(predScm == AlwaysTaken)
        return true;
    if(predScm == AlwaysNotTaken)
        return false;

    if(predScm == Bimodal){
        return twoBitChoose(bitBuf[bufID]);
    }

    if(predScm == SelfAdj){
        for(int i = 0; i < (1 << HISTORYN); i++)
            if(maskHis(saHis[bufID]) == i){
                return twoBitChoose(saBuf[bufID][i]);
            }
    }
    return false;
}

void
Predictor::update(bool success, int64_t addr){

    int bufID = (addr >> 2) % BUFNUM;

    ASSERT((bufID >= 0 && bufID < 256));
    if(predScm == AlwaysTaken)
        return ;
    if(predScm == AlwaysNotTaken)
        return ;

    if(predScm == Bimodal){
        switch(bitBuf[bufID]){
            case 0:
                if(success)
                    bitBuf[bufID] = 0;
                else
                    bitBuf[bufID] = 1;
                break;
            case 1:
                if(success)
                    bitBuf[bufID] = 0;
                else
                    bitBuf[bufID] = 2;
                break;
            case 2:
                if(success)
                    bitBuf[bufID] = 3;
                else 
                    bitBuf[bufID] = 1;
                break;
            case 3:
                if(success)
                    bitBuf[bufID] = 3;
                else
                    bitBuf[bufID] = 2;
                break;
        }
        return;
    }

    if(predScm == SelfAdj){
        int lastHisMode = 0;
        for(int i = 0; i < (1 << HISTORYN); i++)
            if(maskHis(saHis[bufID]) == i){
                lastHisMode = i;
                break;
            }

        int lastJump = 0;
        bool lastPred = twoBitChoose(saBuf[bufID][lastHisMode]);
        lastJump = (!success) ^ lastPred;

        lastJump &= 0x1;
        saHis[bufID] = (maskHis(saHis[bufID]) << 1) | lastJump;

        switch(saBuf[bufID][lastHisMode]){
            case 0:
                if(success)
                    saBuf[bufID][lastHisMode] = 0;
                else
                    saBuf[bufID][lastHisMode] = 1;
                break;
            case 1:
                if(success)
                    saBuf[bufID][lastHisMode] = 0;
                else
                    saBuf[bufID][lastHisMode] = 2;
                break;
            case 2:
                if(success)
                    saBuf[bufID][lastHisMode] = 3;
                else 
                    saBuf[bufID][lastHisMode] = 1;
                break;
            case 3:
                if(success)
                    saBuf[bufID][lastHisMode] = 3;
                else
                    saBuf[bufID][lastHisMode] = 2;
                break;
        }
        return;
    }
}

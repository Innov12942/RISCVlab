#include "pred.h"
#include "utils.h"

Predictor::Predictor(){
    predScm = AlwaysTaken;
    bitBuf = 0;
}

void
Predictor::SetScheme(Scheme scm){
    predScm = scm;
}

bool
Predictor::Predict(){
    if(predScm == AlwaysTaken)
        return true;
    if(predScm == AlwaysNotTaken)
        return false;

    if(predScm == Bimodal){
        if(bitBuf == 0)
            return false;
        if(bitBuf == 1)
            return false;
        if(bitBuf == 2)
            return true;
        if(bitBuf == 3)
            return true;
    }
}

void
Predictor::updata(bool success){
     if(predScm == AlwaysTaken)
        return ;
    if(predScm == AlwaysNotTaken)
        return ;

    if(predScm == Bimodal){
        switch(bitBuf){
            case 0:
                if(success)
                    bitBuf = 0;
                else
                    bitBuf = 1;
            case 1:
                if(success)
                    bitBuf = 1;
                else
                    bitBuf = 2;
            case 2:
                if(success)
                    bitBuf = 3;
                else 
                    bitBuf = 1;
            case 3:
                if(success)
                    bitBuf = 3;
                else
                    bitBuf = 2;
        }
    }
}

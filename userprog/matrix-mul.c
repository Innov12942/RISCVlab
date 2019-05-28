#include <stdio.h>
#include "syscall.h"
#define matDim 3
int matA[matDim][matDim] = {{1,2,3},{4,5,6},{7,8,9}};
int matB[matDim][matDim] = {{1,2,3},{4,5,6},{7,8,9}};
int matC[matDim][matDim] = {};
int matMul()
{
    for(int i = 0; i < matDim; i++)
        for(int j = 0; j < matDim; j++){
            for(int k = 0; k < matDim; k++)
                matC[i][j] += matA[i][k] * matB[k][j];
        }


}

int main()
{
    matMul();

    for(int i = 0; i < matDim; i++){
        for(int j = 0; j < matDim; j++)
            printfint(matC[i][j]);
    }
    return 0;
}

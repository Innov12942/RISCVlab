#include <cstdio>
using namespace std;

int64_t sig64ext(int len, uint32_t val){
    if((val & (1 << len)) > 0)
        val = val | (0xffffffff << (len + 1));

    return (int64_t)((int32_t)val);
}

int main(){
    printf("%lld\n", sig64ext(11, 0xfff));
    printf("%lld\n", sig64ext(11, 0x7ff));
}


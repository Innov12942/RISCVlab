#ifndef CACHE_DEF_H_
#define CACHE_DEF_H_

#include <stdlib.h>

#define TRUE 1
#define FALSE 0

#define ASSERT(condition)                                                   \
        if(!(condition)) {                                                    \
            fprintf(stderr, "ASSERTION failed on line %d file \"%s\" \n",   \
            __LINE__, __FILE__);                                            \
            fflush(stderr);                                                 \
            abort();                                                        \
        }

#endif //CACHE_DEF_H_

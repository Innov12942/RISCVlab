#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <cstdio>

#define ASSERT(condition)                                                   \
        if(!(condition)) {                                                    \
            fprintf(stderr, "ASSERTION failed on line %d file \"%s\" \n",   \
            __LINE__, __FILE__);                                            \
            fflush(stderr);                                                 \
            abort();                                                        \
        }


#endif
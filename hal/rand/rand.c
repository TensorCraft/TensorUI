#include "rand.h"
#include <stdlib.h>

unsigned int hal_rand(void) {
    return (unsigned int)rand();
}

void hal_srand(unsigned int seed) {
    srand(seed);
}

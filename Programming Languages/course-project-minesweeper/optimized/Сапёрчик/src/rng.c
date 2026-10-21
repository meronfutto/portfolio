#include "rng.h"

static uint32_t rng_state = 0x12345678u;

void rng_seed(uint32_t seed) {
    if (seed == 0) seed = 0x12345678u;
    rng_state = seed;
}

uint32_t rng_u32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

// Генератор случайного числа от 0 до bound. 
uint32_t rng_bounded(uint32_t bound) {
    if (bound == 0) return 0;

    uint32_t threshold = (uint32_t)(0u - bound) % bound; // 2^32 % bound
    for (;;) {
        uint32_t r = rng_u32();
        if (r >= threshold) return r % bound;
    }
}
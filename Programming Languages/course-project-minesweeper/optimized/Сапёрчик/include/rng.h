#pragma once
#include <stdint.h>

/*
 * rng.h
 * ѕростой и быстрый генератор псевдослучайных чисел (xorshift32).
 *
 * »спользование:
 *   rng_seed(seed);              // инициализаци€ генератора
 *   rng_u32();      // случайное 32-битное число
 *   rng_bounded(n); // 
 */

void rng_seed(uint32_t seed);
uint32_t rng_u32(void);
uint32_t rng_bounded(uint32_t bound);
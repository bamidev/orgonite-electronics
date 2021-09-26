#ifndef _FACTORIZATION_H
#define _FACTORIZATION_H

#include <stdint.h>



/// Factorizes the given `val` into two factors, by finding the first factor that is equal or greater than `min` and less than 256.
/// If no factor could be found, returns `0`.
/// Also, `min` can not be `0`.
uint8_t factorize_8(uint32_t val, uint8_t min);

/// Finds a number that is not an actual factor, but is a factor of a number that is the closest to `val`.
uint8_t find_approximate_factor_8_16(uint32_t val, uint8_t min, uint16_t* factor2);



#endif//_FACTORIZATION_H
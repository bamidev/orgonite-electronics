#include "factorization.h"

#include <assert.h>
#include <math.h>



uint8_t factorize_8(uint32_t val, uint8_t min) {
	assert(min > 0);

	for (uint8_t i = min; i != 0; i++) {
		if (val % i == 0)
			return i;
	}

	return 0;
}

uint8_t find_approximate_factor_8_16(uint32_t val, uint8_t min, uint16_t* factor2) {
	assert(min > 0);

	double vald = val;
	uint8_t closest_factor = min;
	double closest_fraction = 1.0;

	for (uint8_t i = min; i != 0; i++) {
		
		double f2 = vald / i;
		double fraction = f2 - (uint32_t)f2;

		// If closer, remember
		if (fraction < closest_fraction) {
			closest_factor = i;
			closest_fraction = fraction;
			*factor2 = round(f2);
		}
	}

	return closest_factor;
}
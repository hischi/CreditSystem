#include "checksum.h"

#define MAGIC_PRIME 65521
uint32_t calculate_checksum(uint16_t *buf, uint32_t len) {

	uint32_t sum1 = 0;
	uint32_t sum2 = 0;

	len /= 2;
	for (uint32_t i = 0; i < len; i++)
	{
		sum1 = (sum1 + buf[i]) % MAGIC_PRIME;
		sum2 = (sum2 + sum1) % MAGIC_PRIME;

		#ifndef WIN32
		log(LL_DEBUG, LM_CS, "Word: ", (uint32_t) buf[i]);
		log(LL_DEBUG, LM_CS, "Sum1: ", sum1);
		log(LL_DEBUG, LM_CS, "Sum2: ", sum2);
		#endif
		
	}

	return ((sum2 << 16) | sum1);
}
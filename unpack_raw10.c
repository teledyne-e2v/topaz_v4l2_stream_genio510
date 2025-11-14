#include <stdint.h>

#include "unpack_raw10.h"

/**
 * unpack MTISP RAW10 data into 16bit values
 * 4 x RAW10 = 40 bits = 5 x 8 bytes of input data
 */
void unpack_image(const void * _src, void * _dst, int dst_len) {
	const uint8_t *src = (const uint8_t *)_src;
	uint16_t *dst = (uint16_t *)_dst;

	while (dst_len > 0) {
		uint64_t *chunk = (uint64_t *)src;

		dst[0] = (*chunk >>  0) & 0x3FF;
		dst[1] = (*chunk >> 10) & 0x3FF;
		dst[2] = (*chunk >> 20) & 0x3FF;
		dst[3] = (*chunk >> 30) & 0x3FF;

		dst_len -= 2 * 4;
		dst += 4;
		src += 5;
	}
}

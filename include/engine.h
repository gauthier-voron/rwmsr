#ifndef ENGINE_H
#define ENGINE_H


#include <stdint.h>
#include <stdlib.h>

#include "rwmsr.h"


#define COMMAND_PRINT   (1 << 0)
#define COMMAND_HEXA    (1 << 1)
#define COMMAND_WRITE   (1 << 2)
#define COMMAND_DELAY   (1 << 3)
#define COMMAND_REPEAT  (1 << 4)


struct command
{
	uint8_t   flags;
	msradr_t  address;
	msrval_t  value;
	uint32_t  delay;
	uint32_t  repeat;
};


void execute(const struct command *commands, size_t mlen, const uint8_t *cores,
	     size_t rlen);


#endif

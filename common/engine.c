#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "engine.h"


static uint8_t stopped = 0;


static void handle_signal(int signum __attribute__((unused)))
{
	stopped = 1;
}


static uint64_t getnow(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000ul;
}


static void print_header(const struct command *commands, size_t mlen,
			 const uint8_t *cores, size_t rlen)
{
	size_t i, j;
	char *ptype, *pdec = ":", *phex = "::";
	
	printf("time ");
	for (i=0; i<mlen; i++) {
		if (!(commands[i].flags & COMMAND_PRINT))
			continue;
		if (commands[i].flags & COMMAND_HEXA)
			ptype = phex;
		else
			ptype = pdec;
		
		for (j=0; j<rlen; j++)
			printf("%s0x%lx(%u) ", ptype, commands[i].address,
			       cores[j]);
	}
	printf("\n");
}

static void print_data(const uint64_t *times, const msrval_t *values,
		       const struct command *commands, size_t mlen,
		       size_t rlen, uint64_t start, uint64_t now)
{
	size_t i, j;
	const char *fmt;
	const char *dfmt = " %lu";
	const char *hfmt = " %lx";

	for (i=0; i<mlen; i++) {
		if (!(commands[i].flags & COMMAND_PRINT))
			continue;
		if (times[i] > now || times[i] == 0)
			continue;
		break;
	}

	if (i == mlen)
		return;
	
	printf("%lu.%03lu", (now - start) / 1000, (now - start) % 1000);
		
	for (i=0; i<mlen; i++) {
		if (!(commands[i].flags & COMMAND_PRINT))
			continue;
		
		if (times[i] > now || times[i] == 0) {
			for (j=0; j<rlen; j++)
				printf(" -");
		} else {
			if (commands[i].flags & COMMAND_HEXA)
				fmt = hfmt;
			else
				fmt = dfmt;
			
			for (j=0; j<rlen; j++)
				printf(fmt, values[i * rlen + j]);
		}
	}

	printf("\n");
	fflush(stdout);
}


static void apply_commands(msrval_t *values, const msradr_t *addresses,
			   const uint64_t *times,
			   const struct command *commands, size_t mlen,
			   const uint8_t *cores, size_t rlen, uint64_t now)
{
	size_t i, j, ret;

	for (i=0; i<mlen; i++) {
		if (times[i] > now || times[i] == 0)
			goto end;;

		if (commands[i].flags & COMMAND_WRITE) {
			ret = rwmsr_arr(addresses, values, cores, rlen);
		} else {
			ret = rdmsr_arr(values, addresses, cores, rlen);
		}

		if (ret == rlen)
			goto end;

		for (j=0; j<rlen; j++)
			values[j] = 0;

	end:
		values += rlen;
		addresses += rlen;
	}
}


static void setup_start_data(msradr_t *addresses,
			     const struct command *commands, size_t mlen,
			     size_t rlen)
{
	size_t i, j;

	for (i=0; i<mlen; i++)
		for (j=0; j<rlen; j++)
			addresses[i * rlen + j] = commands[i].address;
}

static void setup_start_times(uint64_t *times, const struct command *commands,
			      size_t mlen, uint64_t start)
{
	size_t i;

	for (i=0; i<mlen; i++) {
		if (commands[i].flags & COMMAND_DELAY)
			times[i] = start + commands[i].delay;
		else
			times[i] = start;
	}
}

static void setup_next_data(msrval_t *values,
			    const struct command *commands, size_t mlen,
			    size_t rlen)
{
	size_t i, j;

	for (i=0; i<mlen; i++)
		for (j=0; j<rlen; j++)
			if (commands[i].flags & COMMAND_WRITE)
				values[i * rlen + j] = commands[i].value;
}

static uint64_t setup_next_times(uint64_t *times,
				 const struct command *commands, size_t mlen,
				 uint64_t now)
{
	size_t i;
	uint64_t nearest = ~(0ul);

	for (i=0; i<mlen; i++) {
		if (times[i] == 0)
			continue;
		if (times[i] > now)
			goto end;
		
		if (commands[i].flags & COMMAND_REPEAT) {
			while (times[i] <= now)
				times[i] += commands[i].repeat;
		} else {
			times[i] = 0;
			continue;
		}

	end:
		if (times[i] > now && times[i] < nearest)
			nearest = times[i];
	}

	return nearest;
}


void execute(const struct command *commands, size_t mlen, const uint8_t *cores,
	     size_t rlen)
{
	uint64_t *times = alloca(mlen * sizeof(uint64_t));
	uint64_t start = getnow(), now, next;
	struct timespec ts;
	msrval_t *values;
	msradr_t *addresses;

	values = alloca(mlen * rlen * sizeof (msrval_t));
	addresses = alloca(mlen * rlen * sizeof (msradr_t));
	
	print_header(commands, mlen, cores, rlen);

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGQUIT, handle_signal);
	
	setup_start_data(addresses, commands, mlen, rlen);
	setup_next_data(values, commands, mlen, rlen);
	setup_start_times(times, commands, mlen, start);

	while (!stopped) {
		now = getnow();

		apply_commands(values, addresses, times, commands, mlen, cores,
			       rlen, now);

		print_data(times, values, commands, mlen, rlen, start, now);

		next = setup_next_times(times, commands, mlen, now);
		if (next == ~(0ul))
			break;
		setup_next_data(values, commands, mlen, rlen);

		ts.tv_sec  =  (next - now)             / 1000;
		ts.tv_nsec = ((next - now) - ts.tv_sec * 1000) * 1000000;
		nanosleep(&ts, NULL);
	}
}

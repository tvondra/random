#ifndef PG_PRNG_H
#define PG_PRNG_H

typedef struct pg_prng_state
{
	uint64		s0,
				s1;
} pg_prng_state;

void pg_prng_seed(pg_prng_state *state, uint64 seed);

extern uint64 pg_prng_uint64(pg_prng_state *state);
extern uint32 pg_prng_uint32(pg_prng_state *state);
extern double pg_prng_double(pg_prng_state *state);

#endif
